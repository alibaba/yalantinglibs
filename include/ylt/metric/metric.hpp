#pragma once
#include <algorithm>
#include <atomic>
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "cinatra/cinatra_log_wrapper.hpp"

#ifdef CINATRA_ENABLE_METRIC_JSON
namespace iguana {

template <typename T>
inline char* to_chars_float(T value, char* buffer) {
  return buffer + snprintf(buffer, 65, "%g", value);
}

}  // namespace iguana

#include <iguana/json_writer.hpp>
#endif
namespace ylt::metric {
enum class MetricType {
  Counter,
  Gauge,
  Histogram,
  Summary,
  Nil,
};

struct metric_filter_options {
  std::optional<std::regex> name_regex{};
  std::optional<std::regex> label_regex{};
  bool is_white = true;
};

class metric_t {
 public:
  metric_t() = default;
  metric_t(MetricType type, std::string name, std::string help)
      : type_(type), name_(std::move(name)), help_(std::move(help)) {}
  metric_t(MetricType type, std::string name, std::string help,
           std::vector<std::string> labels_name)
      : metric_t(type, std::move(name), std::move(help)) {
    labels_name_ = std::move(labels_name);
  }

  metric_t(MetricType type, std::string name, std::string help,
           std::map<std::string, std::string> static_labels)
      : metric_t(type, std::move(name), std::move(help)) {
    static_labels_ = std::move(static_labels);
    for (auto& [k, v] : static_labels_) {
      labels_name_.push_back(k);
      labels_value_.push_back(v);
    }
  }
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

  const std::map<std::string, std::string>& get_static_labels() {
    return static_labels_;
  }

  virtual std::map<std::vector<std::string>, double,
                   std::less<std::vector<std::string>>>
  value_map() {
    return {};
  }

  virtual void serialize(std::string& str) {}

#ifdef CINATRA_ENABLE_METRIC_JSON
  virtual void serialize_to_json(std::string& str) {}
#endif

  // only for summary
  virtual async_simple::coro::Lazy<void> serialize_async(std::string& out) {
    co_return;
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  // only for summary
  virtual async_simple::coro::Lazy<void> serialize_to_json_async(
      std::string& out) {
    co_return;
  }
#endif

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

  void build_label_string(std::string& str,
                          const std::vector<std::string>& label_name,
                          const std::vector<std::string>& label_value) {
    for (size_t i = 0; i < label_name.size(); i++) {
      str.append(label_name[i])
          .append("=\"")
          .append(label_value[i])
          .append("\"")
          .append(",");
    }
    str.pop_back();
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
  std::map<std::string, std::string> static_labels_;
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
    bool r = register_metric_dynamic(m);
    if (!r) {
      return nullptr;
    }
    return m;
  }

  static bool register_metric_static(std::shared_ptr<metric_t> metric) {
    return register_metric_impl<false>(metric);
  }

  static bool register_metric_dynamic(std::shared_ptr<metric_t> metric) {
    return register_metric_impl<true>(metric);
  }

  static bool remove_metric_static(const std::string& name) {
    return remove_metric_impl<false>(name);
  }

  static bool remove_metric_dynamic(const std::string& name) {
    return remove_metric_impl<true>(name);
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

  // static labels: {{"method", "GET"}, {"url", "/"}}
  static std::shared_ptr<metric_t> get_metric_by_labels_static(
      const std::map<std::string, std::string>& labels) {
    std::shared_ptr<metric_t> t = nullptr;
    auto map = metric_map_static();
    for (auto& [name, m] : map) {
      const auto& static_labels = m->get_static_labels();
      if (static_labels == labels) {
        t = m;
        break;
      }
    }
    return t;
  }

  // static label: {"method", "GET"}
  static std::vector<std::shared_ptr<metric_t>> get_metric_by_label_static(
      const std::pair<std::string, std::string>& label) {
    std::vector<std::shared_ptr<metric_t>> vec;
    auto map = metric_map_static();
    for (auto& [name, t] : map) {
      const auto& static_labels = t->get_static_labels();
      for (const auto& pair : static_labels) {
        if (pair.first == label.first && pair.second == label.second) {
          vec.push_back(t);
        }
      }
    }
    return vec;
  }

  // labels: {{"method", "POST"}, {"code", "200"}}
  static std::vector<std::shared_ptr<metric_t>> get_metric_by_labels_dynamic(
      const std::map<std::string, std::string>& labels) {
    std::vector<std::shared_ptr<metric_t>> vec;
    auto map = metric_map_dynamic();
    for (auto& [name, t] : map) {
      auto val_map = t->value_map();
      auto labels_name = t->labels_name();

      for (auto& [k, v] : labels) {
        if (auto it = std::find(labels_name.begin(), labels_name.end(), k);
            it != labels_name.end()) {
          if (auto it = std::find_if(val_map.begin(), val_map.end(),
                                     [label_val = v](auto& pair) {
                                       auto& key = pair.first;
                                       return std::find(key.begin(), key.end(),
                                                        label_val) != key.end();
                                     });
              it != val_map.end()) {
            vec.push_back(t);
          }
        }
      }
    }

    return vec;
  }

  template <typename T>
  static std::shared_ptr<T> get_metric_static(const std::string& name) {
    auto m = get_metric_impl<false>(name);
    if (m == nullptr) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<T>(m);
  }

  template <typename T>
  static std::shared_ptr<T> get_metric_dynamic(const std::string& name) {
    auto m = get_metric_impl<true>(name);
    if (m == nullptr) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<T>(m);
  }

  static std::string serialize(
      const std::vector<std::shared_ptr<metric_t>>& metrics) {
    std::string str;
    for (auto& m : metrics) {
      if (m->metric_type() == MetricType::Summary) {
        async_simple::coro::syncAwait(m->serialize_async(str));
      }
      else {
        m->serialize(str);
      }
    }

    return str;
  }

  static std::string serialize_static() { return serialize(collect<false>()); }

  static std::string serialize_dynamic() { return serialize(collect<true>()); }

#ifdef CINATRA_ENABLE_METRIC_JSON
  static std::string serialize_to_json_static() {
    auto metrics = collect<false>();
    return serialize_to_json(metrics);
  }

  static std::string serialize_to_json_dynamic() {
    auto metrics = collect<true>();
    return serialize_to_json(metrics);
  }

  static std::string serialize_to_json(
      const std::vector<std::shared_ptr<metric_t>>& metrics) {
    if (metrics.empty()) {
      return "";
    }
    std::string str;
    str.append("[");
    for (auto& m : metrics) {
      size_t start = str.size();
      if (m->metric_type() == MetricType::Summary) {
        async_simple::coro::syncAwait(m->serialize_to_json_async(str));
      }
      else {
        m->serialize_to_json(str);
      }

      if (str.size() > start)
        str.append(",");
    }
    str.back() = ']';
    return str;
  }
#endif

  static std::vector<std::shared_ptr<metric_t>> filter_metrics_static(
      const metric_filter_options& options) {
    return filter_metrics<false>(options);
  }

  static std::vector<std::shared_ptr<metric_t>> filter_metrics_dynamic(
      const metric_filter_options& options) {
    return filter_metrics<true>(options);
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
  static size_t remove_metric_impl(const std::string& name) {
    auto lock = get_lock<need_lock>();
    return metric_map_.erase(name);
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

  static void filter_by_label_name(
      std::vector<std::shared_ptr<metric_t>>& filtered_metrics,
      std::shared_ptr<metric_t> m, const metric_filter_options& options,
      std::vector<size_t>& indexs, size_t index) {
    const auto& labels_name = m->labels_name();
    for (auto& label_name : labels_name) {
      if (std::regex_match(label_name, *options.label_regex)) {
        if (options.is_white) {
          filtered_metrics.push_back(m);
        }
        else {
          indexs.push_back(index);
        }
      }
    }
  }

  template <bool need_lock>
  static std::vector<std::shared_ptr<metric_t>> filter_metrics(
      const metric_filter_options& options) {
    auto metrics = collect<need_lock>();
    if (!options.name_regex && !options.label_regex) {
      return metrics;
    }

    std::vector<std::shared_ptr<metric_t>> filtered_metrics;
    std::vector<size_t> indexs;
    size_t index = 0;
    for (auto& m : metrics) {
      if (options.name_regex && !options.label_regex) {
        if (std::regex_match(std::string(m->name()), *options.name_regex)) {
          if (options.is_white) {
            filtered_metrics.push_back(m);
          }
          else {
            indexs.push_back(index);
          }
        }
      }
      else if (options.label_regex && !options.name_regex) {
        filter_by_label_name(filtered_metrics, m, options, indexs, index);
      }
      else {
        if (std::regex_match(std::string(m->name()), *options.name_regex)) {
          filter_by_label_name(filtered_metrics, m, options, indexs, index);
        }
      }
      index++;
    }

    if (!options.is_white) {
      for (size_t i : indexs) {
        metrics.erase(std::next(metrics.begin(), i));
      }
      return metrics;
    }

    return filtered_metrics;
  }

  static inline std::mutex mtx_;
  static inline std::map<std::string, std::shared_ptr<metric_t>> metric_map_;

  static inline null_mutex_t null_mtx_;
  static inline std::atomic_bool need_lock_ = true;
  static inline std::once_flag flag_;
};

using default_metric_manager = metric_manager_t<0>;
}  // namespace ylt::metric