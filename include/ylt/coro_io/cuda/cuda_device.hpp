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
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <vector>

#include "async_simple/coro/ConditionVariable.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"
#include "cuda.h"
#include "ylt/coro_io/detail/client_queue.hpp"
#include "ylt/easylog.hpp"

#define YLT_CHECK_CUDA_ERR(err)                                       \
  do {                                                                \
    if (err != CUDA_SUCCESS && err != CUDA_ERROR_DEINITIALIZED) {     \
      const char* err_str;                                            \
      cuGetErrorString(err, &err_str);                                \
      std::string tmp = "CUDA Driver error: " + std::string(err_str); \
      ELOG_ERROR << tmp;                                              \
      throw std::runtime_error(tmp);                                  \
    }                                                                 \
  } while (0)

namespace coro_io {
class cuda_device_t : public std::enable_shared_from_this<cuda_device_t> {
 public:
  static std::shared_ptr<std::vector<std::shared_ptr<cuda_device_t>>>
  get_cuda_devices() {
    static auto device =
        std::make_shared<std::vector<std::shared_ptr<cuda_device_t>>>(
            get_cuda_devices_impl());
    return device;
  }
  static std::shared_ptr<cuda_device_t> get_cuda_device(int gpu_id) {
    static auto devices = get_cuda_devices();
    if (gpu_id >= devices->size() || gpu_id < 0) [[unlikely]] {
      throw std::logic_error("Out of cuda devices index");
    }
    return (*devices)[gpu_id];
  }

  static bool get_cuda_p2p_linkable(int src_gpu_id, int dst_gpu_id) {
    return get_cuda_p2p_topo()[src_gpu_id][dst_gpu_id];
  }

  operator CUcontext() const noexcept { return context_; }
  cuda_device_t(const cuda_device_t&) = delete;
  cuda_device_t(cuda_device_t&&) = delete;
  cuda_device_t& operator=(const cuda_device_t&) = delete;
  cuda_device_t& operator=(cuda_device_t&&) = delete;
  ~cuda_device_t() {
    ELOG_INFO << "release cuda device:" << name_ << "(" << gpu_id_ << ")";
    cuDevicePrimaryCtxRelease(device_);
  }

  void close() {}

  void set_context() {
    static thread_local CUcontext ctx = nullptr;
    if (ctx != context_) {
      YLT_CHECK_CUDA_ERR(cuCtxSetCurrent(context_));
      ctx = context_;
    }
  }

 private:
  static std::vector<std::shared_ptr<cuda_device_t>> get_cuda_devices_impl() {
    YLT_CHECK_CUDA_ERR(cuInit(0));
    int device_count = 0;
    YLT_CHECK_CUDA_ERR(cuDeviceGetCount(&device_count));
    std::vector<std::shared_ptr<cuda_device_t>> devices;
    devices.reserve(device_count);
    for (int i = 0; i < device_count; ++i) {
      devices.emplace_back(std::make_shared<cuda_device_t>(i));
    }
    return devices;
  }
  static std::vector<std::vector<bool>> get_cuda_p2p_topo_impl() {
    auto devices = get_cuda_devices();
    size_t num_devices = devices->size();
    std::vector<std::vector<bool>> topo(num_devices,
                                        std::vector<bool>(num_devices, false));

    for (size_t i = 0; i < num_devices; ++i) {
      for (size_t j = 0; j < num_devices; ++j) {
        if (i == j) {
          topo[i][j] = true;  // A device can always access itself
          continue;
        }
        int canAccessPeer;
        YLT_CHECK_CUDA_ERR(cuDeviceCanAccessPeer(
            &canAccessPeer, (*devices)[i]->device_, (*devices)[j]->device_));
        topo[i][j] = static_cast<bool>(canAccessPeer);
      }
    }
    return topo;
  }
  static std::span<std::vector<bool>> get_cuda_p2p_topo() {
    static std::vector<std::vector<bool>> topo = get_cuda_p2p_topo_impl();
    return topo;
  }

 public:
  cuda_device_t(int gpu_id) : gpu_id_(gpu_id) {
    YLT_CHECK_CUDA_ERR(cuDeviceGet(&device_, gpu_id_));
    YLT_CHECK_CUDA_ERR(cuDevicePrimaryCtxRetain(&context_, device_));
    name_.resize(256);
    YLT_CHECK_CUDA_ERR(cuDeviceGetName(name_.data(), 256, device_));
    auto pos = name_.find_last_not_of('\0');
    if (pos != std::string::npos) {
      name_.erase(pos + 1);
    }
    else {
      name_.clear();
    }
    ELOG_INFO << "Get cuda device(" << gpu_id_ << "): " << name_;
  }
  int get_gpu_id() const noexcept { return gpu_id_; }
  std::string_view name() const noexcept { return name_; }

 private:
  std::string name_;
  int gpu_id_;
  CUcontext context_;
  CUdevice device_;
};
}  // namespace coro_io