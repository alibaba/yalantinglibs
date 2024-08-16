#pragma once
#include <algorithm>
#include <atomic>

#include "detail/time_window_quantiles.hpp"
#include "metric.hpp"
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
REFLECTION(json_summary_metric_t, labels, quantiles, count, sum);
struct json_summary_t {
  std::string name;
  std::string help;
  std::string type;
  std::vector<json_summary_metric_t> metrics;
};
REFLECTION(json_summary_t, name, help, type, metrics);
#endif

struct block_t {
  std::atomic<bool> stop_ = false;
  ylt::detail::moodycamel::ConcurrentQueue<double> sample_queue_;
  std::shared_ptr<TimeWindowQuantiles> quantile_values_;
  std::uint64_t count_;
  double sum_;
};

class summary_t : public static_metric {
 public:
  using Quantiles = std::vector<CKMSQuantiles::Quantile>;
  summary_t(std::string name, std::string help, Quantiles quantiles,
            std::chrono::milliseconds max_age = std::chrono::seconds{60},
            int age_buckets = 5)
      : quantiles_{std::move(quantiles)},
        static_metric(MetricType::Summary, std::move(name), std::move(help)),
        max_age_(max_age),
        age_buckets_(age_buckets) {
    init_no_label(max_age, age_buckets);
  }

  summary_t(std::string name, std::string help, Quantiles quantiles,
            std::map<std::string, std::string> static_labels,
            std::chrono::milliseconds max_age = std::chrono::seconds{60},
            int age_buckets = 5)
      : quantiles_{std::move(quantiles)},
        static_metric(MetricType::Summary, std::move(name), std::move(help),
                      std::move(static_labels)),
        max_age_(max_age),
        age_buckets_(age_buckets) {
    init_no_label(max_age, age_buckets);
  }

  ~summary_t() {
    if (block_) {
      block_->stop_ = true;
    }
  }

  void observe(double value) {
    int64_t max_limit = (std::min)(ylt_label_capacity, (int64_t)1000000);
    if (block_->sample_queue_.size_approx() >= max_limit) {
      g_summary_failed_count++;
      return;
    }
    block_->sample_queue_.enqueue(value);

    bool expected = false;
    if (is_coro_started_.compare_exchange_strong(expected, true)) {
      start(block_).via(excutor_->get_executor()).start([](auto &&) {
      });
    }
  }

  async_simple::coro::Lazy<std::vector<double>> get_rates(double &sum,
                                                          uint64_t &count) {
    std::vector<double> vec;
    if (quantiles_.empty()) {
      co_return std::vector<double>{};
    }

    co_await coro_io::post(
        [this, &vec, &sum, &count] {
          sum = block_->sum_;
          count = block_->count_;
          for (const auto &quantile : quantiles_) {
            vec.push_back(block_->quantile_values_->get(quantile.quantile));
          }
        },
        excutor_->get_executor());

    co_return vec;
  }

  async_simple::coro::Lazy<double> get_sum() {
    auto ret = co_await coro_io::post(
        [this] {
          return block_->sum_;
        },
        excutor_->get_executor());
    co_return ret.value();
  }

  async_simple::coro::Lazy<uint64_t> get_count() {
    auto ret = co_await coro_io::post(
        [this] {
          return block_->count_;
        },
        excutor_->get_executor());
    co_return ret.value();
  }

  size_t size_approx() { return block_->sample_queue_.size_approx(); }

  async_simple::coro::Lazy<void> serialize_async(std::string &str) override {
    if (quantiles_.empty()) {
      co_return;
    }

    serialize_head(str);

    double sum = 0;
    uint64_t count = 0;
    auto rates = co_await get_rates(sum, count);

    for (size_t i = 0; i < quantiles_.size(); i++) {
      str.append(name_);
      str.append("{");
      if (!labels_name_.empty()) {
        build_label_string(str, labels_name_, labels_value_);
        str.append(",");
      }

      str.append("quantile=\"");
      str.append(std::to_string(quantiles_[i].quantile)).append("\"} ");
      str.append(std::to_string(rates[i])).append("\n");
    }

    str.append(name_).append("_sum ").append(std::to_string(sum)).append("\n");
    str.append(name_)
        .append("_count ")
        .append(std::to_string((uint64_t)count))
        .append("\n");
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  async_simple::coro::Lazy<void> serialize_to_json_async(
      std::string &str) override {
    if (quantiles_.empty()) {
      co_return;
    }

    json_summary_t summary{name_, help_, std::string(metric_name())};
    double sum = 0;
    uint64_t count = 0;
    auto rates = co_await get_rates(sum, count);

    json_summary_metric_t metric;

    for (size_t i = 0; i < quantiles_.size(); i++) {
      for (size_t i = 0; i < labels_name_.size(); i++) {
        metric.labels[labels_name_[i]] = labels_value_[i];
      }
      metric.quantiles.emplace(quantiles_[i].quantile, rates[i]);
    }

    metric.sum = sum;
    metric.count = count;

    summary.metrics.push_back(std::move(metric));

    iguana::to_json(summary, str);
  }
#endif
 private:
  template <typename T>
  void init_block(std::shared_ptr<T> &block) {
    block = std::make_shared<T>();
    start(block).via(excutor_->get_executor()).start([](auto &&) {
    });
  }

  void init_no_label(std::chrono::milliseconds max_age, int age_buckets) {
    init_block(block_);
    block_->quantile_values_ =
        std::make_shared<TimeWindowQuantiles>(quantiles_, max_age, age_buckets);
    g_user_metric_count++;
  }

  async_simple::coro::Lazy<void> start(std::shared_ptr<block_t> block) {
    double sample;
    size_t count = 100000;
    while (!block->stop_) {
      size_t index = 0;
      while (block->sample_queue_.try_dequeue(sample)) {
        block->quantile_values_->insert(sample);
        block->count_ += 1;
        block->sum_ += sample;
        index++;
        if (index == count) {
          break;
        }
      }

      if (block->sample_queue_.size_approx() == 0) {
        is_coro_started_ = false;
        if (block->sample_queue_.size_approx() == 0) {
          break;
        }

        bool expected = false;
        if (!is_coro_started_.compare_exchange_strong(expected, true)) {
          break;
        }

        continue;
      }

      co_await async_simple::coro::Yield{};
    }

    co_return;
  }

  Quantiles quantiles_;  // readonly
  std::shared_ptr<block_t> block_;
  static inline std::shared_ptr<coro_io::io_context_pool> excutor_ =
      coro_io::create_io_context_pool(1);
  std::chrono::milliseconds max_age_;
  int age_buckets_;
  std::atomic<bool> is_coro_started_ = false;
};

template <size_t N>
struct summary_label_sample {
  std::array<std::string, N> labels_value;
  double value;
};

template <size_t N>
struct labels_block_t {
  std::atomic<bool> stop_ = false;
  ylt::detail::moodycamel::ConcurrentQueue<summary_label_sample<N>>
      sample_queue_;
  dynamic_metric_hash_map<std::shared_ptr<TimeWindowQuantiles>, N>
      label_quantile_values_;
  dynamic_metric_hash_map<uint64_t, N> label_count_;
  dynamic_metric_hash_map<double, N> label_sum_;
};

template <size_t N>
class basic_dynamic_summary : public dynamic_metric {
 public:
  using Quantiles = std::vector<CKMSQuantiles::Quantile>;

  basic_dynamic_summary(
      std::string name, std::string help, Quantiles quantiles,
      std::array<std::string, N> labels_name,
      std::chrono::milliseconds max_age = std::chrono::seconds{60},
      int age_buckets = 5)
      : quantiles_{std::move(quantiles)},
        dynamic_metric(MetricType::Summary, std::move(name), std::move(help),
                       std::move(labels_name)),
        max_age_(max_age),
        age_buckets_(age_buckets) {
    init_block(labels_block_);
    g_user_metric_count++;
  }

  ~basic_dynamic_summary() {
    if (labels_block_) {
      labels_block_->stop_ = true;
    }
  }

  void observe(std::array<std::string, N> labels_value, double value) {
    int64_t max_limit = (std::min)(ylt_label_capacity, (int64_t)1000000);
    if (labels_block_->sample_queue_.size_approx() >= max_limit) {
      g_summary_failed_count++;
      return;
    }
    labels_block_->sample_queue_.enqueue({std::move(labels_value), value});

    bool expected = false;
    if (is_coro_started_.compare_exchange_strong(expected, true)) {
      start(labels_block_).via(excutor_->get_executor()).start([](auto &&) {
      });
    }
  }

  size_t size_approx() { return labels_block_->sample_queue_.size_approx(); }

  async_simple::coro::Lazy<std::vector<double>> get_rates(
      const std::array<std::string, N> &labels_value, double &sum,
      uint64_t &count) {
    std::vector<double> vec;
    if (quantiles_.empty()) {
      co_return std::vector<double>{};
    }

    co_await coro_io::post(
        [this, &vec, &sum, &count, &labels_value] {
          auto it = labels_block_->label_quantile_values_.find(labels_value);
          if (it == labels_block_->label_quantile_values_.end()) {
            return;
          }
          sum = labels_block_->label_sum_[labels_value];
          count = labels_block_->label_count_[labels_value];
          for (const auto &quantile : quantiles_) {
            vec.push_back(it->second->get(quantile.quantile));
          }
        },
        excutor_->get_executor());

    co_return vec;
  }

  dynamic_metric_hash_map<double, N> value_map() {
    auto ret = async_simple::coro::syncAwait(coro_io::post(
        [this] {
          return labels_block_->label_sum_;
        },
        excutor_->get_executor()));
    return ret.value();
  }

  async_simple::coro::Lazy<void> serialize_async(std::string &str) override {
    co_await serialize_async_with_label(str);
  }

#ifdef CINATRA_ENABLE_METRIC_JSON
  async_simple::coro::Lazy<void> serialize_to_json_async(
      std::string &str) override {
    co_await serialize_to_json_with_label_async(str);
  }
#endif
 private:
  template <typename T>
  void init_block(std::shared_ptr<T> &block) {
    block = std::make_shared<T>();
    start(block).via(excutor_->get_executor()).start([](auto &&) {
    });
  }

  async_simple::coro::Lazy<void> start(
      std::shared_ptr<labels_block_t<N>> label_block) {
    summary_label_sample<N> sample;
    size_t count = 100000;
    while (!label_block->stop_) {
      size_t index = 0;
      while (label_block->sample_queue_.try_dequeue(sample)) {
        auto &ptr = label_block->label_quantile_values_[sample.labels_value];

        if (ptr == nullptr) {
          ptr = std::make_shared<TimeWindowQuantiles>(quantiles_, max_age_,
                                                      age_buckets_);
        }

        ptr->insert(sample.value);

        label_block->label_count_[sample.labels_value] += 1;
        label_block->label_sum_[sample.labels_value] += sample.value;
        index++;
        if (index == count) {
          break;
        }
      }

      co_await async_simple::coro::Yield{};

      if (label_block->sample_queue_.size_approx() == 0) {
        is_coro_started_ = false;
        if (label_block->sample_queue_.size_approx() == 0) {
          break;
        }

        bool expected = false;
        if (!is_coro_started_.compare_exchange_strong(expected, true)) {
          break;
        }

        continue;
      }
      co_await async_simple::coro::Yield{};
    }

    co_return;
  }

  async_simple::coro::Lazy<void> serialize_async_with_label(std::string &str) {
    if (quantiles_.empty()) {
      co_return;
    }

    serialize_head(str);

    auto sum_map = co_await coro_io::post(
        [this] {
          return labels_block_->label_sum_;
        },
        excutor_->get_executor());

    for (auto &[labels_value, sum_val] : sum_map.value()) {
      double sum = 0;
      uint64_t count = 0;
      auto rates = co_await get_rates(labels_value, sum, count);
      for (size_t i = 0; i < quantiles_.size(); i++) {
        str.append(name_);
        str.append("{");
        build_label_string(str, labels_name_, labels_value);
        str.append(",");
        str.append("quantile=\"");
        str.append(std::to_string(quantiles_[i].quantile)).append("\"} ");
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
  async_simple::coro::Lazy<void> serialize_to_json_with_label_async(
      std::string &str) {
    if (quantiles_.empty()) {
      co_return;
    }

    auto sum_map = co_await coro_io::post(
        [this] {
          return labels_block_->label_sum_;
        },
        excutor_->get_executor());

    json_summary_t summary{name_, help_, std::string(metric_name())};

    for (auto &[labels_value, sum_val] : sum_map.value()) {
      json_summary_metric_t metric;
      double sum = 0;
      uint64_t count = 0;
      auto rates = co_await get_rates(labels_value, sum, count);
      metric.count = count;
      metric.sum = sum;
      for (size_t i = 0; i < quantiles_.size(); i++) {
        for (size_t i = 0; i < labels_value.size(); i++) {
          metric.labels[labels_name_[i]] = labels_value[i];
        }
        metric.quantiles.emplace(quantiles_[i].quantile, rates[i]);
      }

      summary.metrics.push_back(std::move(metric));
    }
    iguana::to_json(summary, str);
  }
#endif

  Quantiles quantiles_;  // readonly
  std::shared_ptr<labels_block_t<N>> labels_block_;
  static inline std::shared_ptr<coro_io::io_context_pool> excutor_ =
      coro_io::create_io_context_pool(1);
  std::chrono::milliseconds max_age_;
  int age_buckets_;
  std::atomic<bool> is_coro_started_ = false;
};

using dynamic_summary_1 = basic_dynamic_summary<1>;
using dynamic_summary_2 = basic_dynamic_summary<2>;
using dynamic_summary = dynamic_summary_2;
using dynamic_summary_3 = basic_dynamic_summary<3>;
using dynamic_summary_4 = basic_dynamic_summary<4>;
using dynamic_summary_5 = basic_dynamic_summary<5>;
}  // namespace ylt::metric