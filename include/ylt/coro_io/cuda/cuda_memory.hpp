#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include "cuda_stream.hpp"
#include "ylt/coro_io/cuda/cuda_device.hpp"
#include "ylt/easylog.hpp"

namespace coro_io {

namespace detail {
  
struct time_guard {
  time_guard(const char* msg):tp(std::chrono::steady_clock::now()),msg(msg) {
  }
  ~time_guard() {
    ELOG_TRACE << "gpu operation " << msg <<" cost time: " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - tp).count() << "us";
  }
  const char* msg;
  std::chrono::steady_clock::time_point tp;
};
}

inline void cuda_copy(void* dst, int dst_gpu_id, void* src, int src_gpu_id,
               std::size_t len) {
  detail::time_guard guard("cuda_copy");
  if (len == 0)
    return;

  // 必须一端是 host (-1)，另一端是 device (>=0)
  if ((dst_gpu_id == -1) && (src_gpu_id == -1)) {
    memcpy(dst,src,len);
  }

  if (dst_gpu_id == -1 && src_gpu_id >= 0) {
    cuda_device_t::get_cuda_device(src_gpu_id)->set_context();
    // Device -> Host
    YLT_CHECK_CUDA_ERR(cuMemcpyDtoH(dst, reinterpret_cast<CUdeviceptr>(src), len));
  }
  else if (src_gpu_id == -1 && dst_gpu_id >= 0) {
    cuda_device_t::get_cuda_device(dst_gpu_id)->set_context();
    // Host -> Device
    YLT_CHECK_CUDA_ERR(cuMemcpyHtoD(reinterpret_cast<CUdeviceptr>(dst), src, len));
  }
  else {
    cuda_device_t::get_cuda_device(dst_gpu_id)->set_context();
    // Device -> Device
    if (src_gpu_id == dst_gpu_id) {
      YLT_CHECK_CUDA_ERR(cuMemcpyDtoD(reinterpret_cast<CUdeviceptr>(dst),reinterpret_cast<CUdeviceptr>(src),len));
    }
    else {
      if (!cuda_device_t::get_cuda_p2p_linkable(src_gpu_id, dst_gpu_id)) {
        std::string err_msg = "GPU device " + std::to_string(src_gpu_id) + " can't visit GPU device " + std::to_string(dst_gpu_id) + ", they are not linkable.";
        ELOG_ERROR << err_msg;
        throw std::runtime_error(err_msg);
      }
      YLT_CHECK_CUDA_ERR(cuMemcpyPeer(reinterpret_cast<CUdeviceptr>(dst), *cuda_device_t::get_cuda_device(dst_gpu_id), reinterpret_cast<CUdeviceptr>(src), *cuda_device_t::get_cuda_device(src_gpu_id), len));
    }
  }
}

// 模拟 cudaMalloc 的函数（Driver API 版）
inline CUdeviceptr cuda_malloc(size_t size, int gpu_id = 0, bool enable_gdr = false) {
  CUdeviceptr d_ptr;
  detail::time_guard guard("cuda_malloc");
  cuda_device_t::get_cuda_device(gpu_id)->set_context();
  YLT_CHECK_CUDA_ERR(cuMemAlloc(&d_ptr, size));
  if (enable_gdr) {
    bool enable = 1;
    cuPointerSetAttribute(&enable, CU_POINTER_ATTRIBUTE_SYNC_MEMOPS, d_ptr);
  }
  return d_ptr;
}

// 模拟 cudaFree
inline void cuda_free(void* d_ptr, int gpu_id = 0) {
  detail::time_guard guard("cuda_free");
  std::cout<<"cuda_free set ctx start"<<std::endl;
  cuda_device_t::get_cuda_device(gpu_id)->set_context();
  std::cout<<"cuda_free set ctx over"<<std::endl;
  YLT_CHECK_CUDA_ERR(cuMemFree((CUdeviceptr)d_ptr));
}


inline void cuda_free(void* d_ptr, cuda_device_t& dev) {
  detail::time_guard guard("cuda_free");
  dev.set_context();
  YLT_CHECK_CUDA_ERR(cuMemFree((CUdeviceptr)d_ptr));
}

inline void cuda_copy_async(cuda_stream_handler_t& stream, void* dst, int dst_gpu_id,
                     void* src, int src_gpu_id, std::size_t len) {
  ELOG_TRACE << "gpu operation cuda_copy_async, dst " << dst << " src " << src << " len " << len;
  detail::time_guard guard("cuda_copy_async");
  if (len == 0)
    return;

  if (dst_gpu_id == -1 && src_gpu_id == -1) {
    memcpy(dst, src, len);
    return;
  }

  int ctx_gpu_id = (dst_gpu_id != -1) ? dst_gpu_id : src_gpu_id;
  cuda_device_t::get_cuda_device(ctx_gpu_id)->set_context();

  CUstream cu_stream = stream.get_stream();

  if (dst_gpu_id == -1) {
    // D2H: src is device, dst is host (pinned)
    CUdeviceptr d_src = reinterpret_cast<CUdeviceptr>(src);
    cuMemcpyDtoHAsync(dst, d_src, len, cu_stream);
  }
  else if (src_gpu_id == -1) {
    // H2D: src is host (pinned), dst is device
    CUdeviceptr d_dst = reinterpret_cast<CUdeviceptr>(dst);
    cuMemcpyHtoDAsync(d_dst, src, len, cu_stream);
  }
  else {
    // D2D
    CUdeviceptr d_dst = reinterpret_cast<CUdeviceptr>(dst);
    CUdeviceptr d_src = reinterpret_cast<CUdeviceptr>(src);

    if (dst_gpu_id == src_gpu_id) {
      cuMemcpyDtoDAsync(d_dst, d_src, len, cu_stream);
    }
    else {
      cuMemcpyPeerAsync(d_dst, *cuda_device_t::get_cuda_device(ctx_gpu_id), d_src, *cuda_device_t::get_cuda_device(src_gpu_id), len, cu_stream);
    }
  }
}

inline CUdeviceptr cuda_malloc_async(cuda_stream_handler_t& stream, std::size_t len, bool enable_gdr = false) {
  detail::time_guard guard("cuda_malloc_async");
  if (len == 0)
    return (CUdeviceptr)nullptr;
  stream.get_device().set_context();
  CUdeviceptr d_ptr;
  cuMemAllocAsync(&d_ptr, len, stream.get_stream());
  bool enable = 1;
  if (enable_gdr) {
    cuPointerSetAttribute(&enable, CU_POINTER_ATTRIBUTE_SYNC_MEMOPS, d_ptr);
  }
  return d_ptr;
}

inline void cuda_free_async(cuda_stream_handler_t& stream, void* mem) {
  detail::time_guard guard("cuda_free_async");
  if (!mem)
    return;
  stream.get_device().set_context();
  cuMemFreeAsync(reinterpret_cast<CUdeviceptr>(mem), stream.get_stream());
}

// // allocate pinned host memory
// void* cuda_host_pin(std::size_t size) {
//   if (size == 0) {
//     return nullptr;
//   }

//   void* ptr = nullptr;
//   CUresult res = cuMemHostAlloc(
//       &ptr, size, CU_MEMHOSTALLOC_PORTABLE | CU_MEMHOSTALLOC_DEVICEMAP);

//   if (res != CUDA_SUCCESS) {
//     throw std::runtime_error("Failed to allocate pinned host memory");
//   }

//   return ptr;
// }

// // deallocate pinned host memory
// void cuda_host_unpin(void* ptr) {
//   if (ptr == nullptr) {
//     return;
//   }

//   CUresult res = cuMemFreeHost(ptr);
//   if (res != CUDA_SUCCESS) {
//     throw std::runtime_error("Failed to free pinned host memory");
//   }
// }

}  // namespace coro_io
