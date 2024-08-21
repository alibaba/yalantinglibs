#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <variant>

#include "metric.hpp"
#include "thread_local_value.hpp"

namespace ylt::metric {
enum class op_type_t { INC, DEC, SET };

#ifdef CINATRA_ENABLE_METRIC_JSON
struct json_counter_metric_t {
  std::map<std::string, std::string> labels;
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

template <typename T, typename value_type>
inline void set_value(T &label_val, value_type value, op_type_t type) {
  switch (type) {
    case op_type_t::INC: {
#ifdef __APPLE__
      if constexpr (std::is_floating_point_v<value_type>) {
        mac_os_atomic_fetch_add(&label_val, value);
      }
      else {
        label_val += value;
      }
#else
      label_val += value;
#endif
    } break;
    case op_type_t::DEC:
#ifdef __APPLE__
      if constexpr (std::is_floating_point_v<value_type>) {
        mac_os_atomic_fetch_sub(&label_val, value);
      }
      else {
        label_val -= value;
      }
#else
      label_val -= value;
#endif
      break;
    case op_type_t::SET:
      label_val = value;
      break;
  }
}

template <typename value_type>
class basic_static_counter : public static_metric {
 public:
  // static counter, no labels, only contains an atomic value.
  basic_static_counter(std::string name, std::string help,
                       size_t dupli_count = 2)
      : static_metric(MetricType::Counter, std::move(name), std::move(help)) {
    init_thread_local(dupli_count);
  }

  // static counter, contains a static labels with atomic value.
  basic_static_counter(std::string name, std::string help,
                       std::map<std::string, std::string> labels,
                       uint32_t dupli_count = 2)
      : static_metric(MetricType::Counter, std::move(name), std::move(help),
                      std::move(labels)) {
    init_thread_local(dupli_count);
  }

  void init_thread_local(uint32_t dupli_count) {
    if (dupli_count > 0) {
      dupli_count_ = dupli_count;
      default_label_value_ = {dupli_count};
    }

    g_user_metric_count++;
  }

  virtual ~basic_static_counter() { g_user_metric_count--; }

  void inc(value_type val = 1) {
    if (val <= 0) {
      return;
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

  value_type update(value_type value) {
    if (!has_change_) [[unlikely]] {
      has_change_ = true;
    }
    return default_label_value_.update(value);
  }

  value_type reset() { return default_label_value_.reset(); }

  value_type value() { return default_label_value_.value(); }

  void serialize(std::string &str) override {
    auto value = default_label_value_.value();
    if (value == 0 && !has_change_) {
      return;
    }

    serialize_head(str);
    serialize_default_label(str, value);
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  void serialize_to_json(std::string &str) override {
    if (default_label_value_.value() == 0) {
      return;
    }

    json_counter_t counter{name_, help_, std::string(metric_name())};
    auto value = default_label_value_.value();
    counter.metrics.push_back({static_labels_, value});
    iguana::to_json(counter, str);
  }
#endif

 protected:
  void serialize_default_label(std::string &str, value_type value) {
    str.append(name_);
    if (labels_name_.empty()) {
      str.append(" ");
    }
    else {
      str.append("{");
      build_string(str, labels_name_, labels_value_);
      str.append("} ");
    }

    str.append(std::to_string(value));

    str.append("\n");
  }

  void build_string(std::string &str, const std::vector<std::string> &v1,
                    const std::vector<std::string> &v2) {
    for (size_t i = 0; i < v1.size(); i++) {
      str.append(v1[i]).append("=\"").append(v2[i]).append("\"").append(",");
    }
    str.pop_back();
  }

  thread_local_value<value_type> default_label_value_;
  uint32_t dupli_count_ = 2;
  bool has_change_ = false;
};

template <typename Key>
struct array_hash {
  size_t operator()(const Key &arr) const {
    unsigned int seed = 131;
    unsigned int hash = 0;

    for (const auto &str : arr) {
      for (auto ch : str) {
        hash = hash * seed + ch;
      }
    }

    return (hash & 0x7FFFFFFF);
  }
};

using counter_t = basic_static_counter<int64_t>;
using counter_d = basic_static_counter<double>;

template <typename Key, typename T>
using dynamic_metric_hash_map = std::unordered_map<Key, T, array_hash<Key>>;

template <typename value_type, uint8_t N>
class basic_dynamic_counter : public dynamic_metric {
 public:
  // dynamic labels value
  basic_dynamic_counter(std::string name, std::string help,
                        std::array<std::string, N> labels_name,
                        size_t dupli_count = 2)
      : dynamic_metric(MetricType::Counter, std::move(name), std::move(help),
                       std::move(labels_name)),
        dupli_count_(dupli_count) {
    g_user_metric_count++;
  }

  virtual ~basic_dynamic_counter() { g_user_metric_count--; }

  void inc(const std::array<std::string, N> &labels_value,
           value_type value = 1) {
    if (value == 0) {
      return;
    }

    std::unique_lock lock(mtx_);
    if (value_map_.size() > ylt_label_capacity) {
      return;
    }
    auto [it, r] = value_map_.try_emplace(
        labels_value, thread_local_value<value_type>(dupli_count_));
    lock.unlock();
    if (r) {
      g_user_metric_label_count->local_value()++;
      if (ylt_label_max_age.count()) {
        it->second.set_created_time(std::chrono::system_clock::now());
      }
    }
    set_value(it->second.local_value(), value, op_type_t::INC);
  }

  value_type update(const std::array<std::string, N> &labels_value,
                    value_type value) {
    std::unique_lock lock(mtx_);
    if (value_map_.size() > ylt_label_capacity) {
      return value_type{};
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
    return it->second.update(value);
  }

  value_type value(const std::array<std::string, N> &labels_value) {
    std::lock_guard lock(mtx_);
    if (auto it = value_map_.find(labels_value); it != value_map_.end()) {
      return it->second.value();
    }

    return value_type{};
  }

  value_type reset() {
    value_type val = {};

    std::lock_guard lock(mtx_);
    for (auto &[key, t] : value_map_) {
      val += t.reset();
    }

    return val;
  }

  dynamic_metric_hash_map<std::array<std::string, N>,
                          thread_local_value<value_type>>
  value_map() {
    [[maybe_unused]] bool has_change = false;
    return value_map(has_change);
  }

  dynamic_metric_hash_map<std::array<std::string, N>,
                          thread_local_value<value_type>>
  value_map(bool &has_change) {
    dynamic_metric_hash_map<std::array<std::string, N>,
                            thread_local_value<value_type>>
        map;
    {
      std::lock_guard lock(mtx_);
      map = value_map_;
      has_change = has_change_;
    }

    return map;
  }

  size_t label_value_count() override {
    std::lock_guard lock(mtx_);
    return value_map_.size();
  }

  void clean_expired_label() override {
    if (ylt_label_max_age.count() == 0) {
      return;
    }

    auto now = std::chrono::system_clock::now();
    std::lock_guard lock(mtx_);
    std::erase_if(value_map_, [&now](auto &pair) mutable {
      bool r = std::chrono::duration_cast<std::chrono::seconds>(
                   now - pair.second.get_created_time())
                   .count() >= ylt_label_max_age.count();
      return r;
    });
  }

  void remove_label_value(
      const std::map<std::string, std::string> &labels) override {
    {
      std::lock_guard lock(mtx_);
      if (value_map_.empty()) {
        return;
      }
    }

    const auto &labels_name = this->labels_name();
    if (labels.size() > labels_name.size()) {
      return;
    }

    if (labels.size() == labels_name.size()) {
      std::vector<std::string> label_value;
      for (auto &lb_name : labels_name) {
        if (auto i = labels.find(lb_name); i != labels.end()) {
          label_value.push_back(i->second);
        }
      }

      std::lock_guard lock(mtx_);
      std::erase_if(value_map_, [&, this](auto &pair) {
        return equal(label_value, pair.first);
      });
      return;
    }
    else {
      std::vector<std::string> vec;
      for (auto &lb_name : labels_name) {
        if (auto i = labels.find(lb_name); i != labels.end()) {
          vec.push_back(i->second);
        }
        else {
          vec.push_back("");
        }
      }
      if (vec.empty()) {
        return;
      }

      std::lock_guard lock(mtx_);
      std::erase_if(value_map_, [&](auto &pair) {
        auto &[arr, _] = pair;
        for (size_t i = 0; i < vec.size(); i++) {
          if (!vec[i].empty() && vec[i] != arr[i]) {
            return false;
          }
        }
        return true;
      });
    }
  }

  bool has_label_value(const std::string &value) override {
    [[maybe_unused]] bool has_change = false;
    auto map = value_map(has_change);
    for (auto &[label_value, _] : map) {
      if (auto it = std::find(label_value.begin(), label_value.end(), value);
          it != label_value.end()) {
        return true;
      }
    }

    return false;
  }

  bool has_label_value(const std::regex &regex) override {
    [[maybe_unused]] bool has_change = false;
    auto map = value_map(has_change);
    for (auto &[label_value, _] : map) {
      if (auto it = std::find_if(label_value.begin(), label_value.end(),
                                 [&](auto &val) {
                                   return std::regex_match(val, regex);
                                 });
          it != label_value.end()) {
        return true;
      }
    }

    return false;
  }

  bool has_label_value(const std::vector<std::string> &label_value) override {
    std::array<std::string, N> arr{};
    size_t size = (std::min)((size_t)N, label_value.size());
    for (size_t i = 0; i < size; i++) {
      arr[i] = label_value[i];
    }
    std::lock_guard lock(mtx_);
    return value_map_.contains(arr);
  }

  void serialize(std::string &str) override {
    bool has_change = false;
    auto map = value_map(has_change);
    if (map.empty()) {
      return;
    }

    std::string value_str;
    serialize_map(map, value_str, has_change);
    if (!value_str.empty()) {
      serialize_head(str);
      str.append(value_str);
    }
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  void serialize_to_json(std::string &str) override {
    std::string s;
    bool has_change = false;
    auto map = value_map(has_change);
    json_counter_t counter{name_, help_, std::string(metric_name())};
    to_json(counter, map, str, has_change);
  }

  template <typename T>
  void to_json(json_counter_t &counter, T &map, std::string &str,
               bool has_change) {
    for (auto &[k, v] : map) {
      auto val = v.value();
      if (val == 0 && !has_change) {
        continue;
      }
      json_counter_metric_t metric;
      size_t index = 0;
      for (auto &label_value : k) {
        metric.labels.emplace(labels_name_[index++], label_value);
      }
      metric.value = (int64_t)val;
      counter.metrics.push_back(std::move(metric));
    }
    if (!counter.metrics.empty()) {
      iguana::to_json(counter, str);
    }
  }
#endif

 protected:
  template <typename T>
  void serialize_map(T &value_map, std::string &str, bool has_change) {
    for (auto &[labels_value, value] : value_map) {
      auto val = value.value();
      if (val == 0 && !has_change) {
        continue;
      }
      str.append(name_);
      if (labels_name_.empty()) {
        str.append(" ");
      }
      else {
        str.append("{");
        build_string(str, labels_name_, labels_value);
        str.append("} ");
      }

      str.append(std::to_string(val));

      str.append("\n");
    }
  }

  template <class T, std::size_t Size>
  bool equal(const std::vector<T> &v, const std::array<T, Size> &a) {
    if (v.size() != N)
      return false;

    return std::equal(v.begin(), v.end(), a.begin());
  }

  void build_string(std::string &str, const std::vector<std::string> &v1,
                    const auto &v2) {
    for (size_t i = 0; i < v1.size(); i++) {
      str.append(v1[i]).append("=\"").append(v2[i]).append("\"").append(",");
    }
    str.pop_back();
  }

  std::mutex mtx_;
  dynamic_metric_hash_map<std::array<std::string, N>,
                          thread_local_value<value_type>>
      value_map_;
  size_t dupli_count_ = 2;
  bool has_change_ = false;
};

using dynamic_counter_1t = basic_dynamic_counter<int64_t, 1>;
using dynamic_counter_1d = basic_dynamic_counter<double, 1>;

using dynamic_counter_2t = basic_dynamic_counter<int64_t, 2>;
using dynamic_counter_2d = basic_dynamic_counter<double, 2>;
using dynamic_counter_t = dynamic_counter_2t;
using dynamic_counter_d = dynamic_counter_2d;

using dynamic_counter_3t = basic_dynamic_counter<int64_t, 3>;
using dynamic_counter_3d = basic_dynamic_counter<double, 3>;

using dynamic_counter_4t = basic_dynamic_counter<int64_t, 4>;
using dynamic_counter_4d = basic_dynamic_counter<double, 4>;

using dynamic_counter_5t = basic_dynamic_counter<int64_t, 5>;
using dynamic_counter_5d = basic_dynamic_counter<double, 5>;
}  // namespace ylt::metric