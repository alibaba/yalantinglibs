#include <infiniband/verbs.h>
#include <sys/socket.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <future>
#include <memory>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>

#include "asio/buffer.hpp"
#include "async_simple/Future.h"
#include "async_simple/Promise.h"
#include "async_simple/Signal.h"
#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "ib_buffer.hpp"
#include "ib_socket.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/ibverbs/ib_device.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/struct_pack.hpp"
#include "ylt/struct_pack/reflection.hpp"
namespace coro_io {

// unlike tcp socket, client won't connnected util server first read ib_socket.
inline async_simple::coro::Lazy<std::error_code> async_accept(
    asio::ip::tcp::acceptor& acceptor, coro_io::ib_socket_t& ib_socket) {
  asio::ip::tcp::socket soc{ib_socket.get_executor()};
  auto ec = co_await async_io<std::error_code>(
      [&](auto&& cb) {
        acceptor.async_accept(soc, std::move(cb));
      },
      acceptor);

  if (ec) [[unlikely]] {
    co_return std::move(ec);
  }
  // TODO: SSL?
  auto ret = co_await ib_socket.accept(soc);
  ELOGV(INFO, "accept over:%s", ret.message().data());
  co_return ret;
}

inline async_simple::coro::Lazy<std::error_code> async_connect(
    coro_io::ib_socket_t& ib_socket, const std::string& host,
    const std::string& port) {
  // todo: get socket from client_pool, not connect each time?

  asio::ip::tcp::socket soc{ib_socket.get_executor()};
  auto ec =
      co_await async_connect(ib_socket.get_coro_executor(), soc, host, port);
  if (ec) [[unlikely]] {
    co_return std::move(ec);
  }
  co_return co_await ib_socket.connect(soc);
}

namespace detail {

template <coro_io::ib_socket_t::io_type io>
void regist(coro_io::ib_socket_t& ib_socket, std::span<ibv_sge>& sge_buffer,
            std::vector<ib_buffer_t>& tmp_buffers,
            std::vector<ibv_sge>& tmp_sges, uint32_t io_size) {
  tmp_buffers.reserve(sge_buffer.size());
  tmp_sges.reserve(sge_buffer.size() + 1);
  for (auto& sge : sge_buffer) {
    tmp_sges.push_back(sge);
    if (sge.lkey == 0) {
      tmp_buffers.push_back(ib_buffer_t::regist(ib_socket.get_device(),
                                                (void*)sge.addr, sge.length));
      tmp_sges.back().lkey = tmp_buffers.back()->lkey;
    }
    else {
      ELOG_INFO << "already regist";
    }
    ELOG_INFO << "regist sge.lkey:" << tmp_sges.back().lkey
              << ",sge.addr:" << tmp_sges.back().addr
              << ",sge.length:" << tmp_sges.back().length;
  }
  if constexpr (io == ib_socket_t::recv) {
    if (ib_socket.get_config().enable_read_buffer_when_zero_copy &&
        io_size < ib_socket.get_buffer_size()) {
      auto buffer = ib_socket.get_buffer<io>();
      tmp_sges.push_back(ibv_sge{
          (uintptr_t)buffer.addr,
          (uint32_t)(ib_socket.get_buffer_size() - io_size), buffer.lkey});
    }
  }
  sge_buffer = tmp_sges;
}

template <typename T>
inline std::size_t consume_buffer(coro_io::ib_socket_t& ib_socket,
                                  std::span<T>& sge_buffer) {
  std::size_t transfer_total = 0;
  if (ib_socket.least_read_buffer_size()) {
    while (sge_buffer.size()) {
      auto length = ib_socket.consume((char*)sge_buffer.front().addr,
                                      sge_buffer.front().length);

      transfer_total += length;
      if (length < sge_buffer.front().length) {
        sge_buffer.front().addr = sge_buffer.front().addr + length;
        sge_buffer.front().length -= length;
        break;
      }
      sge_buffer = sge_buffer.subspan(1);
    }
  }
  return transfer_total;
}

inline std::size_t copy(std::span<ibv_sge> from, ibv_sge to) {
  std::size_t transfer_total = 0;
  for (auto& sge : from) {
    memcpy((void*)(to.addr + transfer_total), (void*)sge.addr, sge.length);
    transfer_total += sge.length;
  }
  return transfer_total;
}
inline void copy(ibv_sge from, std::span<ibv_sge> to) {
  std::size_t transfer_total = 0;
  for (auto& sge : to) {
    memcpy((void*)sge.addr, (void*)(from.addr + transfer_total), sge.length);
    transfer_total += sge.length;
  }
  return;
}

template <ib_socket_t::io_type io>
bool check_enable_zero_copy(coro_io::ib_socket_t& ib_socket,
                            std::size_t sge_size, std::size_t io_size) {
  bool is_enable_zero_copy = ib_socket.get_config().enable_zero_copy;
  if constexpr (io == ib_socket_t::recv) {
    if (is_enable_zero_copy &&
        ib_socket.get_config().enable_read_buffer_when_zero_copy) {
      if (io_size < ib_socket.get_buffer_size())
        sge_size += 1;
    }
  }
  if constexpr (io == ib_socket_t::recv) {
    if (sge_size > ib_socket.get_config().cap.max_recv_sge) {
      return false;
    }
  }
  else {
    if (sge_size > ib_socket.get_config().cap.max_send_sge) {
      return false;
    }
  }
  return is_enable_zero_copy;
}

template <ib_socket_t::io_type io>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_io_impl(
    coro_io::ib_socket_t& ib_socket, std::span<ibv_sge> sge_list,
    std::size_t io_size) {
  std::vector<ib_buffer_t> tmp_buffers;
  std::vector<ibv_sge> tmp_sges;
  ibv_sge socket_buffer;
  bool enable_zero_copy =
      check_enable_zero_copy<io>(ib_socket, sge_list.size(), io_size);
  if (!enable_zero_copy) {
    socket_buffer = ib_socket.get_buffer<io>();
  }
  else {
    regist<io>(ib_socket, sge_list, tmp_buffers, tmp_sges, io_size);
  }
  if constexpr (io == ib_socket_t::send) {
    if (!enable_zero_copy) {
      auto len = copy(sge_list, socket_buffer);
      assert(len == io_size);
    }
  }
  std::span<ibv_sge> io_buffer;
  if (!enable_zero_copy) {
    io_buffer = {&socket_buffer, 1};
    if constexpr (io == ib_socket_t::send)
      socket_buffer.length = io_size;
  }
  else {
    io_buffer = sge_list;
  }
  auto result =
      co_await coro_io::async_io<std::pair<std::error_code, std::size_t>>(
          [&](auto&& cb) {
            ib_socket.async_io<io>(io_buffer, std::move(cb));
          },
          ib_socket);
  if (result.first) [[unlikely]] {
    co_return std::pair{result.first, std::min(result.second, io_size)};
  }
  else if (result.second > ib_socket.get_buffer_size())
      [[unlikely]] {  // it should never happen
    co_return std::pair{std::make_error_code(std::errc::io_error), 0};
  }
  if constexpr (io == ib_socket_t::recv) {
    if (!enable_zero_copy) {
      socket_buffer.length = result.second;
      copy(socket_buffer, sge_list);
      if (socket_buffer.length > io_size) {
        ib_socket.set_read_buffer_len(socket_buffer.length - io_size);
      }
    }
    else if (ib_socket.get_config().enable_read_buffer_when_zero_copy) {
      if (io_size < result.second) {
        auto least_len = result.second - io_size;
        ib_socket.set_read_buffer_len(least_len);
      }
    }
  }
  co_return std::pair{result.first, std::min(result.second, io_size)};
}

template <typename T>
void make_sge(std::vector<ibv_sge>& sge, std::span<T> buffer) {
  constexpr bool is_ibv_sge = requires { buffer.begin()->lkey; };
  sge.resize(buffer.size());
  for (int i = 0; i < buffer.size(); ++i) {
    if constexpr (is_ibv_sge) {
      sge[i].addr = buffer[i].addr;
      sge[i].length = buffer[i].length;
      sge[i].lkey = buffer[i].lkey;
    }
    else {
      sge[i].addr = (uintptr_t)buffer[i].data();
      sge[i].length = buffer[i].size();
    }
  }
  return;
}

template <typename T>
void make_sge(std::vector<ibv_sge>& sge, T& buffer) {
  make_sge(sge, std::span<T>{&buffer, 1});
}

template <coro_io::ib_socket_t::io_type io, typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_io_split(coro_io::ib_socket_t& ib_socket, buffer_t raw_buffer,
               bool read_some = false) {
  std::vector<ibv_sge> buffer_vec0, split_sge_block;
  make_sge(buffer_vec0, raw_buffer);
  split_sge_block.reserve(buffer_vec0.size());
  std::span<ibv_sge> buffer = buffer_vec0;
  uint32_t max_size = ib_socket.get_buffer_size();
  std::size_t io_completed_size = 0;
  std::size_t sge_index = 0;
  std::size_t read_from_buffer_size = consume_buffer(ib_socket, buffer);
  io_completed_size = read_from_buffer_size;
  if (buffer.empty()) {
    co_return std::pair{std::error_code{}, io_completed_size};
  }
  std::size_t block_size;
  uint32_t now_split_size = 0;
  for (auto& sge : buffer) {
    for (std::size_t i = 0; i < sge.length; i += block_size) {
      block_size =
          std::min<uint32_t>(max_size - now_split_size, sge.length - i);
      split_sge_block.push_back(
          ibv_sge{sge.addr + i, (uint32_t)block_size, sge.lkey});
      now_split_size += block_size;
      if (now_split_size == max_size) {
        auto [ec, len] = co_await async_io_impl<io>(ib_socket, split_sge_block,
                                                    now_split_size);
        io_completed_size += len;
        if (ec || read_some) {
          co_return std::pair{ec, io_completed_size};
        }
        if (len < now_split_size) [[unlikely]] {
          // TODO: when read size smaller than want size, we should wait until
          // read over
          co_return std::pair{std::make_error_code(std::errc::protocol_error),
                              io_completed_size};
        }
        now_split_size = 0;
        split_sge_block.clear();
      }
    }
  }
  std::error_code ec;
  std::size_t len;
  if (now_split_size > 0) {
    std::tie(ec, len) =
        co_await async_io_impl<io>(ib_socket, split_sge_block, now_split_size);
    io_completed_size += len;
  }
  co_return std::pair{ec, io_completed_size};
}

}  // namespace detail

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_write(
    coro_io::ib_socket_t& ib_socket, buffer_t&& buffer) {
  return detail::async_io_split<ib_socket_t::send>(
      ib_socket, std::forward<buffer_t>(buffer));
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_read(
    coro_io::ib_socket_t& ib_socket, buffer_t&& buffer) {
  return detail::async_io_split<ib_socket_t::recv>(
      ib_socket, std::forward<buffer_t>(buffer));
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_read_some(coro_io::ib_socket_t& ib_socket, buffer_t buffer) {
  auto old = ib_socket.get_config().enable_read_buffer_when_zero_copy;
  ib_socket.get_config().enable_read_buffer_when_zero_copy = true;
  auto result = co_await detail::async_io_split<ib_socket_t::recv>(
      ib_socket, buffer, true);
  ib_socket.get_config().enable_read_buffer_when_zero_copy = old;
  co_return result;
}

}  // namespace coro_io