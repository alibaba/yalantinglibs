#pragma once
#include <chrono>

#include "counter.hpp"

namespace ylt::metric {

template <typename value_type>
class basic_gauge : public basic_counter<value_type> {
  using metric_t::set_metric_type;
  using basic_counter<value_type>::validate;
  using metric_t::use_atomic_;
  using basic_counter<value_type>::default_label_value_;
  using metric_t::labels_value_;
  using basic_counter<value_type>::atomic_value_map_;
  using basic_counter<value_type>::value_map_;
  using basic_counter<value_type>::mtx_;
  using basic_counter<value_type>::stat_metric;
  using basic_counter<value_type>::set_value_static;
  using basic_counter<value_type>::set_value_dynamic;

 public:
  basic_gauge(std::string name, std::string help, size_t dupli_count = 2)
      : basic_counter<value_type>(std::move(name), std::move(help),
                                  dupli_count) {
    set_metric_type(MetricType::Gauge);
  }
  basic_gauge(std::string name, std::string help,
              std::vector<std::string> labels_name, size_t dupli_count = 2)
      : basic_counter<value_type>(std::move(name), std::move(help),
                                  std::move(labels_name), dupli_count) {
    set_metric_type(MetricType::Gauge);
  }

  basic_gauge(std::string name, std::string help,
              std::map<std::string, std::string> labels, size_t dupli_count = 2)
      : basic_counter<value_type>(std::move(name), std::move(help),
                                  std::move(labels), dupli_count) {
    set_metric_type(MetricType::Gauge);
  }

  void dec(value_type value = 1) {
#ifdef __APPLE__
    mac_os_atomic_fetch_sub(&default_label_value_.local_value(), value);
#else
    default_label_value_.dec(value);
#endif
  }

  void dec(const std::vector<std::string>& labels_value, value_type value = 1) {
    if (value == 0) {
      return;
    }

    validate(labels_value, value);
    if (use_atomic_) {
      if (labels_value != labels_value_) {
        throw std::invalid_argument(
            "the given labels_value is not match with origin labels_value");
      }
      set_value_static(atomic_value_map_[labels_value].local_value(), value,
                       op_type_t::DEC);
    }
    else {
      std::lock_guard lock(mtx_);
      stat_metric(labels_value);
      set_value_dynamic(value_map_[labels_value].local_value(), value,
                        op_type_t::DEC);
    }
  }
};
using gauge_t = basic_gauge<int64_t>;
using gauge_d = basic_gauge<double>;
}  // namespace ylt::metric