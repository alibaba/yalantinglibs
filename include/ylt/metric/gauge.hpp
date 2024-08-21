#pragma once
#include <chrono>

#include "counter.hpp"

namespace ylt::metric {

template <typename value_type>
class basic_static_gauge : public basic_static_counter<value_type> {
  using metric_t::set_metric_type;
  using basic_static_counter<value_type>::default_label_value_;
  using metric_t::labels_value_;
  using basic_static_counter<value_type>::dupli_count_;
  using basic_static_counter<value_type>::has_change_;

 public:
  basic_static_gauge(std::string name, std::string help, size_t dupli_count = 2)
      : basic_static_counter<value_type>(std::move(name), std::move(help),
                                         dupli_count) {
    set_metric_type(MetricType::Gauge);
  }

  basic_static_gauge(std::string name, std::string help,
                     std::map<std::string, std::string> labels,
                     size_t dupli_count = 2)
      : basic_static_counter<value_type>(std::move(name), std::move(help),
                                         std::move(labels), dupli_count) {
    set_metric_type(MetricType::Gauge);
  }

  void dec(value_type value = 1) {
    if (!has_change_) [[unlikely]] {
      has_change_ = true;
    }
#ifdef __APPLE__
    if constexpr (std::is_floating_point_v<value_type>) {
      mac_os_atomic_fetch_sub(&default_label_value_.local_value(), value);
    }
    else {
      default_label_value_.dec(value);
    }
#else
    default_label_value_.dec(value);
#endif
  }
};
using gauge_t = basic_static_gauge<int64_t>;
using gauge_d = basic_static_gauge<double>;

template <typename value_type, uint8_t N>
class basic_dynamic_gauge : public basic_dynamic_counter<value_type, N> {
  using metric_t::set_metric_type;
  using basic_dynamic_counter<value_type, N>::value_map_;
  using basic_dynamic_counter<value_type, N>::mtx_;
  using basic_dynamic_counter<value_type, N>::dupli_count_;
  using basic_dynamic_counter<value_type, N>::has_change_;

 public:
  basic_dynamic_gauge(std::string name, std::string help,
                      std::array<std::string, N> labels_name,
                      size_t dupli_count = 2)
      : basic_dynamic_counter<value_type, N>(std::move(name), std::move(help),
                                             std::move(labels_name),
                                             dupli_count) {
    set_metric_type(MetricType::Gauge);
  }

  void dec(const std::array<std::string, N>& labels_value,
           value_type value = 1) {
    if (value == 0) {
      return;
    }

    std::unique_lock lock(mtx_);
    if (value_map_.size() > ylt_label_capacity) {
      return;
    }
    if (!has_change_) [[unlikely]]
      has_change_ = true;
    auto [it, r] = value_map_.try_emplace(
        labels_value, thread_local_value<value_type>(dupli_count_));
    lock.unlock();
    if (r) {
      g_user_metric_label_count->local_value()++;
      if (ylt_label_max_age.count()) {
        it->second.set_created_time(std::chrono::system_clock::now());
      }
    }

    set_value(it->second.local_value(), value, op_type_t::DEC);
  }
};

using dynamic_gauge_1t = basic_dynamic_gauge<int64_t, 1>;
using dynamic_gauge_1d = basic_dynamic_gauge<double, 1>;

using dynamic_gauge_t = basic_dynamic_gauge<int64_t, 2>;
using dynamic_gauge_d = basic_dynamic_gauge<double, 2>;
using dynamic_gauge_2t = dynamic_gauge_t;
using dynamic_gauge_2d = dynamic_gauge_d;

using dynamic_gauge_3t = basic_dynamic_gauge<int64_t, 3>;
using dynamic_gauge_3d = basic_dynamic_gauge<double, 3>;

using dynamic_gauge_4t = basic_dynamic_gauge<int64_t, 4>;
using dynamic_gauge_4d = basic_dynamic_gauge<double, 4>;

using dynamic_gauge_5t = basic_dynamic_gauge<int64_t, 5>;
using dynamic_gauge_5d = basic_dynamic_gauge<double, 5>;
}  // namespace ylt::metric