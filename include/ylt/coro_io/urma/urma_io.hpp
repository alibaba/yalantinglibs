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
#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include "async_simple/Promise.h"
#include "async_simple/util/move_only_function.h"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/urma/urma_benchmark_profile.hpp"
#include "ylt/coro_io/urma/urma_socket.hpp"

namespace coro_io {
namespace detail {

template <typename AsioBuffer, typename BufferSequence>
void make_urma_buffers(std::vector<AsioBuffer>& result,
                       BufferSequence& buffers) {
  auto first = asio::buffer_sequence_begin(buffers);
  auto last = asio::buffer_sequence_end(buffers);
  for (; first != last; ++first) result.emplace_back(*first);
}

struct urma_write_completion_state {
  std::queue<std::pair<std::error_code, std::size_t>> completions;
  async_simple::util::move_only_function<void()> resume_handler;

  void push(std::pair<std::error_code, std::size_t> result) {
    completions.push(result);
    if (resume_handler) {
      auto h = std::move(resume_handler);
      resume_handler = nullptr;
      h();
    }
  }
};

inline async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
wait_urma_write_completion(
    const std::shared_ptr<urma_write_completion_state>& state,
    urma_socket_t& socket) {
  // Suspend until event_loop polls the CQ and calls state->push.
  // Do NOT call socket.poll_completion_once() here - it would race with
  // event_loop's poll_completion on the same shared state (send_callbacks_,
  // recv_queue_, etc.), causing double-free / SIGBUS.
  while (state->completions.empty()) {
    callback_awaitor<void> awaitor;
    co_await awaitor.await_resume([&state](auto handler) {
      state->resume_handler = [handler]() mutable {
        handler.resume();
      };
    });
  }
  auto result = state->completions.front();
  state->completions.pop();
  co_return result;
}

template <typename Buffer>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_urma_read(urma_socket_t& socket, Buffer&& raw_buffer, bool read_some) {
  if (!socket.get_executor().running_in_this_thread())
    co_await coro_io::dispatch(socket.get_executor());

  std::vector<asio::mutable_buffer> buffers;
  make_urma_buffers(buffers, raw_buffer);
  std::size_t completed = 0;
  for (auto& buffer : buffers) {
    if (socket.remain_read_buffer_size()) {
      auto count =
          socket.consume(static_cast<char*>(buffer.data()), buffer.size());
      buffer += count;
      completed += count;
    }
    while (buffer.size()) {
      auto wait_begin = urma_benchmark_profile::enabled()
                            ? urma_benchmark_profile::now_ns()
                            : 0;
      auto [ec, length] =
          co_await async_io<std::pair<std::error_code, std::size_t>>(
              [&](auto&& callback) {
                socket.post_recv(std::move(callback));
              },
              socket);
      urma_benchmark_profile::record_since_with_size(
          urma_benchmark_profile::stage::urma_read_wait_completion, wait_begin,
          length);
      if (ec)
        co_return std::pair{ec, completed};
      auto recv = socket.get_recv_buffer();
      auto count = std::min<std::size_t>(length, buffer.size());
      auto copy_begin = urma_benchmark_profile::enabled()
                            ? urma_benchmark_profile::now_ns()
                            : 0;
      std::memcpy(buffer.data(), reinterpret_cast<void*>(recv.addr), count);
      urma_benchmark_profile::record_since_with_size(
          urma_benchmark_profile::stage::urma_read_copy, copy_begin, count);
      buffer += count;
      completed += count;
      socket.set_read_buffer_len(count, length - count);
      if (read_some)
        co_return std::pair{std::error_code{}, completed};
    }
  }
  co_return std::pair{std::error_code{}, completed};
}

inline async_simple::coro::Lazy<
    std::pair<std::error_code, std::vector<owned_data_view>>>
async_urma_read_views(urma_socket_t& socket, std::size_t size) {
  if (!socket.get_executor().running_in_this_thread())
    co_await coro_io::dispatch(socket.get_executor());

  std::vector<owned_data_view> views;
  std::size_t completed = 0;
  if (socket.remain_read_buffer_size()) {
    auto view = socket.detach_remain_data_view();
    if (!view.empty()) {
      completed += view.size();
      views.push_back(std::move(view));
    }
  }
  while (completed < size) {
    auto wait_begin = urma_benchmark_profile::enabled()
                          ? urma_benchmark_profile::now_ns()
                          : 0;
    auto [ec, length] =
        co_await async_io<std::pair<std::error_code, std::size_t>>(
            [&](auto&& callback) {
              socket.post_recv(std::move(callback));
            },
            socket);
    urma_benchmark_profile::record_since_with_size(
        urma_benchmark_profile::stage::urma_read_wait_completion, wait_begin,
        length);
    if (ec)
      co_return std::pair{ec, std::move(views)};
    if (completed + length > size) {
      ELOG_ERROR << "URMA read view received more data than requested: "
                 << "requested=" << size << ", completed=" << completed
                 << ", incoming=" << length;
      co_return std::pair{std::make_error_code(std::errc::protocol_error),
                          std::move(views)};
    }
    auto view_begin = urma_benchmark_profile::enabled()
                          ? urma_benchmark_profile::now_ns()
                          : 0;
    auto view = socket.detach_recv_buffer_view(length);
    if (!view.empty()) {
      completed += view.size();
      views.push_back(std::move(view));
    }
    urma_benchmark_profile::record_since_with_size(
        urma_benchmark_profile::stage::urma_read_view, view_begin, length);
  }
  co_return std::pair{std::error_code{}, std::move(views)};
}

}  // namespace detail

template <typename Buffer>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_write(
    urma_socket_t& socket, Buffer&& raw_buffer) {
  if (!socket.get_executor().running_in_this_thread())
    co_await coro_io::dispatch(socket.get_executor());

  std::vector<asio::const_buffer> buffers;
  detail::make_urma_buffers(buffers, raw_buffer);
  std::size_t total_size = 0;
  for (auto& item : buffers) total_size += item.size();
  std::size_t completed = 0;
  auto total_begin =
      urma_benchmark_profile::enabled() ? urma_benchmark_profile::now_ns() : 0;
  ELOG_DEBUG << "URMA async_write start: total_size=" << total_size
             << ", chunk_size=" << socket.get_buffer_size()
             << ", send_window=" << socket.get_send_window_size();
  auto state = std::make_shared<detail::urma_write_completion_state>();
  std::size_t in_flight = 0;
  auto post_next = [&]() -> std::pair<std::error_code, bool> {
    if (buffers.empty())
      return {{}, false};
    auto buffer = socket.get_send_buffer();
    if (!buffer)
      return {std::make_error_code(std::errc::no_buffer_space), false};
    std::size_t length = 0;
    auto copy_begin = urma_benchmark_profile::enabled()
                          ? urma_benchmark_profile::now_ns()
                          : 0;
    while (!buffers.empty() && length < socket.get_buffer_size()) {
      auto count = std::min<std::size_t>(buffers.front().size(),
                                         socket.get_buffer_size() - length);
      std::memcpy(static_cast<char*>(buffer.addr) + length,
                  buffers.front().data(), count);
      length += count;
      buffers.front() += count;
      if (buffers.front().size() == 0)
        buffers.erase(buffers.begin());
    }
    urma_benchmark_profile::record_since_with_size(
        urma_benchmark_profile::stage::urma_write_copy, copy_begin, length);
    auto post_begin = urma_benchmark_profile::enabled()
                          ? urma_benchmark_profile::now_ns()
                          : 0;
    socket.post_send(std::move(buffer), length,
                     [state](std::pair<std::error_code, std::size_t> result) {
                       state->push(result);
                     });
    urma_benchmark_profile::record_since_with_size(
        urma_benchmark_profile::stage::urma_post_send, post_begin, length);
    ++in_flight;
    return {{}, true};
  };

  const auto send_window =
      std::max<std::size_t>(socket.get_send_window_size(), 1);
  while (!buffers.empty() || in_flight != 0) {
    while (!buffers.empty() && in_flight < send_window) {
      auto [ec, posted] = post_next();
      if (ec) {
        if (in_flight == 0)
          co_return std::pair{ec, completed};
        break;
      }
      if (!posted)
        break;
    }
    if (in_flight == 0)
      continue;
    auto wait_begin = urma_benchmark_profile::enabled()
                          ? urma_benchmark_profile::now_ns()
                          : 0;
    auto result = co_await detail::wait_urma_write_completion(state, socket);
    urma_benchmark_profile::record_since_with_size(
        urma_benchmark_profile::stage::urma_wait_send_completion, wait_begin,
        result.second);
    --in_flight;
    if (result.first)
      co_return std::pair{result.first, completed};
    completed += result.second;
  }
  ELOG_DEBUG << "URMA async_write done: total_size=" << total_size
             << ", completed=" << completed;
  urma_benchmark_profile::record_since_with_size(
      urma_benchmark_profile::stage::urma_write_total, total_begin, total_size);
  co_return std::pair{std::error_code{}, completed};
}

template <typename Buffer>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_read(
    urma_socket_t& socket, Buffer&& buffer) {
  return detail::async_urma_read(socket, std::forward<Buffer>(buffer), false);
}

template <typename Buffer>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_read_some(urma_socket_t& socket, Buffer&& buffer) {
  return detail::async_urma_read(socket, std::forward<Buffer>(buffer), true);
}

}  // namespace coro_io
