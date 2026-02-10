#pragma once

#include "cuda.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <span>

#include "async_simple/coro/ConditionVariable.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"


#include "ylt/coro_io/detail/client_queue.hpp"
#include "ylt/easylog.hpp"

#define YLT_CHECK_CUDA_ERR(err) \
    do { \
        if (err != CUDA_SUCCESS) { \
            const char *err_str; \
            cuGetErrorString(err, &err_str); \
            std::string tmp = "CUDA Driver error: " + std::string(err_str); \
            ELOG_ERROR << tmp; \
            throw std::runtime_error(tmp); \
        } \
    } while (0)

namespace coro_io {
  class cuda_device_t: public std::enable_shared_from_this<cuda_device_t> {
    public:
      static std::span<std::shared_ptr<cuda_device_t>> get_cuda_devices() {
        static std::vector<std::shared_ptr<cuda_device_t>> device = get_cuda_devices_impl();
        return device;
      }
      static cuda_device_t& get_cuda_device(int gpu_id) {
        static auto devices = get_cuda_devices();
        if (gpu_id>=devices.size() || gpu_id < 0) [[unlikely]] {
          throw std::logic_error("Out of cuda devices index");
        }
        return *devices[gpu_id];
      }

      static bool get_cuda_p2p_linkable(int src_gpu_id, int dst_gpu_id) {
        return get_cuda_p2p_topo()[src_gpu_id][dst_gpu_id];
      }

      operator CUcontext() const noexcept {
        return context_;
      }
    cuda_device_t(const cuda_device_t&)=delete;
    cuda_device_t(cuda_device_t&&)=delete;
    cuda_device_t& operator =(const cuda_device_t&)=delete;
    cuda_device_t& operator =(cuda_device_t&&)=delete;
    ~cuda_device_t() {
    }

    void close() {

    }

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
      size_t num_devices = devices.size();
      std::vector<std::vector<bool>> topo(
          num_devices, std::vector<bool>(num_devices, false));

      for (size_t i = 0; i < num_devices; ++i) {
        for (size_t j = 0; j < num_devices; ++j) {
          if (i == j) {
            topo[i][j] = true;  // A device can always access itself
            continue;
          }
          int canAccessPeer;
          YLT_CHECK_CUDA_ERR(cuDeviceCanAccessPeer(
              &canAccessPeer, devices[i]->device_, devices[j]->device_));
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
    cuda_device_t(int gpu_id) :gpu_id_(gpu_id){
      YLT_CHECK_CUDA_ERR(cuDeviceGet(&device_, gpu_id_));
      YLT_CHECK_CUDA_ERR(cuDevicePrimaryCtxRetain(&context_, device_));
      name_.resize(256);
      YLT_CHECK_CUDA_ERR(cuDeviceGetName(name_.data(), 256, device_));
      ELOG_INFO << "Get cuda device(" << gpu_id_ << "): " << name_;
    }
  private:
    std::string name_;
    int gpu_id_;
    CUcontext context_;
    CUdevice device_;

  };
}