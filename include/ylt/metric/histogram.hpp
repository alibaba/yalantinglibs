
#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "counter.hpp"
#include "metric.hpp"

namespace ylt::metric {
#ifdef CINATRA_ENABLE_METRIC_JSON
struct json_histogram_metric_t {
  std::map<std::string, std::string> labels;
  std::map<double, int64_t> quantiles;
  int64_t count;
  double sum;
};
REFLECTION(json_histogram_metric_t, labels, quantiles, count, sum);
struct json_histogram_t {
  std::string name;
  std::string help;
  std::string type;
  std::vector<json_histogram_metric_t> metrics;
};
REFLECTION(json_histogram_t, name, help, type, metrics);
#endif

class histogram_t : public metric_t {
 public:
  histogram_t(std::string name, std::string help, std::vector<double> buckets)
      : bucket_boundaries_(buckets),
        metric_t(MetricType::Histogram, std::move(name), std::move(help)),
        sum_(std::make_shared<gauge_t>("", "")) {
    if (!is_strict_sorted(begin(bucket_boundaries_), end(bucket_boundaries_))) {
      throw std::invalid_argument("Bucket Boundaries must be strictly sorted");
    }

    for (size_t i = 0; i < buckets.size() + 1; i++) {
      bucket_counts_.push_back(std::make_shared<counter_t>("", ""));
    }
    use_atomic_ = true;
  }

  histogram_t(std::string name, std::string help, std::vector<double> buckets,
              std::vector<std::string> labels_name)
      : bucket_boundaries_(buckets),
        metric_t(MetricType::Histogram, name, help, labels_name),
        sum_(std::make_shared<gauge_t>(name, help, labels_name)) {
    if (!is_strict_sorted(begin(bucket_boundaries_), end(bucket_boundaries_))) {
      throw std::invalid_argument("Bucket Boundaries must be strictly sorted");
    }

    for (size_t i = 0; i < buckets.size() + 1; i++) {
      bucket_counts_.push_back(
          std::make_shared<counter_t>(name, help, labels_name));
    }
  }

  histogram_t(std::string name, std::string help, std::vector<double> buckets,
              std::map<std::string, std::string> labels)
      : bucket_boundaries_(buckets),
        metric_t(MetricType::Histogram, name, help, labels),
        sum_(std::make_shared<gauge_t>(name, help, labels)) {
    if (!is_strict_sorted(begin(bucket_boundaries_), end(bucket_boundaries_))) {
      throw std::invalid_argument("Bucket Boundaries must be strictly sorted");
    }

    for (size_t i = 0; i < buckets.size() + 1; i++) {
      bucket_counts_.push_back(std::make_shared<counter_t>(name, help, labels));
    }
    use_atomic_ = true;
  }

  void observe(double value) {
    if (!use_atomic_ || !labels_name_.empty()) {
      throw std::invalid_argument("not a default label metric");
    }

    const auto bucket_index = static_cast<std::size_t>(
        std::distance(bucket_boundaries_.begin(),
                      std::lower_bound(bucket_boundaries_.begin(),
                                       bucket_boundaries_.end(), value)));
    sum_->inc(value);
    bucket_counts_[bucket_index]->inc();
  }

  void observe(const std::vector<std::string> &labels_value, double value) {
    if (sum_->labels_name().empty()) {
      throw std::invalid_argument("not a label metric");
    }

    const auto bucket_index = static_cast<std::size_t>(
        std::distance(bucket_boundaries_.begin(),
                      std::lower_bound(bucket_boundaries_.begin(),
                                       bucket_boundaries_.end(), value)));
    sum_->inc(labels_value, value);
    bucket_counts_[bucket_index]->inc(labels_value);
  }

  auto get_bucket_counts() { return bucket_counts_; }

  metric_hash_map<double> value_map() override { return sum_->value_map(); }

  void serialize(std::string &str) override {
    if (!sum_->labels_name().empty()) {
      serialize_with_labels(str);
      return;
    }

    serialize_head(str);
    double count = 0;
    auto bucket_counts = get_bucket_counts();
    for (size_t i = 0; i < bucket_counts.size(); i++) {
      auto counter = bucket_counts[i];
      str.append(name_).append("_bucket{");
      if (i == bucket_boundaries_.size()) {
        str.append("le=\"").append("+Inf").append("\"} ");
      }
      else {
        str.append("le=\"")
            .append(std::to_string(bucket_boundaries_[i]))
            .append("\"} ");
      }

      count += counter->value();
      str.append(std::to_string(count));
      str.append("\n");
    }

    str.append(name_)
        .append("_sum ")
        .append(std::to_string(sum_->value()))
        .append("\n");

    str.append(name_)
        .append("_count ")
        .append(std::to_string(count))
        .append("\n");
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  void serialize_to_json(std::string &str) override {
    if (!sum_->labels_name().empty()) {
      serialize_to_json_with_labels(str);
      return;
    }

    json_histogram_t hist{name_, help_, std::string(metric_name())};

    double count = 0;
    auto bucket_counts = get_bucket_counts();
    json_histogram_metric_t metric{};
    for (size_t i = 0; i < bucket_counts.size(); i++) {
      auto counter = bucket_counts[i];

      count += counter->value();

      if (i == bucket_boundaries_.size()) {
        metric.quantiles.emplace(std::numeric_limits<int>::max(),
                                 (int64_t)count);
      }
      else {
        metric.quantiles.emplace(bucket_boundaries_[i],
                                 (int64_t)counter->value());
      }
    }
    metric.count = (int64_t)count;
    metric.sum = sum_->value();

    hist.metrics.push_back(std::move(metric));

    iguana::to_json(hist, str);
  }
#endif

 private:
  template <class ForwardIterator>
  bool is_strict_sorted(ForwardIterator first, ForwardIterator last) {
    return std::adjacent_find(first, last,
                              std::greater_equal<typename std::iterator_traits<
                                  ForwardIterator>::value_type>()) == last;
  }

  void serialize_with_labels(std::string &str) {
    serialize_head(str);

    auto bucket_counts = get_bucket_counts();

    auto value_map = sum_->value_map();
    for (auto &[labels_value, value] : value_map) {
      if (value == 0) {
        continue;
      }

      double count = 0;
      for (size_t i = 0; i < bucket_counts.size(); i++) {
        auto counter = bucket_counts[i];
        str.append(name_).append("_bucket{");
        build_label_string(str, sum_->labels_name(), labels_value);
        str.append(",");

        if (i == bucket_boundaries_.size()) {
          str.append("le=\"").append("+Inf").append("\"} ");
        }
        else {
          str.append("le=\"")
              .append(std::to_string(bucket_boundaries_[i]))
              .append("\"} ");
        }

        count += counter->value(labels_value);
        str.append(std::to_string(count));
        str.append("\n");
      }

      str.append(name_);
      str.append("_sum{");
      build_label_string(str, sum_->labels_name(), labels_value);
      str.append("} ");

      if (type_ == MetricType::Counter) {
        str.append(std::to_string((int64_t)value));
      }
      else {
        str.append(std::to_string(value));
      }
      str.append("\n");

      str.append(name_).append("_count{");
      build_label_string(str, sum_->labels_name(), labels_value);
      str.append("} ");
      str.append(std::to_string(count));
      str.append("\n");
    }
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  void serialize_to_json_with_labels(std::string &str) {
    json_histogram_t hist{name_, help_, std::string(metric_name())};
    auto bucket_counts = get_bucket_counts();

    auto value_map = sum_->value_map();
    for (auto &[labels_value, value] : value_map) {
      if (value == 0) {
        continue;
      }

      size_t count = 0;
      json_histogram_metric_t metric{};
      for (size_t i = 0; i < bucket_counts.size(); i++) {
        auto counter = bucket_counts[i];

        count += counter->value(labels_value);

        if (i == bucket_boundaries_.size()) {
          metric.quantiles.emplace(std::numeric_limits<int>::max(),
                                   (int64_t)count);
        }
        else {
          metric.quantiles.emplace(bucket_boundaries_[i],
                                   (int64_t)counter->value(labels_value));
        }
      }
      metric.count = (int64_t)count;
      metric.sum = sum_->value(labels_value);

      for (size_t i = 0; i < labels_value.size(); i++) {
        metric.labels[sum_->labels_name()[i]] = labels_value[i];
      }

      hist.metrics.push_back(std::move(metric));
    }

    iguana::to_json(hist, str);
  }
#endif

  std::vector<double> bucket_boundaries_;
  std::vector<std::shared_ptr<counter_t>> bucket_counts_;  // readonly
  std::shared_ptr<gauge_t> sum_;
};
}  // namespace ylt::metric