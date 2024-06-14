#pragma once
#include <chrono>

#include "counter.hpp"

namespace ylt::metric {
class gauge_t : public counter_t {
 public:
  gauge_t(std::string name, std::string help)
      : counter_t(std::move(name), std::move(help)) {
    set_metric_type(MetricType::Gauge);
  }
  gauge_t(std::string name, std::string help,
          std::vector<std::string> labels_name)
      : counter_t(std::move(name), std::move(help), std::move(labels_name)) {
    set_metric_type(MetricType::Gauge);
  }

  gauge_t(std::string name, std::string help,
          std::map<std::string, std::string> labels)
      : counter_t(std::move(name), std::move(help), std::move(labels)) {
    set_metric_type(MetricType::Gauge);
  }

  void dec(double value = 1) {
#ifdef __APPLE__
    mac_os_atomic_fetch_sub(&default_lable_value_, value);
#else
    default_lable_value_ -= value;
#endif
  }

  void dec(const std::vector<std::string>& labels_value, double value = 1) {
    if (value == 0) {
      return;
    }

    validate(labels_value, value);
    if (use_atomic_) {
      if (labels_value != labels_value_) {
        throw std::invalid_argument(
            "the given labels_value is not match with origin labels_value");
      }
      set_value<true>(atomic_value_map_[labels_value], value, op_type_t::DEC);
    }
    else {
      std::lock_guard lock(mtx_);
      set_value<false>(value_map_[labels_value], value, op_type_t::DEC);
    }
  }
};
}  // namespace ylt::metric