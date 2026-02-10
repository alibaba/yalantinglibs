#include <chrono>
#include <cstddef>
#include <thread>
#include <ylt/coro_io/cuda/cuda_memory.hpp>
#include <iostream>
#include "async_simple/Common.h"
#include "ylt/coro_io/cuda/cuda_device.hpp"
#include "ylt/coro_io/cuda/cuda_stream.hpp"
#include "ylt/easylog.hpp"

void sync_memtest() {
  ELOG_DEBUG << "test sync memtest";
  char data[1024],data2[1024];
  auto d_ptr = coro_io::cuda_malloc(sizeof(data));
  auto d_ptr2 = coro_io::cuda_malloc(sizeof(data));
  memset(data, 'A', sizeof(data));
  memset(data+sizeof(data)/2, 'B', sizeof(data)-sizeof(data)/2);
  coro_io::cuda_copy((void*)d_ptr, 0, data, -1, sizeof(data));
  coro_io::cuda_copy((void*)d_ptr2, 0, (void*)d_ptr, 0, sizeof(data));
  coro_io::cuda_copy(data2, -1, (void*)d_ptr2, 0, sizeof(data));
  async_simple::logicAssert(memcmp(data, data2, sizeof(data))==0,"gpu memcheck failed");
  coro_io::cuda_free(d_ptr);
  coro_io::cuda_free(d_ptr2);
}

void sync_memtest_p2p() {
  ELOG_DEBUG << "test sync memtest p2p";
  char data[1024],data2[1024];
  memset(data, 'A', sizeof(data));
  memset(data+sizeof(data)/2, 'B', sizeof(data)-sizeof(data)/2);
  int gpu_id = 
  coro_io::cuda_device_t::get_cuda_devices().size() - 1;
  if (gpu_id >= 1) {

    auto d_ptr = coro_io::cuda_malloc(sizeof(data));
    auto d_ptr1 = coro_io::cuda_malloc(sizeof(data), gpu_id);
    memset(data, 'A', sizeof(data));
    coro_io::cuda_copy((void*)d_ptr, 0, data, -1, sizeof(data));
    coro_io::cuda_copy((void*)d_ptr1, gpu_id, (void*)d_ptr, 0, sizeof(data));
    coro_io::cuda_copy(data2, -1, (void*)d_ptr1, gpu_id, sizeof(data));
    async_simple::logicAssert(memcmp(data, data2, sizeof(data))==0,"gpu memcheck failed");
    coro_io::cuda_free(d_ptr);
    coro_io::cuda_free(d_ptr1);
  }
}

void async_memtest() {
  ELOG_DEBUG << "test async memtest";
  char data[1024*256],data2[1024*256];
  memset(data, 'A', sizeof(data));
  memset(data+sizeof(data)/2, 'B', sizeof(data)-sizeof(data)/2);
  coro_io::cuda_stream_handler_t stream_handler{};
  auto ptr = coro_io::cuda_malloc_async(stream_handler, sizeof(data));
  auto ptr2 = coro_io::cuda_malloc_async(stream_handler, sizeof(data));
  coro_io::cuda_copy_async(stream_handler, (void*)ptr, 0, data, -1, sizeof(data));
  coro_io::cuda_copy_async(stream_handler, (void*)ptr2, 0, (void*)ptr, 0, sizeof(data));
  coro_io::cuda_copy_async(stream_handler, data2, -1, (void*)ptr2, 0, sizeof(data));
  YLT_CHECK_CUDA_ERR(stream_handler.record().get());
  async_simple::logicAssert(memcmp(data, data2, sizeof(data))==0,"gpu memcheck failed");
}

void async_memtest2() {
  ELOG_DEBUG << "test async memtest2";
  char data[1024*256],data2[1024*256];
  memset(data, 'A', sizeof(data));
  memset(data+sizeof(data)/2, 'B', sizeof(data)-sizeof(data)/2);
  coro_io::cuda_stream_handler_t stream_handler{};
  coro_io::cuda_stream_handler_t stream_handler2{};
  auto ptr = coro_io::cuda_malloc_async(stream_handler, sizeof(data)/2);
  auto ptr2 = coro_io::cuda_malloc_async(stream_handler, sizeof(data)/2);
  coro_io::cuda_copy_async(stream_handler, (void*)ptr, 0, data, -1, sizeof(data)/2);
  coro_io::cuda_copy_async(stream_handler2, (void*)ptr2, 0, data+sizeof(data)/2, -1, sizeof(data)/2);
  coro_io::cuda_copy_async(stream_handler, data2, -1, (void*)ptr, 0, sizeof(data)/2);
  coro_io::cuda_copy_async(stream_handler2, data2+sizeof(data2)/2, -1, (void*)ptr2, 0, sizeof(data)/2);
  auto record =stream_handler.record(), record2 = stream_handler2.record();
  YLT_CHECK_CUDA_ERR(stream_handler.record().get());
  YLT_CHECK_CUDA_ERR(stream_handler2.record().get());
  async_simple::logicAssert(memcmp(data, data2, sizeof(data))==0,"gpu memcheck failed");
}

void async_memtest_p2p() {
  ELOG_DEBUG << "test async memtest p2p";
  char data[256],data2[256];
  int gpu_id = 
  coro_io::cuda_device_t::get_cuda_devices().size() - 1; 
  if (gpu_id >= 1) {
    memset(data, 'A', sizeof(data));
    memset(data+sizeof(data)/2, 'B', sizeof(data)-sizeof(data)/2);
    coro_io::cuda_stream_handler_t stream_handler0{0};
    coro_io::cuda_stream_handler_t stream_handler1{1};
    auto ptr = coro_io::cuda_malloc_async(stream_handler0, sizeof(data));
    auto ptr2 = coro_io::cuda_malloc_async(stream_handler1, sizeof(data));
    coro_io::cuda_copy_async(stream_handler0, (void*)ptr, 0, data, -1, sizeof(data));
    coro_io::cuda_copy_async(stream_handler1, (void*)ptr2, 1, (void*)ptr, 0, sizeof(data));
    coro_io::cuda_copy_async(stream_handler1, data2, -1, (void*)ptr2, 1, sizeof(data));
    YLT_CHECK_CUDA_ERR(stream_handler0.record().get());
    YLT_CHECK_CUDA_ERR(stream_handler1.record().get());
    async_simple::logicAssert(memcmp(data, data2, sizeof(data))==0,"gpu memcheck failed");
  }
}

int main() {
  sync_memtest();
  sync_memtest_p2p();
  async_memtest();
  sync_memtest();
  sync_memtest_p2p();
  async_memtest();
  async_memtest2();
  async_memtest_p2p();
  ELOG_INFO << "finished!";

  return 0;
}