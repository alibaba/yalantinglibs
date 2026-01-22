#pragma once
#include <cstddef>
struct memory_owner_t {
  char* memory_ = nullptr;
  std::size_t len_ = 0;  // Add member to store the length
  int gpu_id_ = -1;
  memory_owner_t() noexcept = default;
  memory_owner_t(memory_owner_t&& o) noexcept
      : memory_(o.memory_), len_(o.len_), gpu_id_(o.gpu_id_) {
    o.memory_ = nullptr;
    o.len_ = 0;
  }
  memory_owner_t& operator=(memory_owner_t&& o) noexcept {
    if (this != &o) {  // Self-assignment protection
      // Free existing memory first
      if (memory_) {
        if (gpu_id_ >= 0) {
#ifdef YLT_ENABLE_CUDA
          ylt::cuda_wrapper::cudaFree(memory_);
#endif
        }
        else {
          delete[] memory_;
        }
      }

      memory_ = o.memory_;
      len_ = o.len_;
      gpu_id_ = o.gpu_id_;
      o.memory_ = nullptr;
      o.len_ = 0;
    }
    return *this;
  }

  memory_owner_t(std::size_t len, int gpu_id) : len_(len), gpu_id_(gpu_id) {
    if (gpu_id >= 0) {
#ifdef YLT_ENABLE_CUDA
      if (len) {
        ylt::cuda_wrapper::cuda_set_device(gpu_id);
        ylt::cuda_wrapper::cudaMalloc(&memory_, len);  // Fixed variable name
      }
#endif
    }
    else {
      memory_ = new char[len];
    }
  }
  void* get() const { return memory_; }
  void* data() const { return memory_; }
  std::size_t size() const { return len_; }
  ~memory_owner_t() {
    if (memory_) {
      if (gpu_id_ >= 0) {
#ifdef YLT_ENABLE_CUDA
        ylt::cuda_wrapper::cudaFree(memory_);
#endif
      }
      else {
        delete[] memory_;
      }
    }
  }
};