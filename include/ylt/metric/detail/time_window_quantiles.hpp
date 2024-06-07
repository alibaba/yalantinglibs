#pragma once
#include "ckms_quantiles.hpp"
// https://github.com/jupp0r/prometheus-cpp/blob/master/core/include/prometheus/detail/time_window_quantiles.h

namespace ylt {
class TimeWindowQuantiles {
  using Clock = std::chrono::steady_clock;

 public:
  TimeWindowQuantiles(const std::vector<CKMSQuantiles::Quantile>& quantiles,
                      Clock::duration max_age_seconds, int age_buckets)
      : quantiles_(quantiles),
        ckms_quantiles_(age_buckets, CKMSQuantiles(quantiles_)),
        current_bucket_(0),
        last_rotation_(Clock::now()),
        rotation_interval_(max_age_seconds / age_buckets) {}

  double get(double q) const {
    CKMSQuantiles& current_bucket = rotate();
    return current_bucket.get(q);
  }
  void insert(double value) {
    rotate();
    for (auto& bucket : ckms_quantiles_) {
      bucket.insert(value);
    }
  }

 private:
  CKMSQuantiles& rotate() const {
    auto delta = Clock::now() - last_rotation_;
    while (delta > rotation_interval_) {
      ckms_quantiles_[current_bucket_].reset();

      if (++current_bucket_ >= ckms_quantiles_.size()) {
        current_bucket_ = 0;
      }

      delta -= rotation_interval_;
      last_rotation_ += rotation_interval_;
    }
    return ckms_quantiles_[current_bucket_];
  }

  const std::vector<CKMSQuantiles::Quantile>& quantiles_;
  mutable std::vector<CKMSQuantiles> ckms_quantiles_;
  mutable std::size_t current_bucket_;

  mutable Clock::time_point last_rotation_;
  const Clock::duration rotation_interval_;
};
}  // namespace ylt