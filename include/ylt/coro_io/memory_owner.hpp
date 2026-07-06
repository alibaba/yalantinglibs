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
#include <cstddef>
#include <memory>
#ifdef YLT_ENABLE_CUDA
#include "ylt/coro_io/cuda/cuda_device.hpp"
#include "ylt/coro_io/cuda/cuda_memory.hpp"
#include "ylt/coro_io/cuda/cuda_stream.hpp"
#endif

struct memory_owner_t {
  char* memory_ = nullptr;
  std::size_t len_ = 0;
  std::shared_ptr<void> device_;

  memory_owner_t() noexcept = default;

  memory_owner_t(memory_owner_t&& o) noexcept
      : memory_(o.memory_), len_(o.len_), device_(std::move(o.device_)) {
    o.memory_ = nullptr;
    o.len_ = 0;
  }

  memory_owner_t& operator=(memory_owner_t&& o) noexcept {
    if (this != &o) {  // Self-assignment protection
      // Free existing memory first
      if (memory_) {
        if (device_) {
#ifdef YLT_ENABLE_CUDA
          coro_io::cuda_free(memory_, *(coro_io::cuda_device_t*)device_.get());
#endif
        }
        else {
          delete[] memory_;
        }
      }

      memory_ = o.memory_;
      len_ = o.len_;
      device_ = std::move(o.device_);
      o.memory_ = nullptr;
      o.len_ = 0;
    }
    return *this;
  }

  memory_owner_t(std::size_t len, int gpu_id) : len_(len) {
    if (len) {
      if (gpu_id >= 0) {
#ifdef YLT_ENABLE_CUDA
        constexpr int GPU_PAGE_SIZE = 64 * 1024;
        // The real alloc size should align to GPU_PAGE_SIZE for GDR
        auto alloc_size = (len + GPU_PAGE_SIZE - 1) & ~(GPU_PAGE_SIZE - 1);
        device_ = coro_io::cuda_device_t::get_cuda_device(gpu_id);
        memory_ = (char*)coro_io::cuda_malloc(
            alloc_size, *(coro_io::cuda_device_t*)device_.get(), true);
#endif
      }
      else {
        memory_ = new char[len];
      }
    }
  }

  void* get() const { return memory_; }
  void* data() const { return memory_; }
  std::size_t size() const { return len_; }
  int gpu_id() const noexcept {
#ifdef YLT_ENABLE_CUDA
    return device_ ? ((coro_io::cuda_device_t*)device_.get())->get_gpu_id()
                   : -1;
#else
    return -1;
#endif
  }

  ~memory_owner_t() {
    if (memory_) {
      if (device_) {
#ifdef YLT_ENABLE_CUDA
        coro_io::cuda_free(memory_, *(coro_io::cuda_device_t*)device_.get());
#endif
      }
      else {
        delete[] memory_;
      }
    }
  }
};
