/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <async_simple/Executor.h>
#include <async_simple/coro/Lazy.h>

#include <any>
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/steady_timer.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "asio/dispatch.hpp"
#include "asio/executor.hpp"
#include "async_simple/Common.h"
#include "async_simple/Signal.h"
#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#endif

namespace coro_io {

inline asio::io_context **get_current() {
  static thread_local asio::io_context *current = nullptr;
  return &current;
}

template <typename ExecutorImpl = asio::io_context::executor_type>
class ExecutorWrapper : public async_simple::Executor {
 private:
  ExecutorImpl executor_;
  std::unique_ptr<std::unordered_map<std::string, std::any>> user_defined_data_;

 public:
  ExecutorWrapper(ExecutorImpl executor) : executor_(executor) {}

  using context_t = std::remove_cvref_t<decltype(executor_.context())>;

  template <typename T>
  std::any &set_data(std::string key, T data) {
    if (!user_defined_data_) {
      user_defined_data_ =
          std::make_unique<std::unordered_map<std::string, std::any>>();
    }
    return (*user_defined_data_)[std::move(key)] = std::move(data);
  }
  template <typename T>
  T *get_data_with_default(std::string key) {
    if (!user_defined_data_) {
      user_defined_data_ =
          std::make_unique<std::unordered_map<std::string, std::any>>();
    }
    auto [iter, _] = user_defined_data_->try_emplace(key, T{});
    return std::any_cast<T>(&iter->second);
  }
  template <typename T>
  T *get_data(const std::string &key) {
    if (!user_defined_data_) {
      return nullptr;
    }
    auto iter = (*user_defined_data_).find(key);
    if (iter == user_defined_data_->end()) {
      return nullptr;
    }
    return std::any_cast<T>(&iter->second);
  }

  void clear_all_data() {
    if (user_defined_data_) {
      user_defined_data_ = nullptr;
    }
  }

  virtual bool schedule(Func func) override {
    asio::post(executor_, std::move(func));
    return true;
  }

  virtual bool schedule(Func func, uint64_t hint) override {
    if (hint >=
        static_cast<uint64_t>(async_simple::Executor::Priority::YIELD)) {
      asio::post(executor_, std::move(func));
    }
    else {
      asio::dispatch(executor_, std::move(func));
    }
    return true;
  }

  virtual bool checkin(Func func, void *ctx) override {
    using context_t = std::remove_cvref_t<decltype(executor_.context())>;
    auto &executor = *(context_t *)ctx;
    asio::post(executor, std::move(func));
    return true;
  }
  virtual void *checkout() override { return &executor_.context(); }

  context_t &context() { return executor_.context(); }

  auto get_asio_executor() const { return executor_; }

  operator ExecutorImpl() { return executor_; }

  bool currentThreadInExecutor() const override {
    return executor_.running_in_this_thread();
  }

  size_t currentContextId() const override {
    return (size_t)&executor_.context();
  }

 private:
  void schedule(Func func, Duration dur) override {
    auto timer = std::make_unique<asio::steady_timer>(executor_, dur);
    auto tm = timer.get();
    tm->async_wait([fn = std::move(func), timer = std::move(timer)](auto ec) {
      fn();
    });
  }
  void schedule(Func func, Duration dur, uint64_t hint,
                async_simple::Slot *slot = nullptr) override {
    auto timer =
        std::make_shared<std::pair<asio::steady_timer, std::atomic<bool>>>(
            asio::steady_timer{executor_, dur}, false);
    if (!slot) {
      timer->first.async_wait([fn = std::move(func), timer](const auto &ec) {
        fn();
      });
    }
    else {
      if (!async_simple::signalHelper{async_simple::SignalType::Terminate}
               .tryEmplace(
                   slot, [timer](auto signalType, auto *signal) mutable {
                     if (bool expected = false;
                         !timer->second.compare_exchange_strong(
                             expected, true, std::memory_order_acq_rel)) {
                       timer->first.cancel();
                     }
                   })) {
        asio::dispatch(timer->first.get_executor(), func);
      }
      else {
        timer->first.async_wait([fn = std::move(func), timer](const auto &ec) {
          fn();
        });
        if (bool expected = false; !timer->second.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel)) {
          timer->first.cancel();
        }
      }
    }
  }
};

template <typename ExecutorImpl = asio::io_context>
inline async_simple::coro::Lazy<typename ExecutorImpl::executor_type>
get_current_executor() {
  auto executor = co_await async_simple::CurrentExecutor{};
  assert(executor != nullptr);
  co_return static_cast<ExecutorImpl *>(executor->checkout())->get_executor();
}

class io_context_pool {
 public:
  using executor_type = asio::io_context::executor_type;
  explicit io_context_pool(std::size_t pool_size, bool cpu_affinity = false)
      : next_io_context_(0), cpu_affinity_(cpu_affinity) {
    if (pool_size == 0) {
      pool_size = 1;  // set default value as 1
    }

    total_thread_num_ += pool_size;

    for (std::size_t i = 0; i < pool_size; ++i) {
      io_context_ptr io_context(new asio::io_context(1));
      work_ptr work(new asio::io_context::work(*io_context));
      io_contexts_.push_back(io_context);
      auto executor = std::make_unique<coro_io::ExecutorWrapper<>>(
          io_context->get_executor());
      executors.push_back(std::move(executor));
      work_.push_back(work);
    }
  }

  void run() {
    bool has_run_or_stop = false;
    bool ok = has_run_or_stop_.compare_exchange_strong(has_run_or_stop, true);
    if (!ok) {
      return;
    }

    std::vector<std::shared_ptr<std::thread>> threads;
    for (std::size_t i = 0; i < io_contexts_.size(); ++i) {
      threads.emplace_back(std::make_shared<std::thread>(
          [](io_context_ptr svr) {
            auto ctx = get_current();
            *ctx = svr.get();
            svr->run();
          },
          io_contexts_[i]));

#ifdef __linux__
      if (cpu_affinity_) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);

#ifdef __ANDROID__
        const pid_t tid = pthread_gettid_np(threads.back()->native_handle());
        int rc = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);
#else
        int rc = pthread_setaffinity_np(threads.back()->native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
#endif
        if (rc != 0) {
          std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
      }
#endif
    }

    for (std::size_t i = 0; i < threads.size(); ++i) {
      threads[i]->join();
    }
    promise_.set_value();
  }

  void stop(bool force = false) {
    std::call_once(flag_, [this, force] {
      bool has_run_or_stop = false;
      bool ok = has_run_or_stop_.compare_exchange_strong(has_run_or_stop, true);

      for (auto &executor : executors) {
        executor->schedule([&executor]() {
          executor->clear_all_data();
        });
      }

      work_.clear();

      for (auto &e : io_contexts_) {
        if (ok) {
          e->poll();  // clear all unfinished work
        }
        if (force) {
          e->stop();
        }
      }
      if (!ok)
        promise_.get_future().wait();
    });
  }

  ~io_context_pool() {
    if (!has_stop())
      stop();
  }

  std::size_t pool_size() const noexcept { return io_contexts_.size(); }

  std::span<std::unique_ptr<coro_io::ExecutorWrapper<>>> get_all_executor() {
    return executors;
  }

  bool has_stop() const { return work_.empty(); }

  size_t current_io_context() { return next_io_context_ - 1; }

  coro_io::ExecutorWrapper<> *get_executor() {
    auto i = next_io_context_.fetch_add(1, std::memory_order::relaxed);
    auto *ret = executors[i % io_contexts_.size()].get();
    return ret;
  }

  template <typename T>
  friend io_context_pool &g_io_context_pool();

  static size_t get_total_thread_num() { return total_thread_num_; }

 private:
  using io_context_ptr = std::shared_ptr<asio::io_context>;
  using work_ptr = std::shared_ptr<asio::io_context::work>;

  std::vector<io_context_ptr> io_contexts_;
  std::vector<std::unique_ptr<coro_io::ExecutorWrapper<>>> executors;
  std::vector<work_ptr> work_;
  std::atomic<std::size_t> next_io_context_;
  std::promise<void> promise_;
  std::atomic<bool> has_run_or_stop_ = false;
  std::once_flag flag_;
  bool cpu_affinity_ = false;
  inline static std::atomic<size_t> total_thread_num_ = 0;
};

inline size_t get_total_thread_num() {
  return io_context_pool::get_total_thread_num();
}

class multithread_context_pool {
 public:
  multithread_context_pool(size_t thd_num = std::thread::hardware_concurrency())
      : work_(std::make_unique<asio::io_context::work>(ioc_)),
        executor_(ioc_.get_executor()),
        thd_num_(thd_num) {}

  ~multithread_context_pool() { stop(); }

  void run() {
    for (int i = 0; i < thd_num_; i++) {
      thds_.emplace_back([this] {
        ioc_.run();
      });
    }

    promise_.set_value();
  }

  void stop() {
    if (thds_.empty()) {
      return;
    }

    work_.reset();
    for (auto &thd : thds_) {
      thd.join();
    }
    promise_.get_future().wait();
    thds_.clear();
  }

  coro_io::ExecutorWrapper<> *get_executor() { return &executor_; }

 private:
  asio::io_context ioc_;
  std::unique_ptr<asio::io_context::work> work_;
  coro_io::ExecutorWrapper<> executor_;
  size_t thd_num_;
  std::vector<std::thread> thds_;
  std::promise<void> promise_;
};

template <typename T = io_context_pool>
inline T &g_io_context_pool(
    unsigned pool_size = std::thread::hardware_concurrency()) {
  static auto _g_io_context_pool = std::make_shared<T>(pool_size);
  [[maybe_unused]] static bool run_helper = [](auto pool) {
    std::thread thrd{[pool] {
      pool->run();
    }};
    thrd.detach();
    return true;
  }(_g_io_context_pool);
  return *_g_io_context_pool;
}

template <typename T = io_context_pool>
inline std::shared_ptr<T> create_io_context_pool(
    unsigned pool_size = std::thread::hardware_concurrency()) {
  auto pool = std::make_shared<T>(pool_size);
  std::thread thrd{[pool] {
    pool->run();
  }};
  thrd.detach();

  return pool;
}

template <typename T = io_context_pool>
inline T &g_block_io_context_pool(
    unsigned pool_size = std::thread::hardware_concurrency()) {
  static auto _g_io_context_pool = std::make_shared<T>(pool_size);
  [[maybe_unused]] static bool run_helper = [](auto pool) {
    std::thread thrd{[pool] {
      pool->run();
    }};
    thrd.detach();
    return true;
  }(_g_io_context_pool);
  return *_g_io_context_pool;
}

template <typename T = io_context_pool>
inline auto get_global_executor(
    unsigned pool_size = std::thread::hardware_concurrency()) {
  return g_io_context_pool<T>(pool_size).get_executor();
}

template <typename T = io_context_pool>
inline auto get_global_block_executor(
    unsigned pool_size = std::thread::hardware_concurrency()) {
  return g_block_io_context_pool<T>(pool_size).get_executor();
}

}  // namespace coro_io
