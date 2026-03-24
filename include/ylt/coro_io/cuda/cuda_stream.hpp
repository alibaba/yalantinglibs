/*
 * Copyright (c) 2026, Alibaba Group Holding Limited;
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
#include <atomic>
#include <chrono>
#include <list>
#include <memory>
#include <thread>

#include "asio/high_resolution_timer.hpp"
#include "asio/io_context.hpp"
#include "async_simple/Executor.h"
#include "async_simple/Promise.h"
#include "async_simple/coro/Lazy.h"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/cuda/cuda_device.hpp"
#include "ylt/coro_io/detail/client_queue.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"

namespace coro_io {

class cuda_event_t {
  std::unique_ptr<CUevent> event_;

 public:
  cuda_event_t(cuda_event_t&&) = default;
  cuda_event_t& operator=(cuda_event_t&&) = default;
  operator CUevent*() { return event_.get(); }
  cuda_event_t(int flag = CU_EVENT_DISABLE_TIMING) {
    event_ = std::make_unique<CUevent>();
    cuEventCreate(event_.get(), flag);
  }
  void record(CUstream& stream) { cuEventRecord(*event_, stream); }
  ~cuda_event_t() {
    if (event_ != nullptr) {
      cuEventDestroy(*event_);
    }
  }
};

class cuda_event_watcher
    : public std::enable_shared_from_this<cuda_event_watcher> {
 private:
  using event_node_t = std::pair<async_simple::Promise<CUresult>, cuda_event_t>;
  std::chrono::microseconds sleep_interval_ = std::chrono::microseconds(20);
  detail::moodycamel::ConcurrentQueue<event_node_t> events_queue_;
  std::list<event_node_t> events_list_;
  coro_io::io_context_pool io_context_pool_;
  coro_io::ExecutorWrapper<>* executor_ = nullptr;
  coro_io::high_resolution_timer timer_;
  bool has_trigger_event_ = false;

  alignas(64) std::atomic<bool> is_sleeping_ = false;

 public:
  cuda_event_watcher(bool bind_cpu = false)
      : io_context_pool_(1, bind_cpu),
        executor_(io_context_pool_.get_executor()),
        timer_(executor_) {}
  static std::shared_ptr<cuda_event_watcher> get_instance(
      coro_io::ExecutorWrapper<>* executor = nullptr) {
    static auto instance = get_instance_impl(executor);
    return instance;
  }

 private:
  static std::shared_ptr<cuda_event_watcher> get_instance_impl(
      coro_io::ExecutorWrapper<>* executor = nullptr) {
    auto instance = std::make_shared<cuda_event_watcher>(executor);
    instance->init();
    return instance;
  }
  void init() {
    std::thread thrd{[self = shared_from_this()] {
      self->io_context_pool_.run();
    }};
    thrd.detach();
    watch_event().via(executor_).detach();
  }
  bool trigger_event(event_node_t& node) {
    CUresult result = cuEventQuery(*node.second);
    if (result != CUDA_ERROR_NOT_READY) {
      node.first.setValue(result);
      has_trigger_event_ = true;
      ELOG_TRACE << "trigger event!";
      return true;
    }
    else {
      return false;
    }
  }
  async_simple::coro::Lazy<void> watch_event() {
    event_node_t node;
    auto pre_tick = std::chrono::steady_clock::now();
    auto sleep_interval = sleep_interval_;
    while (true) {
      for (auto iter = events_list_.begin(); iter != events_list_.end();) {
        if (trigger_event(*iter)) [[unlikely]] {
          iter = events_list_.erase(iter);
        }
        else {
          ++iter;
        }
      }
      while (true) {
        if (events_queue_.try_dequeue(node)) {
          sleep_interval = sleep_interval_;
          if (!trigger_event(node)) {
            events_list_.push_back(std::move(node));
          }
        }
        else {
          break;
        }
      }
      auto now = std::chrono::steady_clock::now();
      if (has_trigger_event_) {
        has_trigger_event_ = false;
        pre_tick = now;
        sleep_interval = sleep_interval_;
        continue;
      }
      if (sleep_interval_ <= std::chrono::microseconds{0}) {
        std::this_thread::yield();
        continue;
      }
      auto dur = now - pre_tick;
      if (dur >= sleep_interval_) {
        // ELOG_TRACE << "start sleep! dur before last active work= " <<
        // dur/std::chrono::milliseconds{1} << "ms";
        timer_.expires_after(sleep_interval);
        is_sleeping_.store(true, std::memory_order_release);
        co_await timer_.async_await();
        is_sleeping_.store(false, std::memory_order_release);
        // ELOG_TRACE << "finish sleep! start watch cuda events";
        if (sleep_interval < std::chrono::seconds{1})
          sleep_interval = sleep_interval * 6 / 5;
        else {
          sleep_interval_ = std::chrono::seconds{1};
        }
      }
    }
  }

 public:
  static void post(async_simple::Promise<CUresult>&& promise,
                   cuda_event_t&& event) {
    auto self = get_instance();
    self->events_queue_.enqueue(
        event_node_t{std::move(promise), std::move(event)});
    if (self->is_sleeping_.load(std::memory_order_acquire)) {
      self->executor_->schedule([self]() {
        if (self->is_sleeping_.load(std::memory_order_relaxed)) {
          std::error_code ec;
          self->timer_.cancel(ec);
        }
      });
    }
  }
};

class cuda_stream_handler_t {
  cuda_stream_handler_t(const cuda_stream_handler_t&) = delete;
  cuda_stream_handler_t& operator=(const cuda_stream_handler_t&) = delete;

 public:
  cuda_stream_handler_t(cuda_stream_handler_t&&) = default;
  cuda_stream_handler_t& operator=(cuda_stream_handler_t&&) = default;

  cuda_stream_handler_t(std::shared_ptr<cuda_device_t> device)
      : device_(std::move(device)) {
    device_->set_context();
    cuStreamCreate(&stream_, CU_STREAM_DEFAULT);
  }

  operator bool() const { return stream_ != nullptr; }
  int get_gpu_id() const { return device_ ? device_->get_gpu_id() : -1; }
  cuda_stream_handler_t(int gpu_id = -1) {
    if (gpu_id >= 0) {
      *this = cuda_stream_handler_t{cuda_device_t::get_cuda_device(gpu_id)};
    }
  }
  async_simple::Future<CUresult> record(
      async_simple::Executor* executor = nullptr) {
    cuda_event_t event;
    event.record(stream_);
    async_simple::Promise<CUresult> p;
    auto future = p.getFuture().via(executor);
    cuda_event_watcher::post(std::move(p), std::move(event));
    return std::move(future);
  }
  ~cuda_stream_handler_t() {
    if (device_) {
      device_->set_context();
      cuStreamDestroy(stream_);
    }
  }
  CUstream get_stream() { return stream_; }
  cuda_device_t& get_device() { return *device_; }

 private:
  CUstream stream_;
  std::shared_ptr<cuda_device_t> device_;
};
}  // namespace coro_io