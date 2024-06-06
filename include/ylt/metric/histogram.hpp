
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
        sum_(std::make_shared<gauge_t>("", "")) {
    if (!is_strict_sorted(begin(bucket_boundaries_), end(bucket_boundaries_))) {
      throw std::invalid_argument("Bucket Boundaries must be strictly sorted");
    }

    for (size_t i = 0; i < buckets.size() + 1; i++) {
      bucket_counts_.push_back(std::make_shared<counter_t>("", ""));
    }
    use_atomic_ = true;
  }

  void observe(double value) {
    const auto bucket_index = static_cast<std::size_t>(
        std::distance(bucket_boundaries_.begin(),
                      std::lower_bound(bucket_boundaries_.begin(),
                                       bucket_boundaries_.end(), value)));
    sum_->inc(value);
    bucket_counts_[bucket_index]->inc();
  }

  auto get_bucket_counts() { return bucket_counts_; }

  void serialize(std::string& str) override {
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

 private:
  template <class ForwardIterator>
  bool is_strict_sorted(ForwardIterator first, ForwardIterator last) {
    return std::adjacent_find(first, last,
                              std::greater_equal<typename std::iterator_traits<
                                  ForwardIterator>::value_type>()) == last;
  }

  std::vector<double> bucket_boundaries_;
  std::vector<std::shared_ptr<counter_t>> bucket_counts_;  // readonly
  std::shared_ptr<gauge_t> sum_;
};
}  // namespace ylt