#pragma once
#include <cstddef>
#include "ylt/coro_io/cuda/cuda_device.hpp"
#include "ylt/coro_io/cuda/cuda_memory.hpp"
#include "ylt/coro_io/cuda/cuda_stream.hpp"

struct memory_owner_t {
  char* memory_ = nullptr;
  std::size_t len_ = 0;
  int gpu_id = -1;
  
  memory_owner_t() noexcept = default;
  
  memory_owner_t(memory_owner_t&& o) noexcept
      : memory_(o.memory_), len_(o.len_), gpu_id(o.gpu_id) {
    o.memory_ = nullptr;
    o.len_ = 0;
    o.gpu_id = -1;
  }
  
  memory_owner_t& operator=(memory_owner_t&& o) noexcept {
    if (this != &o) {  // Self-assignment protection
      // Free existing memory first
      if (memory_) {
        if (gpu_id >= 0) {
          coro_io::cuda_free(memory_,gpu_id);
        }
        else {
          delete[] memory_;
        }
      }

      memory_ = o.memory_;
      len_ = o.len_;
      gpu_id = o.gpu_id;
      o.memory_ = nullptr;
      o.len_ = 0;
      o.gpu_id = -1;
    }
    return *this;
  }

  memory_owner_t(std::size_t len, int gpu_id) 
      : len_(len), gpu_id(gpu_id) {
    if (len) {
      if (gpu_id >= 0) {
        constexpr int GPU_PAGE_SIZE = 64 * 1024;
        // The real alloc size should align to GPU_PAGE_SIZE for GDR
        auto alloc_size = (len + GPU_PAGE_SIZE -1) & ~(GPU_PAGE_SIZE - 1);
        memory_ = (char*)coro_io::cuda_malloc(alloc_size, gpu_id, true);
      }
      else {
        memory_ = new char[len];
      }
    }
  }
  
  void* get() const { return memory_; }
  void* data() const { return memory_; }
  std::size_t size() const { return len_; }
  
  ~memory_owner_t() {
    if (memory_) {
      if (gpu_id >= 0) {
        coro_io::cuda_free(memory_,gpu_id);
      }
      else {
        delete[] memory_;
      }
    }
  }
};