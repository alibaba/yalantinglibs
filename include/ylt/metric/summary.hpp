#pragma once
#include <atomic>

#include "detail/time_window_quantiles.hpp"
#include "metric.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/util/concurrentqueue.h"

namespace ylt {
class summary_t : public metric_t {
 public:
  using Quantiles = std::vector<CKMSQuantiles::Quantile>;
  summary_t(std::string name, std::string help, Quantiles quantiles,
            std::chrono::milliseconds max_age = std::chrono::seconds{60},
            int age_buckets = 5)
      : quantiles_{std::move(quantiles)},
        metric_t(MetricType::Summary, std::move(name), std::move(help)) {
    work_ = std::make_shared<asio::io_context::work>(ctx_);
    thd_ = std::thread([this] {
      ctx_.run();
    });
    excutor_ =
        std::make_unique<coro_io::ExecutorWrapper<>>(ctx_.get_executor());
    block_ = std::make_shared<block_t>();
    block_->quantile_values_ =
        std::make_shared<TimeWindowQuantiles>(quantiles_, max_age, age_buckets);
    start_timer(block_).via(excutor_.get()).start([](auto &&) {
    });
  }

  ~summary_t() {
    block_->stop_ = true;
    work_ = nullptr;
    thd_.join();
  }

  struct block_t {
    std::atomic<bool> stop_ = false;
    moodycamel::ConcurrentQueue<double> sample_queue_;
    std::shared_ptr<TimeWindowQuantiles> quantile_values_;
    std::uint64_t count_;
    double sum_;
  };

  void observe(double value) { block_->sample_queue_.enqueue(value); }

  async_simple::coro::Lazy<std::vector<double>> get_rates(double &sum,
                                                          uint64_t &count) {
    std::vector<double> vec;
    if (quantiles_.empty()) {
      co_return std::vector<double>{};
    }

    co_await coro_io::post([this, &vec, &sum, &count] {
      sum = block_->sum_;
      count = block_->count_;
      for (const auto &quantile : quantiles_) {
        vec.push_back(block_->quantile_values_->get(quantile.quantile));
      }
    });

    co_return vec;
  }

  async_simple::coro::Lazy<double> get_sum() {
    auto ret = co_await coro_io::post([this] {
      return block_->sum_;
    });
    co_return ret.value();
  }

  async_simple::coro::Lazy<uint64_t> get_count() {
    auto ret = co_await coro_io::post([this] {
      return block_->count_;
    });
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
      str.append("{quantile=\"");
      str.append(std::to_string(quantiles_[i].quantile)).append("\"} ");
      str.append(std::to_string(rates[i])).append("\n");
    }

    str.append(name_).append("_sum ").append(std::to_string(sum)).append("\n");
    str.append(name_)
        .append("_count ")
        .append(std::to_string((uint64_t)count))
        .append("\n");
  }

 private:
  async_simple::coro::Lazy<void> start_timer(std::shared_ptr<block_t> block) {
    double sample;
    size_t count = 1000000;
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

      co_await async_simple::coro::Yield{};

      if (block->sample_queue_.size_approx() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }

    co_return;
  }

  Quantiles quantiles_;  // readonly
  std::shared_ptr<block_t> block_;
  std::unique_ptr<coro_io::ExecutorWrapper<>> excutor_ = nullptr;
  std::shared_ptr<asio::io_context::work> work_;
  asio::io_context ctx_;
  std::thread thd_;
};
}  // namespace ylt