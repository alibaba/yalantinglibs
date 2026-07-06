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

#include <cuda.h>
#include <nvcomp.h>
#include <nvcomp/crc32.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "ylt/coro_io/cuda/cuda_device.hpp"
#include "ylt/coro_io/cuda/cuda_memory.hpp"
#include "ylt/coro_io/cuda/cuda_stream.hpp"
#include "ylt/easylog.hpp"

namespace coro_io {

#define YLT_CHECK_NVCOMP_ERR(err)                                \
  do {                                                           \
    if (err != nvcompSuccess) {                                  \
      const char* err_str = nvcompGetStatusString(err);          \
      std::string tmp = "nvCOMP error: " + std::string(err_str); \
      ELOG_ERROR << tmp;                                         \
      throw std::runtime_error(tmp);                             \
    }                                                            \
  } while (0)

// Submit a CRC32 computation over a single device-resident buffer to @p stream.
// Results are written asynchronously to @p device_out (a device uint32_t).
// Caller must sync the stream (e.g. stream.record().get()) before reading
// *device_out. Default spec is CRC-32/PKZIP; pass nvcompCRC32_C for iSCSI etc.
inline void cuda_crc32_async(cuda_stream_handler_t& stream,
                             const void* device_data, std::size_t len,
                             uint32_t* device_out,
                             nvcompCRC32Spec_t spec = nvcompCRC32) {
  detail::time_guard guard("cuda_crc32_async");

  stream.get_device().set_context();
  CUstream s = stream.get_stream();

  // nvCOMP requires the chunk_ptrs / chunk_bytes arrays in device memory.
  // For the single-buffer case we stage a 16-byte scratch via stream-ordered
  // allocation; the free is queued after nvCOMP so it stays alive until read.
  constexpr std::size_t kScratchBytes = sizeof(void*) + sizeof(std::size_t);
  CUdeviceptr scratch;
  YLT_CHECK_CUDA_ERR(cuMemAllocAsync(&scratch, kScratchBytes, s));
  YLT_CHECK_CUDA_ERR(
      cuMemcpyHtoDAsync(scratch, &device_data, sizeof(void*), s));
  YLT_CHECK_CUDA_ERR(
      cuMemcpyHtoDAsync(scratch + sizeof(void*), &len, sizeof(std::size_t), s));

  // max_input_chunk_bytes given directly => host-only, no stream sync.
  nvcompCRC32KernelConf_t conf{};
  YLT_CHECK_NVCOMP_ERR(nvcompBatchedCRC32GetHeuristicConf(
      nvcompCRC32IgnoredInputChunkBytes, 1, &conf, len, nullptr));

  nvcompBatchedCRC32Opts_t opts{};
  opts.spec = spec;
  opts.kernel_conf = conf;

  YLT_CHECK_NVCOMP_ERR(nvcompBatchedCRC32Async(
      reinterpret_cast<const void* const*>(scratch),
      reinterpret_cast<const std::size_t*>(scratch + sizeof(void*)), 1,
      device_out, opts, nvcompCRC32OnlySegment, nullptr,
      reinterpret_cast<cudaStream_t>(s)));

  YLT_CHECK_CUDA_ERR(cuMemFreeAsync(scratch, s));
}

}  // namespace coro_io
