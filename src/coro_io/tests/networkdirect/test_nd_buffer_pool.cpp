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
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <thread>

#include "asio/io_context.hpp"
#include "doctest.h"
#include "ylt/coro_io/networkdirect/nd.hpp"

using namespace std::chrono_literals;

namespace {

constexpr std::size_t kPoolBufferSize = 16 * 1024;

coro_io::nd_device_ptr try_get_device() {
  try {
    return coro_io::nd_device_manager_t::instance().get_first_available_device(
        {});
  } catch (const std::exception &e) {
    MESSAGE("no ND device available, skipping buffer pool test: " << e.what());
    return nullptr;
  }
}

bool wait_until(auto predicate, std::chrono::milliseconds timeout = 1s) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  do {
    if (predicate()) {
      return true;
    }
    std::this_thread::sleep_for(10ms);
  } while (std::chrono::steady_clock::now() < deadline);
  return predicate();
}

std::size_t current_usage(
    const std::shared_ptr<coro_io::nd_buffer_pool_t::nd_buffer_mem_control_t>&
        recorder) {
  return recorder->now_usage.load(std::memory_order_acquire);
}

}  // namespace

TEST_CASE("nd_buffer_pool memory statistics") {
  auto device = try_get_device();
  if (!device) {
    return;
  }

  auto usage_before = coro_io::nd_buffer_pool_t::global_memory_usage();
  auto max_before =
      coro_io::nd_buffer_pool_t::global_history_max_memory_usage();

  auto pool = coro_io::nd_buffer_pool_t::create(
      device, {.buffer_size = kPoolBufferSize,
               .max_memory_usage = usage_before + kPoolBufferSize * 4});

  CHECK(pool->memory_usage() == usage_before);
  CHECK(pool->max_recorded_memory_usage() == max_before);

  auto buffer = pool->get_buffer();
  REQUIRE(buffer);
  CHECK(pool->memory_usage() == usage_before + kPoolBufferSize);
  CHECK(pool->max_recorded_memory_usage() >= max_before);
  CHECK(pool->max_recorded_memory_usage() >= pool->memory_usage());
}

TEST_CASE("nd_buffer_pool reuses freed buffers") {
  auto device = try_get_device();
  if (!device) {
    return;
  }

  auto usage_before = coro_io::nd_buffer_pool_t::global_memory_usage();

  auto pool = coro_io::nd_buffer_pool_t::create(
      device, {.buffer_size = kPoolBufferSize,
               .max_memory_usage = usage_before + kPoolBufferSize * 4});

  void *first_ptr = nullptr;
  {
    auto buffer = pool->get_buffer();
    REQUIRE(buffer);
    first_ptr = buffer.data();
  }

  CHECK(pool->memory_usage() == usage_before + kPoolBufferSize);
  CHECK(pool->free_buffer_size() == 1);

  auto reused = pool->get_buffer();
  REQUIRE(reused);
  CHECK(reused.data() == first_ptr);
  CHECK(pool->memory_usage() == usage_before + kPoolBufferSize);
  CHECK(pool->free_buffer_size() == 0);
}

TEST_CASE("nd_buffer_pool idle reclaim updates memory usage") {
  auto device = try_get_device();
  if (!device) {
    return;
  }

  auto usage_before = coro_io::nd_buffer_pool_t::global_memory_usage();

  auto pool = coro_io::nd_buffer_pool_t::create(
      device, {.buffer_size = kPoolBufferSize,
               .max_memory_usage = usage_before + kPoolBufferSize * 4,
               .idle_timeout = 50ms});

  {
    auto buffer = pool->get_buffer();
    REQUIRE(buffer);
  }

  CHECK(pool->memory_usage() == usage_before + kPoolBufferSize);
  CHECK(pool->free_buffer_size() == 1);
  CHECK(wait_until([&] {
    return pool->memory_usage() == usage_before;
  }));
}

TEST_CASE("nd_buffer_pool release accounting when over limit") {
  auto device = try_get_device();
  if (!device) {
    return;
  }

  auto usage_before = coro_io::nd_buffer_pool_t::global_memory_usage();

  auto pool = coro_io::nd_buffer_pool_t::create(
      device, {.buffer_size = kPoolBufferSize,
               .max_memory_usage = usage_before + kPoolBufferSize});

  {
    auto buffer = pool->get_buffer();
    REQUIRE(buffer);
    CHECK(pool->memory_usage() == usage_before + kPoolBufferSize);
  }

  CHECK(pool->memory_usage() == usage_before);
  CHECK(pool->free_buffer_size() == 0);

  auto buffer = pool->get_buffer();
  REQUIRE(buffer);
  CHECK(pool->memory_usage() == usage_before + kPoolBufferSize);
}

TEST_CASE("nd_buffer_pool destructor releases cached buffer accounting") {
  auto device = try_get_device();
  if (!device) {
    return;
  }

  auto recorder =
      std::make_shared<coro_io::nd_buffer_pool_t::nd_buffer_mem_control_t>();
  {
    auto pool = coro_io::nd_buffer_pool_t::create(
        device, {.buffer_size = kPoolBufferSize,
                 .max_memory_usage = kPoolBufferSize * 4,
                 .memory_usage_recorder = recorder,
                 .idle_timeout = 1h});
    {
      auto buffer = pool->get_buffer();
      REQUIRE(buffer);
      CHECK(current_usage(recorder) == kPoolBufferSize);
    }

    CHECK(pool->free_buffer_size() == 1);
    CHECK(current_usage(recorder) == kPoolBufferSize);
  }

  CHECK(current_usage(recorder) == 0);
}

TEST_CASE("nd_buffer_pool accounting survives buffer outliving pool") {
  auto device = try_get_device();
  if (!device) {
    return;
  }

  auto recorder =
      std::make_shared<coro_io::nd_buffer_pool_t::nd_buffer_mem_control_t>();
  coro_io::nd_buffer_t buffer;
  {
    auto pool = coro_io::nd_buffer_pool_t::create(
        device, {.buffer_size = kPoolBufferSize,
                 .max_memory_usage = kPoolBufferSize * 4,
                 .memory_usage_recorder = recorder,
                 .idle_timeout = 1h});
    buffer = pool->get_buffer();
    REQUIRE(buffer);
    CHECK(current_usage(recorder) == kPoolBufferSize);
  }

  CHECK(current_usage(recorder) == kPoolBufferSize);
  buffer = coro_io::nd_buffer_t{};
  CHECK(current_usage(recorder) == 0);
}

TEST_CASE("nd_memory_region slice accepts base address") {
  auto device = try_get_device();
  if (!device) {
    return;
  }

  coro_io::nd_host_memory_t memory{kPoolBufferSize};
  coro_io::nd_memory_region mr(device, memory.data(), memory.size());

  auto mutable_view = mr.slice(memory.data(), 128);
  CHECK(mutable_view.data() == memory.data());
  CHECK(mutable_view.length() == 128);

  const auto& const_mr = mr;
  auto const_view = const_mr.slice(static_cast<void const*>(memory.data()), 128);
  CHECK(const_view.data() == memory.data());
  CHECK(const_view.length() == 128);
}

TEST_CASE("nd_socket forwards buffer pool config") {
  auto device = try_get_device();
  if (!device) {
    return;
  }

  auto recorder =
      std::make_shared<coro_io::nd_buffer_pool_t::nd_buffer_mem_control_t>();
  coro_io::nd_socket_t::config_t config{
      .buffer_size = kPoolBufferSize,
      .buffer_pool_config = {.buffer_size = kPoolBufferSize / 2,
                             .max_memory_usage = kPoolBufferSize,
                             .memory_usage_recorder = recorder,
                             .idle_timeout = 50ms},
      .device = device};

  auto executor = coro_io::get_global_executor();
  asio::error_code ec;
  coro_io::use_device(static_cast<asio::io_context &>(executor->context()),
                      device, {}, ec);
  if (ec && ec != coro_io::make_error_code(
                    coro_io::rdma_errc::already_registered)) {
    MESSAGE("use_device failed, skipping socket config test: " << ec.message());
    return;
  }

  coro_io::nd_socket_t socket(executor, config);
  auto pool = socket.buffer_pool();
  REQUIRE(pool);
  CHECK(socket.get_buffer_size() == kPoolBufferSize);
  CHECK(pool->buffer_size() == kPoolBufferSize);
  CHECK(pool->max_memory_usage() == kPoolBufferSize);
  CHECK(pool->get_config().memory_usage_recorder == recorder);
  CHECK(pool->get_config().idle_timeout == 50ms);

  {
    auto buffer = pool->get_buffer();
    REQUIRE(buffer);
    CHECK(pool->memory_usage() == kPoolBufferSize);
  }
  CHECK(pool->memory_usage() == 0);
}
