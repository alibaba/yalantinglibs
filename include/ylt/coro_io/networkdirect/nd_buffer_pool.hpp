/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>

#include "async_simple/coro/Lazy.h"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/detail/client_queue.hpp"
#include "ylt/easylog.hpp"
#include "nd_buffer.hpp"
#include "nd_device.hpp"
#include "nd_mr.hpp"

namespace coro_io {

class nd_buffer_pool_t;

namespace detail {
struct nd_buffer_mem_control_t {
  std::atomic<std::size_t> now_usage = 0;
  std::atomic<std::size_t> history_max_usage = 0;
};
}  // namespace detail

class nd_host_memory_t {
 public:
  nd_host_memory_t() = default;
  explicit nd_host_memory_t(std::size_t size)
      : data_(std::make_unique<char[]>(size)), size_(size) {}

  void* data() const noexcept { return data_.get(); }
  std::size_t size() const noexcept { return size_; }

 private:
  std::unique_ptr<char[]> data_;
  std::size_t size_ = 0;
};

// A pooled, registered buffer: owns a chunk of host memory plus its
// nd_memory_region. Hands out value-semantic coro_io::{mutable,const}_buffer
// views (lkey embedded). Returns itself to the owning pool on destruction.
// Mirrors ib_buffer_t (CPU-only -- ND has no GPU path here).
struct nd_buffer_t {
 private:
  std::weak_ptr<nd_buffer_pool_t> owner_pool_;
  nd_host_memory_t memory_;
  std::unique_ptr<nd_memory_region> mr_;
  std::shared_ptr<detail::nd_buffer_mem_control_t> memory_usage_recorder_;
  std::size_t accounted_size_ = 0;

  inline void release_resource();

 public:
  nd_buffer_t() = default;
  nd_buffer_t(std::unique_ptr<nd_memory_region> mr, nd_host_memory_t memory,
              nd_buffer_pool_t& owner_pool,
              std::shared_ptr<detail::nd_buffer_mem_control_t>
                  memory_usage_recorder,
              std::size_t accounted_size) noexcept;
  friend class nd_buffer_pool_t;

  nd_buffer_t(nd_buffer_t&&) = default;
  inline nd_buffer_t& operator=(nd_buffer_t&& o);
  inline ~nd_buffer_t();

  operator bool() const noexcept { return mr_ != nullptr; }

  void* data() const noexcept { return memory_.data(); }
  std::size_t size() const noexcept { return memory_.size(); }
  std::uint32_t local_key() const { return mr_->local_key(); }

  // A registered view over [start, start+length) of this buffer (lkey carried).
  mutable_buffer mutable_view(std::size_t start, std::size_t length) const {
    return mr_->slice(start, length);
  }
  const_buffer const_view(std::size_t start, std::size_t length) const {
    return mr_->cslice(start, length);
  }
  // Whole-buffer mutable view.
  mutable_buffer mutable_view() const {
    return mr_->slice(std::size_t{0}, size());
  }
};

// Pool of registered buffers bound to one ND device. Mirrors ib_buffer_pool_t
// and stays CPU only.
class nd_buffer_pool_t : public std::enable_shared_from_this<nd_buffer_pool_t> {
 public:
  using nd_buffer_mem_control_t = detail::nd_buffer_mem_control_t;

  struct config_t {
    std::size_t buffer_size = 256 * 1024;  // 256KB
    std::uint64_t max_memory_usage = UINT32_MAX;
    // Matches ib_buffer_pool_t: nullptr shares a global recorder, so the
    // default max_memory_usage is also a global ND memory limit.
    std::shared_ptr<nd_buffer_mem_control_t> memory_usage_recorder = nullptr;
    std::chrono::milliseconds idle_timeout = std::chrono::milliseconds{5000};
  };

 private:
  static std::shared_ptr<nd_buffer_mem_control_t> g_memory_usage_recorder() {
    static auto used_memory = std::make_shared<nd_buffer_mem_control_t>();
    return used_memory;
  }

  struct private_construct_token {};

  struct nd_buffer_impl_t {
    nd_host_memory_t memory_;
    std::unique_ptr<nd_memory_region> mr_;
    std::shared_ptr<nd_buffer_mem_control_t> memory_usage_recorder_;
    std::size_t accounted_size_ = 0;

    nd_buffer_t convert_to_buffer(nd_buffer_pool_t& pool) && {
      auto accounted_size = accounted_size_;
      accounted_size_ = 0;
      return nd_buffer_t{std::move(mr_), std::move(memory_), pool,
                         std::move(memory_usage_recorder_), accounted_size};
    }
    nd_buffer_impl_t& operator=(nd_buffer_impl_t&& o) noexcept = default;
    nd_buffer_impl_t(nd_buffer_impl_t&& o) noexcept = default;
    nd_buffer_impl_t() noexcept = default;
    nd_buffer_impl_t(nd_host_memory_t&& memory,
                     std::unique_ptr<nd_memory_region>&& mr,
                     std::shared_ptr<nd_buffer_mem_control_t> recorder,
                     std::size_t accounted_size) noexcept
        : memory_(std::move(memory)),
          mr_(std::move(mr)),
          memory_usage_recorder_(std::move(recorder)),
          accounted_size_(accounted_size) {}
  };

  static async_simple::coro::Lazy<void> collect_idle_timeout_client(
      std::weak_ptr<nd_buffer_pool_t> self_weak,
      std::chrono::milliseconds sleep_time) {
    std::shared_ptr<nd_buffer_pool_t> self = self_weak.lock();
    if (self == nullptr) {
      co_return;
    }
    while (true) {
      self->free_buffers_.reselect();
      self = nullptr;
      co_await coro_io::sleep_for(sleep_time);
      if ((self = self_weak.lock()) == nullptr) {
        break;
      }
      while (true) {
        ELOG_TRACE << "start nd_buffer timeout free of pool{" << self.get()
                   << "}, now nd_buffer count: " << self->free_buffers_.size();
        auto [is_all_cleared, clear_cnt] = self->free_buffers_.clear_old(1000);
        self->modify_memory_usage(
            -1 * static_cast<std::int64_t>(clear_cnt) *
            static_cast<std::int64_t>(self->buffer_size()));
        ELOG_INFO << "finish nd_buffer timeout free of pool{" << self.get()
                  << "}, now nd_buffer cnt: " << self->free_buffers_.size()
                  << " mem usage:"
                  << static_cast<std::int64_t>(self->memory_usage() /
                                               (1024 * 1024))
                  << " MB";
        if (!is_all_cleared) {
          try {
            co_await async_simple::coro::Yield{};
          } catch (std::exception& e) {
            ELOG_ERROR << "unexpected yield exception: " << e.what();
          }
        }
        else {
          break;
        }
      }
    }
    co_return;
  }

  void enqueue(nd_buffer_t& buffer) {
    auto accounting_recorder = buffer.memory_usage_recorder_;
    auto accounting_size = buffer.accounted_size_;
    auto impl = std::make_unique<nd_buffer_impl_t>(
        std::move(buffer.memory_), std::move(buffer.mr_),
        std::move(buffer.memory_usage_recorder_), buffer.accounted_size_);
    buffer.accounted_size_ = 0;
    auto enqueue_count = free_buffers_.enqueue(std::move(impl));
    if (enqueue_count == 0) {
      release_accounting(accounting_recorder, accounting_size);
      return;
    }
    if (enqueue_count == 1) {
      std::size_t expected = 0;
      if (free_buffers_.collecter_cnt_.compare_exchange_strong(expected, 1))
          [[unlikely]] {
        ELOG_TRACE << "start timeout collector of nd_buffer_pool{" << this
                   << "}";
        collect_idle_timeout_client(
            this->weak_from_this(),
            (std::max)(pool_config_.idle_timeout,
                       std::chrono::milliseconds{50}))
            .directlyStart(
                [](auto&&) {
                },
                coro_io::get_global_executor());
      }
    }
  }

 public:
  friend struct nd_buffer_t;

  nd_buffer_pool_t(private_construct_token, nd_device_ptr device,
                   const config_t& config)
      : device_(std::move(device)),
        memory_usage_recorder_(config.memory_usage_recorder
                                   ? config.memory_usage_recorder
                                   : g_memory_usage_recorder()),
        pool_config_(config) {}

  static std::shared_ptr<nd_buffer_pool_t> create(nd_device_ptr device,
                                                    const config_t& config) {
    return std::make_shared<nd_buffer_pool_t>(private_construct_token{},
                                              std::move(device), config);
  }

  ~nd_buffer_pool_t() { drain_free_buffers(); }

  static std::size_t global_memory_usage() {
    return g_memory_usage_recorder()->now_usage.load(std::memory_order_acquire);
  }
  static std::size_t global_history_max_memory_usage() {
    return g_memory_usage_recorder()->history_max_usage.load(
        std::memory_order_acquire);
  }

  std::size_t buffer_size() const noexcept { return pool_config_.buffer_size; }
  const config_t& get_config() const noexcept { return pool_config_; }
  std::size_t memory_usage() const noexcept {
    return memory_usage_recorder_->now_usage.load(std::memory_order_relaxed);
  }
  std::size_t max_recorded_memory_usage() const noexcept {
    return memory_usage_recorder_->history_max_usage.load(
        std::memory_order_relaxed);
  }
  std::size_t free_buffer_size() const noexcept {
    return free_buffers_.size();
  }
  std::size_t max_memory_usage() const noexcept {
    return pool_config_.max_memory_usage;
  }

  bool memory_out_of_limit() const noexcept {
    return max_memory_usage() < buffer_size() + memory_usage();
  }

  static void modify_memory_usage(
      std::shared_ptr<nd_buffer_mem_control_t> const& memory_usage_recorder,
      std::int64_t count) {
    if (!memory_usage_recorder) {
      return;
    }
    std::size_t new_usage = 0;
    if (count >= 0) {
      auto inc = static_cast<std::size_t>(count);
      auto old_usage =
          memory_usage_recorder->now_usage.fetch_add(
              inc, std::memory_order_release);
      new_usage = old_usage + inc;
    }
    else {
      auto dec = static_cast<std::size_t>(-count);
      auto old_usage = memory_usage_recorder->now_usage.load(
          std::memory_order_acquire);
      do {
        new_usage = old_usage > dec ? old_usage - dec : 0;
      } while (!memory_usage_recorder->now_usage.compare_exchange_weak(
          old_usage, new_usage, std::memory_order_acq_rel));
    }

    auto old_max = memory_usage_recorder->history_max_usage.load(
        std::memory_order_acquire);
    while (new_usage > old_max) {
      if (memory_usage_recorder->history_max_usage.compare_exchange_strong(
              old_max, new_usage, std::memory_order_acq_rel)) [[likely]] {
        break;
      }
    }
  }

  void modify_memory_usage(std::int64_t count) {
    modify_memory_usage(memory_usage_recorder_, count);
  }

  // Get a registered buffer (reused from the free list, or freshly allocated +
  // registered). Returns an empty buffer (operator bool == false) on failure.
  nd_buffer_t get_buffer() {
    std::unique_ptr<nd_buffer_impl_t> buffer;
    free_buffers_.try_dequeue(buffer);
    if (buffer) {
      return std::move(*buffer).convert_to_buffer(*this);
    }
    if (memory_out_of_limit()) [[unlikely]] {
      ELOG_WARN << "nd_buffer_pool memory out of limit";
      return nd_buffer_t{};
    }
    auto length = buffer_size();
    nd_host_memory_t memory{length};
    if (!memory.data()) {
      return nd_buffer_t{};
    }
    std::unique_ptr<nd_memory_region> mr;
    try {
      mr = std::make_unique<nd_memory_region>(device_, memory.data(), length);
    } catch (std::exception const& e) {
      ELOG_ERROR << "nd_buffer_pool regist mr failed: " << e.what();
      return nd_buffer_t{};
    }
    modify_memory_usage(static_cast<std::int64_t>(length));
    return nd_buffer_t{std::move(mr), std::move(memory), *this,
                       memory_usage_recorder_, length};
  }

 private:
  static void release_accounting(
      std::shared_ptr<nd_buffer_mem_control_t>& memory_usage_recorder,
      std::size_t& accounted_size) {
    if (accounted_size != 0) {
      modify_memory_usage(memory_usage_recorder,
                          -static_cast<std::int64_t>(accounted_size));
      accounted_size = 0;
    }
    memory_usage_recorder.reset();
  }

  void drain_free_buffers() {
    std::unique_ptr<nd_buffer_impl_t> buffer;
    while (free_buffers_.try_dequeue(buffer)) {
      if (buffer) {
        release_accounting(buffer->memory_usage_recorder_,
                           buffer->accounted_size_);
      }
      buffer.reset();
    }
  }

  void collect_free(nd_buffer_t& buffer) {
    if (buffer && !memory_out_of_limit()) {
      enqueue(buffer);
    }
  }

  nd_device_ptr device_;
  std::shared_ptr<nd_buffer_mem_control_t> memory_usage_recorder_;
  config_t pool_config_;
  coro_io::detail::client_queue<std::unique_ptr<nd_buffer_impl_t>>
      free_buffers_;
};

inline nd_buffer_t::nd_buffer_t(std::unique_ptr<nd_memory_region> mr,
                                 nd_host_memory_t memory,
                                 nd_buffer_pool_t& owner_pool,
                                 std::shared_ptr<detail::nd_buffer_mem_control_t>
                                     memory_usage_recorder,
                                 std::size_t accounted_size) noexcept
    : owner_pool_(owner_pool.weak_from_this()),
      memory_(std::move(memory)),
      mr_(std::move(mr)),
      memory_usage_recorder_(std::move(memory_usage_recorder)),
      accounted_size_(accounted_size) {}

inline nd_buffer_t& nd_buffer_t::operator=(nd_buffer_t&& o) {
  if (this != &o) {
    release_resource();
    owner_pool_ = std::move(o.owner_pool_);
    memory_ = std::move(o.memory_);
    mr_ = std::move(o.mr_);
    memory_usage_recorder_ = std::move(o.memory_usage_recorder_);
    accounted_size_ = o.accounted_size_;
    o.accounted_size_ = 0;
  }
  return *this;
}

inline nd_buffer_t::~nd_buffer_t() { release_resource(); }

inline void nd_buffer_t::release_resource() {
  if (mr_) {
    if (auto pool = owner_pool_.lock(); pool) {
      pool->collect_free(*this);
      if (mr_) [[unlikely]] {
        nd_buffer_pool_t::release_accounting(memory_usage_recorder_,
                                             accounted_size_);
      }
    }
    else {
      nd_buffer_pool_t::release_accounting(memory_usage_recorder_,
                                           accounted_size_);
    }
    // If the pool collected it, mr_/memory_ were moved out; otherwise they are
    // freed here by their own destructors.
  }
}

}  // namespace coro_io
