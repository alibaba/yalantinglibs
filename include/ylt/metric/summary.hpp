#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>

#include "counter.hpp"
#include "metric.hpp"
#include "summary_impl.hpp"
#if __has_include("ylt/util/concurrentqueue.h")
#include "ylt/util/concurrentqueue.h"
#else
#include "cinatra/ylt/util/concurrentqueue.h"
#endif

namespace ylt::metric {
#ifdef CINATRA_ENABLE_METRIC_JSON
struct json_summary_metric_t {
  std::map<std::string, std::string> labels;
  std::map<double, double> quantiles;
  int64_t count;
  double sum;
};
YLT_REFL(json_summary_metric_t, labels, quantiles, count, sum);
struct json_summary_t {
  std::string name;
  std::string help;
  std::string type;
  std::vector<json_summary_metric_t> metrics;
};
YLT_REFL(json_summary_t, name, help, type, metrics);
#endif

class summary_t : public static_metric {
 public:
  summary_t(std::string name, std::string help, std::vector<double> quantiles,
            std::chrono::seconds max_age = std::chrono::seconds{60})
      : static_metric(MetricType::Summary, std::move(name), std::move(help)),
        quantiles_(std::move(quantiles)),
        impl_(quantiles_,
              std::chrono::duration_cast<std::chrono::seconds>(max_age)) {
    if (!std::is_sorted(quantiles_.begin(), quantiles_.end()))
      std::sort(quantiles_.begin(), quantiles_.end());
    g_user_metric_count++;
  }

  summary_t(std::string name, std::string help, std::vector<double> quantiles,
            std::map<std::string, std::string> static_labels,
            std::chrono::seconds max_age = std::chrono::seconds{60})
      : static_metric(MetricType::Summary, std::move(name), std::move(help),
                      std::move(static_labels)),
        quantiles_(std::move(quantiles)),
        impl_(quantiles_,
              std::chrono::duration_cast<std::chrono::seconds>(max_age)) {
    if (!std::is_sorted(quantiles_.begin(), quantiles_.end()))
      std::sort(quantiles_.begin(), quantiles_.end());
    g_user_metric_count++;
  }

  void observe(float value) { impl_.insert(value); }

  std::vector<float> get_rates() {
    uint64_t count;
    double sum;
    return get_rates(sum, count);
  }
  std::vector<float> get_rates(uint64_t& count) {
    double sum;
    return get_rates(sum, count);
  }
  std::vector<float> get_rates(double& sum) {
    uint64_t count;
    return get_rates(sum, count);
  }

  std::vector<float> get_rates(double& sum, uint64_t& count) {
    return impl_.stat(sum, count);
  }

  virtual void serialize(std::string& str) override {
    if (quantiles_.empty()) {
      return;
    }
    double sum = 0;
    uint64_t count = 0;
    auto rates = get_rates(sum, count);
    if (count == 0) {
      return;
    }
    serialize_head(str);

    for (size_t i = 0; i < quantiles_.size(); i++) {
      str.append(name_);
      str.append("{");
      if (!labels_name_.empty()) {
        build_label_string(str, labels_name_, labels_value_);
        str.append(",");
      }

      str.append("quantile=\"");
      str.append(std::to_string(quantiles_[i])).append("\"} ");
      str.append(std::to_string(rates[i])).append("\n");
    }

    str.append(name_).append("_sum ").append(std::to_string(sum)).append("\n");
    str.append(name_)
        .append("_count ")
        .append(std::to_string((uint64_t)count))
        .append("\n");
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  virtual void serialize_to_json(std::string& str) override {
    if (quantiles_.empty()) {
      return;
    }

    double sum = 0;
    uint64_t count = 0;
    auto rates = get_rates(sum, count);
    if (count == 0) {
      return;
    }

    json_summary_t summary{name_, help_, std::string(metric_name())};

    json_summary_metric_t metric;

    for (size_t i = 0; i < quantiles_.size(); i++) {
      for (size_t i = 0; i < labels_name_.size(); i++) {
        metric.labels[labels_name_[i]] = labels_value_[i];
      }
      metric.quantiles.emplace(quantiles_[i], rates[i]);
    }

    metric.sum = sum;
    metric.count = count;

    summary.metrics.push_back(std::move(metric));

    iguana::to_json(summary, str);
  }
#endif

 private:
  std::vector<double> quantiles_;
  ylt::metric::detail::summary_impl<> impl_;
};

template <size_t N>
class basic_dynamic_summary : public dynamic_metric {
 private:
  auto visit(const std::array<std::string, N>& labels_value) {
    decltype(label_quantile_values_.begin()) iter;
    bool has_inserted;
    {
      std::lock_guard guard(mutex_);
      std::tie(iter, has_inserted) =
          label_quantile_values_.try_emplace(labels_value, nullptr);
      if (has_inserted) {
        iter->second = std::make_unique<detail::summary_impl<>>(quantiles_);
      }
    }
    return iter;
  }

 public:
  basic_dynamic_summary(
      std::string name, std::string help, std::vector<double> quantiles,
      std::array<std::string, N> labels_name,
      std::chrono::milliseconds max_age = std::chrono::seconds{60})
      : dynamic_metric(MetricType::Summary, std::move(name), std::move(help),
                       std::move(labels_name)),
        quantiles_(std::move(quantiles)),
        max_age_(max_age) {
    if (!std::is_sorted(quantiles_.begin(), quantiles_.end()))
      std::sort(quantiles_.begin(), quantiles_.end());
    g_user_metric_count++;
  }

  void observe(const std::array<std::string, N>& labels_value, float value) {
    visit(labels_value)->second->insert(value);
  }

  std::vector<float> get_rates(const std::array<std::string, N>& labels_value) {
    double sum;
    uint64_t count;
    return visit(labels_value)->second->get_rates(sum, count);
  }

  std::vector<float> get_rates(const std::array<std::string, N>& labels_value,
                               uint64_t& count) {
    double sum;
    return visit(labels_value)->second->get_rates(sum, count);
  }

  std::vector<float> get_rates(const std::array<std::string, N>& labels_value,
                               double& sum) {
    uint64_t count;
    return visit(labels_value)->second->get_rates(sum, count);
  }

  std::vector<float> get_rates(const std::array<std::string, N>& labels_value,
                               double& sum, uint64_t& count) {
    return visit(labels_value)->second->stat(sum, count);
  }

  virtual void serialize(std::string& str) override {
    double sum = 0;
    uint64_t count = 0;
    std::lock_guard guard(mutex_);
    // TODO: copy pointer to avoid big lock
    for (auto& [labels_value, summary_value] : label_quantile_values_) {
      auto rates = summary_value->stat(sum, count);
      for (size_t i = 0; i < quantiles_.size(); i++) {
        str.append(name_);
        str.append("{");
        build_label_string(str, labels_name_, labels_value);
        str.append(",");
        str.append("quantile=\"");
        str.append(std::to_string(quantiles_[i])).append("\"} ");
        str.append(std::to_string(rates[i])).append("\n");
      }

      str.append(name_).append("_sum ");
      str.append("{");
      build_label_string(str, labels_name_, labels_value);
      str.append("} ");
      str.append(std::to_string(sum)).append("\n");

      str.append(name_).append("_count ");
      str.append("{");
      build_label_string(str, labels_name_, labels_value);
      str.append("} ");
      str.append(std::to_string((uint64_t)count)).append("\n");
    }
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  virtual void serialize_to_json(std::string& str) override {
    json_summary_t summary{name_, help_, std::string(metric_name())};
    {
      std::lock_guard guard(mutex_);
      for (auto& [labels_value, summary_value] : label_quantile_values_) {
        json_summary_metric_t metric;
        double sum = 0;
        uint64_t count = 0;
        auto rates = summary_value->stat(sum, count);
        metric.count = count;
        metric.sum = sum;
        for (size_t i = 0; i < quantiles_.size(); i++) {
          for (size_t i = 0; i < labels_value.size(); i++) {
            metric.labels[labels_name_[i]] = labels_value[i];
          }
          metric.quantiles.emplace(quantiles_[i], rates[i]);
        }
        summary.metrics.push_back(std::move(metric));
      }
    }
    iguana::to_json(summary, str);
  }
#endif

 private:
  using hashtable_t = dynamic_metric_hash_map<
      std::array<std::string, N>,
      std::unique_ptr<ylt::metric::detail::summary_impl<>>>;
  std::mutex mutex_;
  std::vector<double> quantiles_;
  std::chrono::milliseconds max_age_;
  hashtable_t label_quantile_values_;
};

using dynamic_summary_1 = basic_dynamic_summary<1>;
using dynamic_summary_2 = basic_dynamic_summary<2>;
using dynamic_summary = dynamic_summary_2;
using dynamic_summary_3 = basic_dynamic_summary<3>;
using dynamic_summary_4 = basic_dynamic_summary<4>;
using dynamic_summary_5 = basic_dynamic_summary<5>;
}  // namespace ylt::metric