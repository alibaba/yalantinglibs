#pragma once
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace ylt {
enum class MetricType {
  Counter,
  Guage,
  Histogram,
  Summary,
  Nil,
};

struct sample_t {
  double value;
  int64_t timestamp;
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
  std::string_view name() { return name_; }

  std::string_view help() { return help_; }

  MetricType metric_type() { return type_; }

  std::string_view metric_name() {
    switch (type_) {
      case MetricType::Counter:
        return "counter";
      case MetricType::Guage:
        return "guage";
      case MetricType::Histogram:
        return "histogram";
      case MetricType::Summary:
        return "summary";
      case MetricType::Nil:
        return "unknown";
    }
  }

  const std::vector<std::string>& labels_name() { return labels_name_; }
  void enable_timestamp(bool r) { enable_timestamp_ = r; }

  virtual std::map<std::vector<std::string>, sample_t,
                   std::less<std::vector<std::string>>>
  values(bool need_lock = true) {
    return {};
  }

  virtual void serialize(std::string& out) {}

  static void regiter_metric(std::shared_ptr<metric_t> metric) {
    std::scoped_lock guard(mtx_);
    std::string name(metric->name());
    auto pair = metric_map_.emplace(name, std::move(metric));
    if (!pair.second) {
      throw std::invalid_argument("duplicate metric name: " + name);
    }
  }

  static void remove_metric(std::string name) {
    std::scoped_lock guard(mtx_);
    metric_map_.erase(name);
  }

  static auto collect() {
    std::vector<std::shared_ptr<metric_t>> metrics;
    {
      std::scoped_lock guard(mtx_);
      for (auto& pair : metric_map_) {
        metrics.push_back(pair.second);
      }
    }
    return metrics;
  }

  static auto metric_map() {
    std::scoped_lock guard(mtx_);
    return metric_map_;
  }

  static size_t metric_count() {
    std::scoped_lock guard(mtx_);
    return metric_map_.size();
  }

  static std::vector<std::string> metric_keys() {
    std::vector<std::string> keys;
    {
      std::scoped_lock guard(mtx_);
      for (auto& pair : metric_map_) {
        keys.push_back(pair.first);
      }
    }

    return keys;
  }

 protected:
  void set_metric_type(MetricType type) { type_ = type; }

  MetricType type_ = MetricType::Nil;
  std::string name_;
  std::string help_;
  std::vector<std::string> labels_name_;
  bool enable_timestamp_ = false;
  static inline std::mutex mtx_;
  static inline std::map<std::string, std::shared_ptr<metric_t>> metric_map_;
};
}  // namespace ylt