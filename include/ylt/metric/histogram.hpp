
#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "counter.hpp"
#include "metric.hpp"

namespace ylt {
class histogram_t : public metric_t {
 public:
  histogram_t(std::string name, std::string help, std::vector<double> buckets)
      : bucket_boundaries_(buckets),
        metric_t(MetricType::Histogram, std::move(name), std::move(help)),
        sum_(std::make_shared<guage_t>()) {
    if (!is_strict_sorted(begin(bucket_boundaries_), end(bucket_boundaries_))) {
      throw std::invalid_argument("Bucket Boundaries must be strictly sorted");
    }

    for (size_t i = 0; i < buckets.size() + 1; i++) {
      bucket_counts_.push_back(std::make_shared<counter_t>());
    }
  }

  void observe(double value) {
    const auto bucket_index = static_cast<std::size_t>(
        std::distance(bucket_boundaries_.begin(),
                      std::lower_bound(bucket_boundaries_.begin(),
                                       bucket_boundaries_.end(), value)));

    std::lock_guard guard(mtx_);
    std::lock_guard<std::mutex> lock(mutex_);
    sum_->inc({}, value);
    bucket_counts_[bucket_index]->inc();
  }

  void observe(const std::vector<std::string>& label, double value) {
    const auto bucket_index = static_cast<std::size_t>(
        std::distance(bucket_boundaries_.begin(),
                      std::lower_bound(bucket_boundaries_.begin(),
                                       bucket_boundaries_.end(), value)));

    std::lock_guard guard(mtx_);
    std::lock_guard<std::mutex> lock(mutex_);
    sum_->inc(label, value);
    bucket_counts_[bucket_index]->inc(label);
  }

  void reset() {
    std::lock_guard guard(mtx_);
    for (auto& c : bucket_counts_) {
      c->reset();
    }

    sum_->reset();
  }

  auto bucket_counts() {
    std::lock_guard guard(mtx_);
    return bucket_counts_;
  }

  void serialize(std::string& str) override {
    if (sum_->values(false).empty()) {
      return;
    }
    str.append("# HELP ").append(name_).append(" ").append(help_).append("\n");
    str.append("# TYPE ")
        .append(name_)
        .append(" ")
        .append(metric_name())
        .append("\n");
    double count = 0;
    for (size_t i = 0; i < bucket_counts_.size(); i++) {
      auto counter = bucket_counts_[i];
      auto values = counter->values(false);
      for (auto& [labels_value, sample] : values) {
        str.append(name_).append("_bucket{");
        if (i == bucket_boundaries_.size()) {
          str.append("le=\"").append("+Inf").append("\"} ");
        }
        else {
          str.append("le=\"")
              .append(std::to_string(bucket_boundaries_[i]))
              .append("\"} ");
        }

        count += sample.value;
        str.append(std::to_string(count));
        if (enable_timestamp_) {
          str.append(" ").append(std::to_string(sample.timestamp));
        }
        str.append("\n");
      }
    }

    str.append(name_)
        .append("_sum ")
        .append(std::to_string((sum_->values(false)[{}].value)))
        .append("\n");

    str.append(name_)
        .append("_count ")
        .append(std::to_string(count))
        .append("\n");
  }

 private:
  template <class ForwardIterator>
  bool is_strict_sorted(ForwardIterator first, ForwardIterator last) {
    return std::adjacent_find(first, last,
                              std::greater_equal<typename std::iterator_traits<
                                  ForwardIterator>::value_type>()) == last;
  }

  std::vector<double> bucket_boundaries_;
  std::mutex mutex_;
  std::vector<std::shared_ptr<counter_t>> bucket_counts_;
  std::shared_ptr<guage_t> sum_;
};
}  // namespace ylt