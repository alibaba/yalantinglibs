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
#include <unordered_map>
#include <vector>

#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "cinatra/cinatra_log_wrapper.hpp"
#include "thread_local_value.hpp"
#if __has_include("ylt/coro_io/coro_io.hpp")
#include "ylt/coro_io/coro_io.hpp"
#else
#include "cinatra/ylt/coro_io/coro_io.hpp"
#endif

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
  std::optional<std::regex> label_value_regex{};
  bool is_white = true;
};

#ifdef __APPLE__
inline double mac_os_atomic_fetch_add(std::atomic<double>* obj, double arg) {
  double v;
  do {
    v = obj->load();
  } while (!std::atomic_compare_exchange_weak(obj, &v, v + arg));
  return v;
}

inline double mac_os_atomic_fetch_sub(std::atomic<double>* obj, double arg) {
  double v;
  do {
    v = obj->load();
  } while (!std::atomic_compare_exchange_weak(obj, &v, v - arg));
  return v;
}
#endif

class metric_t {
 public:
  metric_t() = default;
  metric_t(MetricType type, std::string name, std::string help)
      : type_(type),
        name_(std::move(name)),
        help_(std::move(help)),
        metric_created_time_(std::chrono::system_clock::now()) {}

  template <size_t N>
  metric_t(MetricType type, std::string name, std::string help,
           std::array<std::string, N> labels_name)
      : metric_t(type, std::move(name), std::move(help)) {
    for (size_t i = 0; i < N; i++) {
      labels_name_.push_back(std::move(labels_name[i]));
    }
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

  const std::string& str_name() { return name_; }

  std::string_view help() { return help_; }

  MetricType metric_type() { return type_; }

  auto get_created_time() { return metric_created_time_; }

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

  virtual size_t label_value_count() { return 0; }

  virtual bool has_label_value(const std::string& label_value) {
    return std::find(labels_value_.begin(), labels_value_.end(), label_value) !=
           labels_value_.end();
  }

  virtual void clean_expired_label() {}

  virtual bool has_label_value(const std::vector<std::string>& label_value) {
    return labels_value_ == label_value;
  }
  virtual bool has_label_value(const std::regex& regex) {
    auto it = std::find_if(labels_value_.begin(), labels_value_.end(),
                           [&](auto& value) {
                             return std::regex_match(value, regex);
                           });

    return it != labels_value_.end();
  }
  bool has_label_name(const std::vector<std::string>& label_name) {
    return labels_name_ == label_name;
  }
  bool has_label_name(const std::string& label_name) {
    return std::find(labels_name_.begin(), labels_name_.end(), label_name) !=
           labels_name_.end();
  }

  virtual void remove_label_value(
      const std::map<std::string, std::string>& labels) {}

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
                          const auto& label_value) {
    for (size_t i = 0; i < label_name.size(); i++) {
      str.append(label_name[i])
          .append("=\"")
          .append(label_value[i])
          .append("\"")
          .append(",");
    }
    str.pop_back();
  }

  MetricType type_ = MetricType::Nil;
  std::string name_;
  std::string help_;
  std::map<std::string, std::string> static_labels_;
  std::vector<std::string> labels_name_;   // read only
  std::vector<std::string> labels_value_;  // read only
  std::chrono::system_clock::time_point metric_created_time_{};
};

class static_metric : public metric_t {
  using metric_t::metric_t;
};

class dynamic_metric : public metric_t {
  using metric_t::metric_t;
};

inline auto g_user_metric_label_count = new thread_local_value<int64_t>(2);
inline std::atomic<int64_t> g_summary_failed_count = 0;
inline std::atomic<int64_t> g_user_metric_count = 0;

inline std::atomic<int64_t> ylt_metric_capacity = 10000000;
inline int64_t ylt_label_capacity = 20000000;

inline std::chrono::seconds ylt_label_max_age{0};
inline std::chrono::seconds ylt_label_check_expire_duration{0};

inline void set_metric_capacity(int64_t max_count) {
  ylt_metric_capacity = max_count;
}

inline void set_label_capacity(int64_t max_label_count) {
  ylt_label_capacity = max_label_count;
}

inline void set_label_max_age(
    std::chrono::seconds max_age,
    std::chrono::seconds check_duration = std::chrono::seconds(60 * 10)) {
  ylt_label_max_age = max_age;
  ylt_label_check_expire_duration = check_duration;
}
}  // namespace ylt::metric