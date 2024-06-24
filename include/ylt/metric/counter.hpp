#pragma once
#include <atomic>
#include <chrono>

#include "metric.hpp"

namespace ylt::metric {
enum class op_type_t { INC, DEC, SET };

#ifdef CINATRA_ENABLE_METRIC_JSON
struct json_counter_metric_t {
  std::unordered_multimap<std::string, std::string> labels;
  int64_t value;
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

class counter_t : public metric_t {
 public:
  // default, no labels, only contains an atomic value.
  counter_t(std::string name, std::string help)
      : metric_t(MetricType::Counter, std::move(name), std::move(help)) {
    use_atomic_ = true;
    g_user_metric_count++;
  }

  // static labels value, contains a map with atomic value.
  counter_t(std::string name, std::string help,
            std::map<std::string, std::string> labels)
      : metric_t(MetricType::Counter, std::move(name), std::move(help),
                 std::move(labels)) {
    atomic_value_map_.emplace(labels_value_, 0);
    g_user_metric_count++;
    use_atomic_ = true;
  }

  // dynamic labels value
  counter_t(std::string name, std::string help,
            std::vector<std::string> labels_name)
      : metric_t(MetricType::Counter, std::move(name), std::move(help),
                 std::move(labels_name)) {
    g_user_metric_count++;
  }

  virtual ~counter_t() { g_user_metric_count--; }

  double value() { return default_lable_value_; }

  double value(const std::vector<std::string> &labels_value) {
    if (use_atomic_) {
      double val = atomic_value_map_[labels_value];
      return val;
    }
    else {
      std::lock_guard lock(mtx_);
      return value_map_[labels_value];
    }
  }

  metric_hash_map<double> value_map() override {
    metric_hash_map<double> map;
    if (use_atomic_) {
      map = {atomic_value_map_.begin(), atomic_value_map_.end()};
    }
    else {
      std::lock_guard lock(mtx_);
      map = value_map_;
    }
    return map;
  }

  void serialize(std::string &str) override {
    if (labels_name_.empty()) {
      if (default_lable_value_ == 0) {
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
      if (default_lable_value_ == 0) {
        return;
      }
      json_counter_t counter{name_, help_, std::string(metric_name())};
      int64_t value = default_lable_value_;
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
      if (v == 0) {
        continue;
      }
      json_counter_metric_t metric;
      size_t index = 0;
      for (auto &label_value : k) {
        metric.labels.emplace(labels_name_[index++], label_value);
      }
      metric.value = (int64_t)v;
      counter.metrics.push_back(std::move(metric));
    }
    if (!counter.metrics.empty()) {
      iguana::to_json(counter, str);
    }
  }
#endif

  void inc(double val = 1) {
    if (val < 0) {
      throw std::invalid_argument("the value is less than zero");
    }

#ifdef __APPLE__
    mac_os_atomic_fetch_add(&default_lable_value_, val);
#else
    default_lable_value_ += val;
#endif
  }

  void inc(const std::vector<std::string> &labels_value, double value = 1) {
    if (value == 0) {
      return;
    }

    validate(labels_value, value);
    if (use_atomic_) {
      if (labels_value != labels_value_) {
        throw std::invalid_argument(
            "the given labels_value is not match with origin labels_value");
      }
      set_value<true>(atomic_value_map_[labels_value], value, op_type_t::INC);
    }
    else {
      std::lock_guard lock(mtx_);
      stat_metric(labels_value);
      set_value<false>(value_map_[labels_value], value, op_type_t::INC);
    }
  }

  void stat_metric(const std::vector<std::string> &labels_value) {
    if (!value_map_.contains(labels_value)) {
      for (auto &key : labels_value) {
        g_user_metric_memory->inc(key.size());
      }
      g_user_metric_memory->inc(8);
      g_user_metric_labels->inc();
    }
  }

  void update(double value) { default_lable_value_ = value; }

  void update(const std::vector<std::string> &labels_value, double value) {
    if (labels_value.empty() || labels_name_.size() != labels_value.size()) {
      throw std::invalid_argument(
          "the number of labels_value name and labels_value is not match");
    }
    if (use_atomic_) {
      if (labels_value != labels_value_) {
        throw std::invalid_argument(
            "the given labels_value is not match with origin labels_value");
      }
      set_value<true>(atomic_value_map_[labels_value], value, op_type_t::SET);
    }
    else {
      std::lock_guard lock(mtx_);
      set_value<false>(value_map_[labels_value], value, op_type_t::SET);
    }
  }

  metric_hash_map<std::atomic<double>> &atomic_value_map() {
    return atomic_value_map_;
  }

 protected:
  void serialize_default_label(std::string &str) {
    str.append(name_);
    if (labels_name_.empty()) {
      str.append(" ");
    }

    if (type_ == MetricType::Counter) {
      str.append(std::to_string((int64_t)default_lable_value_));
    }
    else {
      str.append(std::to_string(default_lable_value_));
    }

    str.append("\n");
  }

  template <typename T>
  void serialize_map(T &value_map, std::string &str) {
    for (auto &[labels_value, value] : value_map) {
      if (value == 0) {
        continue;
      }
      str.append(name_);
      str.append("{");
      build_string(str, labels_name_, labels_value);
      str.append("} ");

      if (type_ == MetricType::Counter) {
        str.append(std::to_string((int64_t)value));
      }
      else {
        str.append(std::to_string(value));
      }

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

  void validate(const std::vector<std::string> &labels_value, double value) {
    if (value < 0) {
      throw std::invalid_argument("the value is less than zero");
    }
    if (labels_value.empty() || labels_name_.size() != labels_value.size()) {
      throw std::invalid_argument(
          "the number of labels_value name and labels_value is not match");
    }
  }

  template <bool is_atomic = false, typename T>
  void set_value(T &label_val, double value, op_type_t type) {
    switch (type) {
      case op_type_t::INC: {
#ifdef __APPLE__
        if constexpr (is_atomic) {
          mac_os_atomic_fetch_add(&label_val, value);
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
          mac_os_atomic_fetch_sub(&label_val, value);
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

  metric_hash_map<std::atomic<double>> atomic_value_map_;
  std::atomic<double> default_lable_value_ = 0;

  std::mutex mtx_;
  metric_hash_map<double> value_map_;
};
}  // namespace ylt::metric