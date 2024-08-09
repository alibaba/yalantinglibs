#pragma once
#include <shared_mutex>
#include <system_error>
#include <utility>

#include "metric.hpp"

namespace ylt::metric {
class manager_helper {
 public:
  static bool register_metric(auto& metric_map, auto metric) {
    if (g_user_metric_count > ylt_metric_capacity) {
      CINATRA_LOG_ERROR << "metric count at capacity size: "
                        << g_user_metric_count;
      return false;
    }
    auto [it, r] = metric_map.try_emplace(metric->str_name(), metric);
    if (!r) {
      CINATRA_LOG_ERROR << "duplicate registered metric name: "
                        << metric->str_name();
      return false;
    }

    return true;
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

#ifdef CINATRA_ENABLE_METRIC_JSON
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

    if (str.size() == 1) {
      return "";
    }

    str.back() = ']';
    return str;
  }
#endif

  static std::vector<std::shared_ptr<metric_t>> filter_metrics_by_label_value(
      auto& metrics, const std::regex& label_regex, bool is_white) {
    std::vector<std::shared_ptr<metric_t>> filtered_metrics;
    std::vector<size_t> indexs;
    size_t index = 0;
    for (auto& m : metrics) {
      if (m->has_label_value(label_regex)) {
        if (is_white) {
          filtered_metrics.push_back(m);
        }
        else {
          indexs.push_back(index);
        }
      }

      index++;
    }

    if (!is_white) {
      for (size_t i : indexs) {
        metrics.erase(std::next(metrics.begin(), i));
      }
      return metrics;
    }

    return filtered_metrics;
  }

  static std::vector<std::shared_ptr<metric_t>> filter_metrics(
      auto& metrics, const metric_filter_options& options) {
    if (!options.name_regex && !options.label_regex) {
      return metrics;
    }

    std::vector<std::shared_ptr<metric_t>> filtered_metrics;
    for (auto& m : metrics) {
      if (options.name_regex &&
          !options.label_regex) {  // only check metric name
        if (std::regex_match(std::string(m->name()), *options.name_regex)) {
          filtered_metrics.push_back(m);
        }
      }
      else if (options.label_regex &&
               !options.name_regex) {  // only check label name
        filter_by_label_name(filtered_metrics, m, options);
      }
      else {
        if (std::regex_match(
                std::string(m->name()),
                *options.name_regex)) {  // check label name and label value
          filter_by_label_name(filtered_metrics, m, options);
        }
      }
    }

    if (!options.is_white) {
      auto it = metrics.begin();
      for (auto& m : filtered_metrics) {
        std::erase_if(metrics, [&](auto t) {
          return t == m;
        });
      }
      return metrics;
    }

    return filtered_metrics;
  }

  static void filter_by_label_name(
      std::vector<std::shared_ptr<metric_t>>& filtered_metrics,
      std::shared_ptr<metric_t> m, const metric_filter_options& options) {
    const auto& labels_name = m->labels_name();
    for (auto& label_name : labels_name) {
      if (std::regex_match(label_name, *options.label_regex)) {
        filtered_metrics.push_back(m);
      }
    }
  }
};

template <typename Tag>
class static_metric_manager {
 public:
  static_metric_manager(static_metric_manager const&) = delete;
  static_metric_manager(static_metric_manager&&) = delete;
  static_metric_manager& operator=(static_metric_manager const&) = delete;
  static_metric_manager& operator=(static_metric_manager&&) = delete;

  static static_metric_manager<Tag>& instance() {
    static static_metric_manager<Tag> inst;
    return inst;
  }

  template <typename T, typename... Args>
  std::pair<std::error_code, std::shared_ptr<T>> create_metric_static(
      const std::string& name, const std::string& help, Args&&... args) {
    auto m = std::make_shared<T>(name, help, std::forward<Args>(args)...);
    bool r = register_metric(m);
    if (!r) {
      return std::make_pair(std::make_error_code(std::errc::invalid_argument),
                            nullptr);
    }

    return std::make_pair(std::error_code{}, m);
  }

  bool register_metric(std::shared_ptr<static_metric> metric) {
    return manager_helper::register_metric(metric_map_, metric);
  }

  size_t metric_count() { return metric_map_.size(); }

  auto metric_map() { return metric_map_; }

  auto collect() {
    std::vector<std::shared_ptr<metric_t>> metrics;

    for (auto& pair : metric_map_) {
      metrics.push_back(pair.second);
    }

    return metrics;
  }

  std::string serialize(const std::vector<std::shared_ptr<metric_t>>& metrics) {
    return manager_helper::serialize(metrics);
  }

  std::string serialize_static() {
    return manager_helper::serialize(collect());
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  std::string serialize_to_json_static() {
    return manager_helper::serialize_to_json(collect());
  }
#endif
  template <typename T>
  std::shared_ptr<T> get_metric_static(const std::string& name) {
    static_assert(std::is_base_of_v<static_metric, T>,
                  "must be dynamic metric");
    auto it = metric_map_.find(name);
    if (it == metric_map_.end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<T>(it->second);
  }

  std::shared_ptr<static_metric> get_metric_by_name(const std::string& name) {
    auto it = metric_map_.find(name);
    if (it == metric_map_.end()) {
      return nullptr;
    }

    return it->second;
  }

  std::vector<std::shared_ptr<static_metric>> get_metric_by_label(
      const std::map<std::string, std::string>& labels) {
    std::vector<std::shared_ptr<static_metric>> metrics;

    for (auto& [key, m] : metric_map_) {
      if (m->get_static_labels() == labels) {
        metrics.push_back(m);
      }
    }

    return metrics;
  }

  std::vector<std::shared_ptr<metric_t>> filter_metrics_by_label_value(
      const std::regex& label_regex, bool is_white = true) {
    auto metrics = collect();
    return manager_helper::filter_metrics_by_label_value(metrics, label_regex,
                                                         is_white);
  }

  std::vector<std::shared_ptr<metric_t>> filter_metrics_static(
      const metric_filter_options& options) {
    auto metrics = collect();
    return manager_helper::filter_metrics(metrics, options);
  }

 private:
  static_metric_manager() = default;

  std::unordered_map<std::string, std::shared_ptr<static_metric>> metric_map_;
};

// using metric_manager_t = static_metric_manager;

template <typename Tag>
class dynamic_metric_manager {
 public:
  dynamic_metric_manager(dynamic_metric_manager const&) = delete;
  dynamic_metric_manager(dynamic_metric_manager&&) = delete;
  dynamic_metric_manager& operator=(dynamic_metric_manager const&) = delete;
  dynamic_metric_manager& operator=(dynamic_metric_manager&&) = delete;

  static dynamic_metric_manager<Tag>& instance() {
    static dynamic_metric_manager<Tag> inst;
    return inst;
  }

  template <typename T, typename... Args>
  std::pair<std::error_code, std::shared_ptr<T>> create_metric_dynamic(
      const std::string& name, const std::string& help, Args&&... args) {
    auto m = std::make_shared<T>(name, help, std::forward<Args>(args)...);
    bool r = register_metric(m);
    if (!r) {
      return std::make_pair(std::make_error_code(std::errc::invalid_argument),
                            nullptr);
    }

    return std::make_pair(std::error_code{}, m);
  }

  bool register_metric(std::shared_ptr<dynamic_metric> metric) {
    std::unique_lock lock(mtx_);
    return manager_helper::register_metric(metric_map_, metric);
  }

  bool register_metric(std::vector<std::shared_ptr<dynamic_metric>> metrics) {
    bool r = false;
    for (auto& m : metrics) {
      r = register_metric(m);
      if (!r) {
        break;
      }
    }

    return r;
  }

  std::string serialize_dynamic() {
    return manager_helper::serialize(collect());
  }

  std::string serialize(const std::vector<std::shared_ptr<metric_t>>& metrics) {
    return manager_helper::serialize(metrics);
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  std::string serialize_to_json_dynamic() {
    return manager_helper::serialize_to_json(collect());
  }
#endif

  bool remove_metric(const std::string& name) {
    std::unique_lock lock(mtx_);
    return metric_map_.erase(name);
  }

  bool remove_metric(std::shared_ptr<dynamic_metric> metric) {
    if (metric == nullptr) {
      return false;
    }

    return remove_metric(metric->str_name());
  }

  void remove_metric(const std::vector<std::string>& names) {
    if (names.empty()) {
      return;
    }

    for (auto& name : names) {
      remove_metric(name);
    }
  }

  void remove_metric(std::vector<std::shared_ptr<dynamic_metric>> metrics) {
    if (metrics.empty()) {
      return;
    }

    for (auto& metric : metrics) {
      remove_metric(metric);
    }
  }

  void remove_metric_by_label(
      const std::vector<std::pair<std::string, std::string>>& labels) {
    std::vector<std::string> label_value;
    for (auto& [k, v] : labels) {
      label_value.push_back(v);
    }

    remove_metric_by_label_value(label_value);
  }

  void remove_metric_by_label_name(
      const std::vector<std::string>& labels_name) {
    std::unique_lock lock(mtx_);
    for (auto& [name, m] : metric_map_) {
      if (m->labels_name() == labels_name) {
        metric_map_.erase(name);
        break;
      }
    }
  }

  void remove_metric_by_label_name(std::string_view labels_name) {
    std::unique_lock lock(mtx_);
    for (auto it = metric_map_.cbegin(); it != metric_map_.cend();) {
      auto& names = it->second->labels_name();
      if (auto sit = std::find(names.begin(), names.end(), labels_name);
          sit != names.end()) {
        metric_map_.erase(it++);
      }
      else {
        ++it;
      }
    }
  }

  size_t metric_count() {
    std::unique_lock lock(mtx_);
    return metric_map_.size();
  }

  auto metric_map() {
    std::unique_lock lock(mtx_);
    return metric_map_;
  }

  auto collect() {
    std::vector<std::shared_ptr<metric_t>> metrics;
    {
      std::unique_lock lock(mtx_);
      for (auto& pair : metric_map_) {
        metrics.push_back(pair.second);
      }
    }
    return metrics;
  }

  template <typename T>
  std::shared_ptr<T> get_metric_dynamic(const std::string& name) {
    static_assert(std::is_base_of_v<dynamic_metric, T>,
                  "must be dynamic metric");
    auto map = metric_map();
    auto it = map.find(name);
    if (it == map.end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<T>(it->second);
  }

  std::shared_ptr<dynamic_metric> get_metric_by_name(std::string_view name) {
    auto map = metric_map();
    auto it = map.find(name);
    if (it == map.end()) {
      return nullptr;
    }

    return it->second;
  }

  std::vector<std::shared_ptr<dynamic_metric>> get_metric_by_label(
      const std::vector<std::pair<std::string, std::string>>& labels) {
    std::vector<std::string> label_value;
    for (auto& [k, v] : labels) {
      label_value.push_back(v);
    }

    return get_metric_by_label_value(label_value);
  }

  std::vector<std::shared_ptr<dynamic_metric>> get_metric_by_label_name(
      const std::vector<std::string>& labels_name) {
    auto map = metric_map();
    std::vector<std::shared_ptr<dynamic_metric>> vec;
    for (auto& [name, m] : map) {
      if (m->labels_name() == labels_name) {
        vec.push_back(m);
      }
    }
    return vec;
  }

  std::vector<std::shared_ptr<metric_t>> filter_metrics_dynamic(
      const metric_filter_options& options) {
    auto metrics = collect();
    return manager_helper::filter_metrics(metrics, options);
  }

  std::vector<std::shared_ptr<metric_t>> filter_metrics_by_label_value(
      const std::regex& label_regex, bool is_white = true) {
    auto metrics = collect();
    return manager_helper::filter_metrics_by_label_value(metrics, label_regex,
                                                         is_white);
  }

 private:
  dynamic_metric_manager() = default;

  std::vector<std::shared_ptr<dynamic_metric>> get_metric_by_label_value(
      const std::vector<std::string>& label_value) {
    auto map = metric_map();
    std::vector<std::shared_ptr<dynamic_metric>> vec;
    for (auto& [name, m] : map) {
      if (m->has_label_value(label_value)) {
        vec.push_back(m);
      }
    }
    return vec;
  }

  void remove_metric_by_label_value(
      const std::vector<std::string>& label_value) {
    std::unique_lock lock(mtx_);
    for (auto& [name, m] : metric_map_) {
      if (m->has_label_value(label_value)) {
        metric_map_.erase(name);
        break;
      }
    }
  }

  std::shared_mutex mtx_;
  std::unordered_map<std::string, std::shared_ptr<dynamic_metric>> metric_map_;
};

struct ylt_default_metric_tag_t {};
using default_static_metric_manager =
    static_metric_manager<ylt_default_metric_tag_t>;
using default_dynamiv_metric_manager =
    dynamic_metric_manager<ylt_default_metric_tag_t>;

template <typename Tag>
struct metric_manager_t;

struct ylt_system_tag_t {};
using system_metric_manager = static_metric_manager<ylt_system_tag_t>;

template <typename... Args>
struct metric_collector_t {
  static std::string serialize() {
    auto vec = get_all_metrics();
    return manager_helper::serialize(vec);
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  static std::string serialize_to_json() {
    auto vec = get_all_metrics();
    return manager_helper::serialize_to_json(vec);
  }
#endif

  static std::string serialize(std::vector<std::shared_ptr<metric_t>> metrics) {
    return manager_helper::serialize(metrics);
  }

  static std::vector<std::shared_ptr<metric_t>> get_all_metrics() {
    std::vector<std::shared_ptr<metric_t>> vec;
    (append_vector<Args>(vec), ...);
    return vec;
  }

  static std::vector<std::shared_ptr<metric_t>> filter_metrics(
      const metric_filter_options& options) {
    auto vec = get_all_metrics();
    return manager_helper::filter_metrics(vec, options);
  }

  static std::vector<std::shared_ptr<metric_t>> filter_metrics_by_label_value(
      const std::regex& label_regex, bool is_white = true) {
    auto vec = get_all_metrics();
    return manager_helper::filter_metrics_by_label_value(vec, label_regex,
                                                         is_white);
  }

 private:
  template <typename T>
  static void append_vector(std::vector<std::shared_ptr<metric_t>>& vec) {
    auto v = T::instance().collect();
    vec.insert(vec.end(), v.begin(), v.end());
  }
};
}  // namespace ylt::metric