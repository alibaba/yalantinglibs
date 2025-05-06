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
#include <type_traits>
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
  auto soc = std::make_unique<asio::ip::tcp::socket>(ib_socket.get_executor());
  auto ec = co_await async_io<std::error_code>(
      [&](auto cb) {
        acceptor.async_accept(*soc, cb);
      },
      acceptor);

  if (ec) [[unlikely]] {
    co_return std::move(ec);
  }
  // TODO: SSL?
  auto ret = co_await ib_socket.accept(std::move(soc));
  ELOGV(INFO, "accept over:%s", ret.message().data());
  co_return ret;
}

inline async_simple::coro::Lazy<std::error_code> async_connect(
    coro_io::ib_socket_t& ib_socket, const std::string& host,
    const std::string& port) {
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
    if (ib_socket.get_config().enable_zero_copy_recv_unknown_size_data &&
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
        ib_socket.get_config().enable_zero_copy_recv_unknown_size_data) {
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
      if (io_size < result.second) {
        ib_socket.set_read_buffer_len(io_size, result.second - io_size);
      }
    }
    else if (ib_socket.get_config().enable_zero_copy_recv_unknown_size_data) {
      if (io_size < result.second) {
        ib_socket.set_read_buffer_len(0, result.second - io_size);
      }
    }
  }
  co_return std::pair{result.first, std::min(result.second, io_size)};
}

template <typename T>
void make_sge_impl(std::vector<ibv_sge>& sge, std::span<T> buffers) {
  constexpr bool is_ibv_sge = requires { buffers.begin()->lkey; };
  sge.reserve(buffers.size());
  for (auto& buffer : buffers) {
    if constexpr (is_ibv_sge) {
      if (buffer.length == 0) [[unlikely]] {
        continue;
      }
      sge.push_back(ibv_sge{buffer.addr, buffer.length, buffer.lkey});
    }
    else {
      if (buffer.size() == 0) [[unlikely]] {
        continue;
      }
      for (std::size_t i = 0; i < buffer.size(); i += UINT32_MAX) {
        sge.push_back(ibv_sge{(uintptr_t)buffer.data() + i,
                              std::min<uint32_t>(buffer.size() - i, UINT32_MAX),
                              0});
      }
    }
  }
}

template <typename T>
inline void make_sge(std::vector<ibv_sge>& sge, T& buffer) {
  using pointer_t = decltype(buffer.data());
  if constexpr (std::is_same_v<pointer_t, void*> ||
                std::is_same_v<pointer_t, char*> ||
                std::is_same_v<pointer_t, const char*>) {
    make_sge_impl(sge, std::span{&buffer, 1});
  }
  else {
    // multiple buffers
    make_sge_impl(sge, std::span<typename T::value_type>{buffer});
  }
}

inline void reset_buffer(std::vector<ibv_sge>& buffer, std::size_t read_size) {
  if (read_size) {
    for (auto& e : buffer) {
      if (e.length <= read_size) {
        read_size -= e.length;
        e.length = 0;
      }
      else {
        e.addr += read_size;
        e.length -= read_size;
        break;
      }
    }
    auto it = std::find_if(buffer.begin(), buffer.end(), [](const ibv_sge& x) {
      return x.length != 0;
    });
    buffer.erase(buffer.begin(), it);
  }
}

template <coro_io::ib_socket_t::io_type io, typename Buffer>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_io_split(coro_io::ib_socket_t& ib_socket, Buffer&& raw_buffer,
               bool read_some = false) {
  std::vector<ibv_sge> sge_list;
  make_sge(sge_list, raw_buffer);
  std::span<ibv_sge> buffer = sge_list;
  if (buffer.size() == 0) [[unlikely]] {
    co_return std::pair{std::error_code{}, std::size_t{0}};
  }

  std::vector<ibv_sge> split_sge_block;
  split_sge_block.reserve(buffer.size());
  uint32_t max_size = ib_socket.get_buffer_size();
  std::size_t io_completed_size = consume_buffer(ib_socket, buffer);
  if (buffer.empty()) {
    co_return std::pair{std::error_code{}, io_completed_size};
  }
  std::size_t block_size;
  uint32_t now_split_size = 0;
  for (auto& sge : buffer) {
    for (std::size_t i = 0; i < sge.length; i += block_size) {
      if (io_completed_size > 0 && ib_socket.get_config().enable_zero_copy) {
        bool check;
        if constexpr (io == ib_socket_t::recv) {
          check =
              ib_socket.get_config().enable_zero_copy_recv_unknown_size_data;
        }
        else {
          check =
              ib_socket.get_config().enable_zero_copy_send_unknown_size_data;
        }
        if (!check) {
          max_size = std::max(max_size, ib_socket.get_max_zero_copy_size());
        }
      }
      block_size =
          std::min<uint32_t>(max_size - now_split_size, sge.length - i);
      if (split_sge_block.size() &&
          split_sge_block.back().addr + split_sge_block.back().length ==
              sge.addr + i &&
          split_sge_block.back().lkey == sge.lkey) {  // try combine iov
        split_sge_block.back().length += block_size;
      }
      else {
        split_sge_block.push_back(
            ibv_sge{sge.addr + i, (uint32_t)block_size, sge.lkey});
      }
      now_split_size += block_size;
      if (now_split_size == max_size) {
        auto [ec, len] = co_await async_io_impl<io>(ib_socket, split_sge_block,
                                                    now_split_size);
        io_completed_size += len;
        ELOG_TRACE << "has completed size:" << io_completed_size;
        if (ec) {
          co_return std::pair{ec, io_completed_size};
        }
        else if (len == 0 || len > now_split_size) [[unlikely]] {
          ELOG_ERROR << "read size error, it shouldn't be:" << len;
          co_return std::pair{std::make_error_code(std::errc::io_error),
                              io_completed_size};
        }
        else if (read_some) {
          co_return std::pair{ec, io_completed_size};
        }
        if (len < now_split_size) [[unlikely]] {
          reset_buffer(split_sge_block, len);
        }
        else {
          split_sge_block.clear();
        }
        now_split_size -= len;
      }
    }
  }
  std::error_code ec;
  std::size_t len = 0;
  while (now_split_size > 0) {
    reset_buffer(split_sge_block, len);
    std::tie(ec, len) =
        co_await async_io_impl<io>(ib_socket, split_sge_block, now_split_size);
    io_completed_size += len;
    ELOG_TRACE << "has completed size:" << io_completed_size;
    if (ec) {
      co_return std::pair{ec, io_completed_size};
    }
    else if (len == 0 || len > now_split_size) [[unlikely]] {
      ELOG_ERROR << "read size error, it shouldn't be:" << len;
      co_return std::pair{std::make_error_code(std::errc::io_error),
                          io_completed_size};
    }
    else if (read_some) {
      co_return std::pair{ec, io_completed_size};
    }
    now_split_size -= len;
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
async_read_some(coro_io::ib_socket_t& ib_socket, buffer_t&& buffer) {
  auto result = co_await detail::async_io_split<ib_socket_t::recv>(
      ib_socket, std::forward<buffer_t>(buffer), true);
  co_return result;
}

}  // namespace coro_io