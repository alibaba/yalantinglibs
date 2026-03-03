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
#include <cstdint>
#include <span>
#include <string_view>

namespace coro_io {

/**
 * @brief Represents a view of memory data with address, size and GPU
 * information Inherits from std::string_view and adds GPU ID functionality
 */
class data_view : public std::string_view {
 public:
  // Default constructor
  data_view() : std::string_view(), gpu_id_(-1) {}

  // Constructor with string_view
  data_view(std::string_view sv, int gpu_id)
      : std::string_view(sv), gpu_id_(gpu_id) {}

  // Constructor with span
  data_view(std::span<char> sv, int gpu_id)
      : std::string_view(sv.data(), sv.size()), gpu_id_(gpu_id) {}

  // Constructor with pointer, size and gpu_id (for void* compatibility)
  data_view(const void* ptr, std::size_t size, int gpu_id)
      : std::string_view(static_cast<const char*>(ptr), size),
        gpu_id_(gpu_id) {}

  // Get the GPU ID (-1 for CPU memory, >=0 for GPU memory)
  int gpu_id() const noexcept { return gpu_id_; }

  // Check if this is GPU memory
  bool is_gpu_memory() const noexcept { return gpu_id_ >= 0; }

  // Check if this is CPU memory
  bool is_cpu_memory() const noexcept { return gpu_id_ == -1; }

  // Set GPU ID
  void set_gpu_id(int gpu_id) noexcept { gpu_id_ = gpu_id; }

  // Get mutable pointer to the data (if available)
  char* mutable_data() noexcept {
    // Note: This is potentially unsafe if original data was const
    return const_cast<char*>(this->data());
  }

  // Conversion operator to std::span<char>
  explicit operator std::span<char>() const {
    return std::span<char>(const_cast<char*>(this->data()), this->size());
  }

 private:
  int gpu_id_;  // GPU ID (-1 for CPU memory, >=0 for GPU memory)
};
}  // namespace coro_io