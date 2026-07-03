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

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

#include "ylt/coro_io/memory_owner.hpp"
#include "ylt/easylog.hpp"
#include "nd_buffer.hpp"
#include "nd_device.hpp"
#include "nd_mr.hpp"

namespace coro_io {

class nd_buffer_pool_t;

// A pooled, registered buffer: owns a chunk of host memory plus its
// nd_memory_region. Hands out value-semantic coro_io::{mutable,const}_buffer
// views (lkey embedded). Returns itself to the owning pool on destruction.
// Mirrors ib_buffer_t (CPU-only -- ND has no GPU path here).
struct nd_buffer_t {
 private:
  std::weak_ptr<nd_buffer_pool_t> owner_pool_;
  memory_owner_t memory_;
  std::unique_ptr<nd_memory_region> mr_;

  inline void release_resource();

 public:
  nd_buffer_t() = default;
  nd_buffer_t(std::unique_ptr<nd_memory_region> mr, memory_owner_t memory,
              nd_buffer_pool_t& owner_pool) noexcept;
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
  mutable_buffer mutable_view() const { return mr_->slice(std::size_t{0}, size()); }
};

// Pool of registered buffers bound to one ND device. Mirrors ib_buffer_pool_t
// but with a simple mutex-guarded free list (CPU only, no idle-timeout reclaim).
class nd_buffer_pool_t : public std::enable_shared_from_this<nd_buffer_pool_t> {
 public:
  struct config_t {
    std::size_t buffer_size = 256 * 1024;  // 256KB
    std::uint64_t max_memory_usage = UINT32_MAX;
  };

 private:
  struct private_construct_token {};

  struct nd_buffer_impl_t {
    memory_owner_t memory_;
    std::unique_ptr<nd_memory_region> mr_;
    nd_buffer_t convert_to_buffer(nd_buffer_pool_t& pool) && {
      return nd_buffer_t{std::move(mr_), std::move(memory_), pool};
    }
  };

  void enqueue(nd_buffer_t& buffer) {
    auto impl = std::make_unique<nd_buffer_impl_t>();
    impl->memory_ = std::move(buffer.memory_);
    impl->mr_ = std::move(buffer.mr_);
    std::lock_guard<std::mutex> lock(mutex_);
    free_buffers_.push_back(std::move(impl));
  }

 public:
  friend struct nd_buffer_t;

  nd_buffer_pool_t(private_construct_token, nd_device_ptr device,
                   const config_t& config)
      : device_(std::move(device)), pool_config_(config) {}

  static std::shared_ptr<nd_buffer_pool_t> create(nd_device_ptr device,
                                                   const config_t& config) {
    return std::make_shared<nd_buffer_pool_t>(private_construct_token{},
                                              std::move(device), config);
  }

  std::size_t buffer_size() const noexcept { return pool_config_.buffer_size; }
  const config_t& get_config() const noexcept { return pool_config_; }
  std::size_t memory_usage() const noexcept {
    return now_usage_.load(std::memory_order_relaxed);
  }
  std::size_t free_buffer_size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return free_buffers_.size();
  }

  bool memory_out_of_limit() const noexcept {
    return pool_config_.max_memory_usage < buffer_size() + memory_usage();
  }

  // Get a registered buffer (reused from the free list, or freshly allocated +
  // registered). Returns an empty buffer (operator bool == false) on failure.
  nd_buffer_t get_buffer() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!free_buffers_.empty()) {
        auto impl = std::move(free_buffers_.back());
        free_buffers_.pop_back();
        return std::move(*impl).convert_to_buffer(*this);
      }
    }
    if (memory_out_of_limit()) [[unlikely]] {
      ELOG_WARN << "nd_buffer_pool memory out of limit";
      return nd_buffer_t{};
    }
    auto length = buffer_size();
    memory_owner_t memory{length, -1};
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
    now_usage_.fetch_add(length, std::memory_order_relaxed);
    return nd_buffer_t{std::move(mr), std::move(memory), *this};
  }

 private:
  void collect_free(nd_buffer_t& buffer) {
    if (buffer && !memory_out_of_limit()) {
      enqueue(buffer);
    }
  }

  nd_device_ptr device_;
  config_t pool_config_;
  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<nd_buffer_impl_t>> free_buffers_;
  std::atomic<std::size_t> now_usage_{0};
};

inline nd_buffer_t::nd_buffer_t(std::unique_ptr<nd_memory_region> mr,
                                memory_owner_t memory,
                                nd_buffer_pool_t& owner_pool) noexcept
    : owner_pool_(owner_pool.weak_from_this()),
      memory_(std::move(memory)),
      mr_(std::move(mr)) {}

inline nd_buffer_t& nd_buffer_t::operator=(nd_buffer_t&& o) {
  if (this != &o) {
    release_resource();
    owner_pool_ = std::move(o.owner_pool_);
    memory_ = std::move(o.memory_);
    mr_ = std::move(o.mr_);
  }
  return *this;
}

inline nd_buffer_t::~nd_buffer_t() { release_resource(); }

inline void nd_buffer_t::release_resource() {
  if (mr_) {
    if (auto pool = owner_pool_.lock(); pool) {
      pool->collect_free(*this);
    }
    // If the pool collected it, mr_/memory_ were moved out; otherwise they are
    // freed here by their own destructors.
  }
}

}  // namespace coro_io
