#pragma once
#include <atomic>
#include <chrono>
#include <variant>

#include "metric.hpp"
#include "thread_local_value.hpp"

namespace ylt::metric {
enum class op_type_t { INC, DEC, SET };

#ifdef CINATRA_ENABLE_METRIC_JSON
struct json_counter_metric_t {
  std::unordered_multimap<std::string, std::string> labels;
  std::variant<int64_t, double> value;
};
REFLECTION(json_counter_metric_t, labels, value);
struct json_counter_t {
  std::string name;
  std::string help;
  std::string type;
  std::vector<json_counter_metric_t> metrics;
};
REFLECTION(json_counter_t, name, help, type, metrics);
#endif

template <typename value_type>
class basic_counter : public metric_t {
 public:
  // default, no labels, only contains an atomic value.
  basic_counter(std::string name, std::string help, size_t dupli_count = 2)
      : metric_t(MetricType::Counter, std::move(name), std::move(help)),
        dupli_count_(dupli_count) {
    use_atomic_ = true;
    g_user_metric_count++;
  }

  // static labels value, contains a map with atomic value.
  basic_counter(std::string name, std::string help,
                std::map<std::string, std::string> labels,
                size_t dupli_count = 2)
      : metric_t(MetricType::Counter, std::move(name), std::move(help),
                 std::move(labels)),
        dupli_count_(dupli_count) {
    atomic_value_map_.emplace(labels_value_,
                              thread_local_value<value_type>(dupli_count));
    g_user_metric_count++;
    use_atomic_ = true;
  }

  // dynamic labels value
  basic_counter(std::string name, std::string help,
                std::vector<std::string> labels_name, size_t dupli_count = 2)
      : metric_t(MetricType::Counter, std::move(name), std::move(help),
                 std::move(labels_name)),
        dupli_count_(dupli_count) {
    g_user_metric_count++;
  }

  virtual ~basic_counter() { g_user_metric_count--; }

  value_type value() { return default_label_value_.value(); }

  value_type value(const std::vector<std::string> &labels_value) {
    if (use_atomic_) {
      value_type val = atomic_value_map_[labels_value].value();
      return val;
    }
    else {
      std::lock_guard lock(mtx_);
      auto [it, r] = value_map_.try_emplace(
          labels_value, thread_local_value<value_type>(dupli_count_));
      return it->second.value();
    }
  }

  metric_hash_map<thread_local_value<value_type>> value_map() {
    metric_hash_map<thread_local_value<value_type>> map;
    if (use_atomic_) {
      map = {atomic_value_map_.begin(), atomic_value_map_.end()};
    }
    else {
      std::lock_guard lock(mtx_);
      map = value_map_;
    }
    return map;
  }

  bool has_label_value(const std::string &label_val) override {
    auto map = value_map();
    auto it = std::find_if(map.begin(), map.end(), [&label_val](auto &pair) {
      auto &key = pair.first;
      return std::find(key.begin(), key.end(), label_val) != key.end();
    });
    return it != map.end();
  }

  void serialize(std::string &str) override {
    if (labels_name_.empty()) {
      if (default_label_value_.value() == 0) {
        return;
      }
      serialize_head(str);
      serialize_default_label(str);
      return;
    }

    auto map = value_map();
    if (map.empty()) {
      return;
    }

    std::string value_str;
    serialize_map(map, value_str);
    if (!value_str.empty()) {
      serialize_head(str);
      str.append(value_str);
    }
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  void serialize_to_json(std::string &str) override {
    std::string s;
    if (labels_name_.empty()) {
      if (default_label_value_.value() == 0) {
        return;
      }
      json_counter_t counter{name_, help_, std::string(metric_name())};
      auto value = default_label_value_.value();
      counter.metrics.push_back({{}, value});
      iguana::to_json(counter, str);
      return;
    }

    auto map = value_map();
    json_counter_t counter{name_, help_, std::string(metric_name())};
    to_json(counter, map, str);
  }

  template <typename T>
  void to_json(json_counter_t &counter, T &map, std::string &str) {
    for (auto &[k, v] : map) {
      if (v.value() == 0) {
        continue;
      }
      json_counter_metric_t metric;
      size_t index = 0;
      for (auto &label_value : k) {
        metric.labels.emplace(labels_name_[index++], label_value);
      }
      metric.value = (int64_t)v.value();
      counter.metrics.push_back(std::move(metric));
    }
    if (!counter.metrics.empty()) {
      iguana::to_json(counter, str);
    }
  }
#endif

  void inc(value_type val = 1) {
    if (val < 0) {
      throw std::invalid_argument("the value is less than zero");
    }

#ifdef __APPLE__
    if constexpr (std::is_floating_point_v<value_type>) {
      mac_os_atomic_fetch_add(&default_label_value_.local_value(), val);
    }
    else {
      default_label_value_.inc(val);
    }
#else
    default_label_value_.inc(val);
#endif
  }

  void inc(const std::vector<std::string> &labels_value, value_type value = 1) {
    if (value == 0) {
      return;
    }

    validate(labels_value, value);
    if (use_atomic_) {
      if (labels_value != labels_value_) {
        throw std::invalid_argument(
            "the given labels_value is not match with origin labels_value");
      }
      set_value<true>(atomic_value_map_[labels_value].local_value(), value,
                      op_type_t::INC);
    }
    else {
      std::lock_guard lock(mtx_);
      stat_metric(labels_value);
      set_value<false>(value_map_[labels_value].local_value(), value,
                       op_type_t::INC);
    }
  }

  void stat_metric(const std::vector<std::string> &labels_value) {
    if (!value_map_.contains(labels_value)) {
      g_user_metric_label_count++;
    }
  }

  value_type update(value_type value) {
    return default_label_value_.update(value);
  }

  value_type reset() {
    value_type val = {};
    if (use_atomic_) {
      if (labels_name_.empty()) {
        val = default_label_value_.reset();
        return val;
      }

      for (auto &[key, t] : atomic_value_map_) {
        val += t.reset();
      }
    }
    else {
      std::lock_guard lock(mtx_);
      for (auto &[key, t] : value_map_) {
        val += t.reset();
      }
    }
    return val;
  }

  void update(const std::vector<std::string> &labels_value, value_type value) {
    if (labels_value.empty() || labels_name_.size() != labels_value.size()) {
      throw std::invalid_argument(
          "the number of labels_value name and labels_value is not match");
    }
    if (use_atomic_) {
      if (labels_value != labels_value_) {
        throw std::invalid_argument(
            "the given labels_value is not match with origin labels_value");
      }
      atomic_value_map_[labels_value].update(value);
    }
    else {
      std::lock_guard lock(mtx_);
      value_map_[labels_value].update(value);
    }
  }

  auto &atomic_value_map() { return atomic_value_map_; }

 protected:
  void serialize_default_label(std::string &str) {
    str.append(name_);
    if (labels_name_.empty()) {
      str.append(" ");
    }

    str.append(std::to_string(default_label_value_.value()));

    str.append("\n");
  }

  template <typename T>
  void serialize_map(T &value_map, std::string &str) {
    for (auto &[labels_value, value] : value_map) {
      if (value.value() == 0) {
        continue;
      }
      str.append(name_);
      str.append("{");
      build_string(str, labels_name_, labels_value);
      str.append("} ");

      str.append(std::to_string(value.value()));

      str.append("\n");
    }
  }

  void build_string(std::string &str, const std::vector<std::string> &v1,
                    const std::vector<std::string> &v2) {
    for (size_t i = 0; i < v1.size(); i++) {
      str.append(v1[i]).append("=\"").append(v2[i]).append("\"").append(",");
    }
    str.pop_back();
  }

  void validate(const std::vector<std::string> &labels_value,
                value_type value) {
    if (value < 0) {
      throw std::invalid_argument("the value is less than zero");
    }
    if (labels_value.empty() || labels_name_.size() != labels_value.size()) {
      throw std::invalid_argument(
          "the number of labels_value name and labels_value is not match");
    }
  }

  template <typename T>
  void set_value_static(T &label_val, value_type value, op_type_t type) {
    set_value<true>(label_val, value, type);
  }

  template <typename T>
  void set_value_dynamic(T &label_val, value_type value, op_type_t type) {
    set_value<false>(label_val, value, type);
  }

  template <bool is_atomic = false, typename T>
  void set_value(T &label_val, value_type value, op_type_t type) {
    switch (type) {
      case op_type_t::INC: {
#ifdef __APPLE__
        if constexpr (is_atomic) {
          if constexpr (std::is_floating_point_v<value_type>) {
            mac_os_atomic_fetch_add(&label_val.local_value(), value);
          }
          else {
            label_val += value;
          }
        }
        else {
          label_val += value;
        }
#else
        if constexpr (is_atomic) {
          label_val.fetch_add(value, std::memory_order_relaxed);
        }
        else {
          label_val += value;
        }
#endif
      } break;
      case op_type_t::DEC:
#ifdef __APPLE__
        if constexpr (is_atomic) {
          if constexpr (std::is_floating_point_v<value_type>) {
            mac_os_atomic_fetch_sub(&label_val, value);
          }
          else {
            label_val -= value;
          }
        }
        else {
          label_val -= value;
        }
#else
        if constexpr (is_atomic) {
          label_val.fetch_sub(value, std::memory_order_relaxed);
        }
        else {
          label_val -= value;
        }
#endif
        break;
      case op_type_t::SET:
        label_val = value;
        break;
    }
  }

  metric_hash_map<thread_local_value<value_type>> atomic_value_map_;
  thread_local_value<value_type> default_label_value_ = {10};

  std::mutex mtx_;
  metric_hash_map<thread_local_value<value_type>> value_map_;
  size_t dupli_count_ = 1;
};
using counter_t = basic_counter<int64_t>;
using counter_d = basic_counter<double>;
}  // namespace ylt::metric