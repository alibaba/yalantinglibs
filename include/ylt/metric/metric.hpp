#pragma once
#include <atomic>
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "async_simple/coro/Lazy.h"
#include "cinatra/cinatra_log_wrapper.hpp"

namespace ylt {
enum class MetricType {
  Counter,
  Gauge,
  Histogram,
  Summary,
  Nil,
};

class metric_t {
 public:
  metric_t() = default;
  metric_t(MetricType type, std::string name, std::string help,
           std::vector<std::string> labels_name = {})
      : type_(type),
        name_(std::move(name)),
        help_(std::move(help)),
        labels_name_(std::move(labels_name)) {}
  virtual ~metric_t() {}

  std::string_view name() { return name_; }

  std::string_view help() { return help_; }

  MetricType metric_type() { return type_; }

  std::string_view metric_name() {
    switch (type_) {
      case MetricType::Counter:
        return "counter";
      case MetricType::Gauge:
        return "gauge";
      case MetricType::Histogram:
        return "histogram";
      case MetricType::Summary:
        return "summary";
      case MetricType::Nil:
      default:
        return "unknown";
    }
  }

  const std::vector<std::string>& labels_name() { return labels_name_; }

  virtual void serialize(std::string& str) {}

  // only for summary
  virtual async_simple::coro::Lazy<void> serialize_async(std::string& out) {
    co_return;
  }

  bool is_atomic() const { return use_atomic_; }

  template <typename T>
  T* as() {
    return dynamic_cast<T*>(this);
  }

 protected:
  void set_metric_type(MetricType type) { type_ = type; }
  void serialize_head(std::string& str) {
    str.append("# HELP ").append(name_).append(" ").append(help_).append("\n");
    str.append("# TYPE ")
        .append(name_)
        .append(" ")
        .append(metric_name())
        .append("\n");
  }

#ifdef __APPLE__
  double mac_os_atomic_fetch_add(std::atomic<double>* obj, double arg) {
    double v;
    do {
      v = obj->load();
    } while (!std::atomic_compare_exchange_weak(obj, &v, v + arg));
    return v;
  }

  double mac_os_atomic_fetch_sub(std::atomic<double>* obj, double arg) {
    double v;
    do {
      v = obj->load();
    } while (!std::atomic_compare_exchange_weak(obj, &v, v - arg));
    return v;
  }
#endif

  MetricType type_ = MetricType::Nil;
  std::string name_;
  std::string help_;
  std::vector<std::string> labels_name_;   // read only
  std::vector<std::string> labels_value_;  // read only
  bool use_atomic_ = false;
};

template <size_t ID = 0>
struct metric_manager_t {
  struct null_mutex_t {
    void lock() {}
    void unlock() {}
  };

  // create and register metric
  template <typename T, typename... Args>
  static std::shared_ptr<T> create_metric_static(const std::string& name,
                                                 const std::string& help,
                                                 Args&&... args) {
    auto m = std::make_shared<T>(name, help, std::forward<Args>(args)...);
    bool r = register_metric_static(m);
    if (!r) {
      return nullptr;
    }
    return m;
  }

  template <typename T, typename... Args>
  static std::shared_ptr<T> create_metric_dynamic(const std::string& name,
                                                  const std::string& help,
                                                  Args&&... args) {
    auto m = std::make_shared<T>(name, help, std::forward<Args>(args)...);
    bool r = register_metric_static(m);
    if (!r) {
      return nullptr;
    }
    return m;
  }

  static bool register_metric_dynamic(std::shared_ptr<metric_t> metric) {
    return register_metric_impl<true>(metric);
  }

  static bool register_metric_static(std::shared_ptr<metric_t> metric) {
    return register_metric_impl<false>(metric);
  }

  template <typename... Metrics>
  static bool register_metric_dynamic(Metrics... metrics) {
    bool r = true;
    ((void)(r && (r = register_metric_impl<true>(metrics), true)), ...);
    return r;
  }

  template <typename... Metrics>
  static bool register_metric_static(Metrics... metrics) {
    bool r = true;
    ((void)(r && (r = register_metric_impl<false>(metrics), true)), ...);
    return r;
  }

  static auto metric_map_static() { return metric_map_impl<false>(); }
  static auto metric_map_dynamic() { return metric_map_impl<true>(); }

  static size_t metric_count_static() { return metric_count_impl<false>(); }

  static size_t metric_count_dynamic() { return metric_count_impl<true>(); }

  static std::vector<std::string> metric_keys_static() {
    return metric_keys_impl<false>();
  }

  static std::vector<std::string> metric_keys_dynamic() {
    return metric_keys_impl<true>();
  }

  template <typename T>
  static T* get_metric_static(const std::string& name) {
    auto m = get_metric_impl<false>(name);
    if (m == nullptr) {
      return nullptr;
    }
    return m->template as<T>();
  }

  template <typename T>
  static T* get_metric_dynamic(const std::string& name) {
    auto m = get_metric_impl<true>(name);
    if (m == nullptr) {
      return nullptr;
    }
    return m->template as<T>();
  }

  static async_simple::coro::Lazy<std::string> serialize_static() {
    return serialize_impl<false>();
  }

  static async_simple::coro::Lazy<std::string> serialize_dynamic() {
    return serialize_impl<true>();
  }

 private:
  template <bool need_lock>
  static void check_lock() {
    if (need_lock_ != need_lock) {
      std::string str = "need lock ";
      std::string s = need_lock_ ? "true" : "false";
      std::string r = need_lock ? "true" : "false";
      str.append(s).append(" but set as ").append(r);
      throw std::invalid_argument(str);
    }
  }

  template <bool need_lock = true>
  static auto get_lock() {
    check_lock<need_lock>();
    if constexpr (need_lock) {
      return std::scoped_lock(mtx_);
    }
    else {
      return std::scoped_lock(null_mtx_);
    }
  }

  template <bool need_lock>
  static bool register_metric_impl(std::shared_ptr<metric_t> metric) {
    // the first time regiter_metric will set metric_manager_t lock or not lock.
    // visit metric_manager_t with different lock strategy will cause throw
    // exception.
    std::call_once(flag_, [] {
      need_lock_ = need_lock;
    });

    std::string name(metric->name());
    auto lock = get_lock<need_lock>();
    bool r = metric_map_.emplace(name, std::move(metric)).second;
    if (!r) {
      CINATRA_LOG_ERROR << "duplicate registered metric name: " << name;
    }
    return r;
  }

  template <bool need_lock>
  static auto metric_map_impl() {
    auto lock = get_lock<need_lock>();
    return metric_map_;
  }

  template <bool need_lock>
  static size_t metric_count_impl() {
    auto lock = get_lock<need_lock>();
    return metric_map_.size();
  }

  template <bool need_lock>
  static std::vector<std::string> metric_keys_impl() {
    std::vector<std::string> keys;
    {
      auto lock = get_lock<need_lock>();
      for (auto& pair : metric_map_) {
        keys.push_back(pair.first);
      }
    }

    return keys;
  }

  template <bool need_lock>
  static std::shared_ptr<metric_t> get_metric_impl(const std::string& name) {
    auto lock = get_lock<need_lock>();
    auto it = metric_map_.find(name);
    if (it == metric_map_.end()) {
      return nullptr;
    }
    return it->second;
  }

  template <bool need_lock>
  static auto collect() {
    std::vector<std::shared_ptr<metric_t>> metrics;
    {
      auto lock = get_lock<need_lock>();
      for (auto& pair : metric_map_) {
        metrics.push_back(pair.second);
      }
    }
    return metrics;
  }

  template <bool need_lock = true>
  static async_simple::coro::Lazy<std::string> serialize_impl() {
    std::string str;
    auto metrics = collect<need_lock>();
    for (auto& m : metrics) {
      if (m->metric_type() == MetricType::Summary) {
        co_await m->serialize_async(str);
      }
      else {
        m->serialize(str);
      }
    }
    co_return str;
  }

  static inline std::mutex mtx_;
  static inline std::map<std::string, std::shared_ptr<metric_t>> metric_map_;

  static inline null_mutex_t null_mtx_;
  static inline std::atomic_bool need_lock_ = true;
  static inline std::once_flag flag_;
};

using default_metric_manager = metric_manager_t<0>;
}  // namespace ylt