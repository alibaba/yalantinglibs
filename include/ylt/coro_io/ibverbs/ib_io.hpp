#include <infiniband/verbs.h>

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
            std::vector<std::unique_ptr<ibv_mr, ib_deleter>>& tmp_buffers,std::vector<ibv_sge>& tmp_sges,
            ib_buffer_t& buffer, uint32_t io_size) {
  tmp_buffers.reserve(sge_buffer.size());
  tmp_sges.reserve(sge_buffer.size());
  for (auto& sge : sge_buffer) {
    tmp_sges.push_back(sge);
    if (sge.lkey == 0) {
      // todo: flag too many
      int flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                  IBV_ACCESS_REMOTE_WRITE;
      tmp_buffers.push_back(std::unique_ptr<ibv_mr, ib_deleter>{ibv_reg_mr(
          ib_socket.get_device()->pd(), (void*)sge.addr, sge.length, flags)});
      tmp_sges.back().lkey=tmp_buffers.back()->lkey;
    }
    else {
      ELOG_INFO<<"already regist";
    }
    ELOG_INFO<<"regist sge.lkey:"<<tmp_sges.back().lkey<<",sge.addr:"<<tmp_sges.back().addr<<",sge.length:"<<tmp_sges.back().length;
  }
  if (io == ib_socket_t::recv && 
      ib_socket.get_config().enable_read_buffer_when_zero_copy &&
      io_size < ib_socket.get_buffer_size()) {
    
    buffer = ib_socket.buffer_pool().get_buffer();
    tmp_sges.push_back(
        ibv_sge{(uintptr_t)buffer->addr,
                ib_socket.get_buffer_size() - io_size, buffer->lkey});
  }
  sge_buffer=tmp_sges;
}

inline std::size_t consume_buffer(coro_io::ib_socket_t& ib_socket,
                                  std::span<ibv_sge>& sge_buffer) {
  std::size_t transfer_total = 0;
  if (ib_socket.least_buffer_size()) {
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
inline std::size_t copy(ibv_sge from, std::span<ibv_sge> to) {
  std::size_t transfer_total = 0;
  for (auto& sge : to) {
    memcpy((void*)sge.addr, (void*)(from.addr + transfer_total), sge.length);
    transfer_total += sge.length;
  }
  return transfer_total;
}

template <coro_io::ib_socket_t::io_type io>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_io_impl(
    coro_io::ib_socket_t& ib_socket, std::span<ibv_sge> sge_buffer,
    std::vector<std::unique_ptr<ibv_mr, ib_deleter>> tmp_buffers,
    ib_buffer_t buffer = {}) {
  std::pair<std::error_code, std::size_t> result{};
  if (!ib_socket.get_config().enable_zero_copy) {
    buffer = ib_socket.buffer_pool().get_buffer();
  }
  if constexpr (io == ib_socket_t::send) {
    if (!ib_socket.get_config().enable_zero_copy) {
      copy(sge_buffer, buffer.subview());
    }
  }
  if (!ib_socket.get_config().enable_zero_copy) {
    result = co_await coro_io::async_io<decltype(result)>(
        [&](auto&& cb) {
          ib_socket.async_io<io>(ibv_sge{buffer}, std::move(cb));
        },
        ib_socket);
  }
  else {
    result = co_await coro_io::async_io<decltype(result)>(
        [&](auto&& cb) {
          ib_socket.async_io<io>(sge_buffer, std::move(cb));
        },
        ib_socket);
  }

  if (result.first) [[unlikely]] {
    ib_socket.buffer_pool().collect_free(std::move(buffer));
    co_return result;
  }
  if constexpr (io == ib_socket_t::recv) {
    if (!ib_socket.get_config().enable_zero_copy) {
      ibv_sge sge = buffer.subview();
      sge.length = result.second;
      auto sz = copy(sge, sge_buffer);
      if (sge.length > sz) {
        ib_socket.set_read_buffer(std::move(buffer),
                                  {(char*)buffer->addr + sz, sge.length - sz});
      }
    }
    else if (ib_socket.get_config().enable_read_buffer_when_zero_copy) {
      auto least_len = ib_socket.get_buffer_size() - result.second;
      if (least_len) {
        ib_socket.set_read_buffer(std::move(buffer),
                                  {(char*)buffer->addr, least_len});
      }
    }
  }
  ib_socket.buffer_pool().collect_free(std::move(buffer));
  co_return result;
}

template <coro_io::ib_socket_t::io_type io>
async_simple::coro::Lazy<std::error_code> async_io_regist(
    coro_io::ib_socket_t& ib_socket, std::span<ibv_sge> sge_buffer,
    uint32_t io_size, std::size_t& total_size,
    std::unique_ptr<async_simple::Promise<std::error_code>>& p,
    bool read_some = false, bool is_first_call = false) {
  std::vector<std::unique_ptr<ibv_mr, ib_deleter>> tmp_buffers;
  std::size_t read_from_buffer_size = 0;
  if constexpr (io == ib_socket_t::recv) {
    if (total_size == 0) {
      read_from_buffer_size = consume_buffer(ib_socket, sge_buffer);
      io_size -= read_from_buffer_size;
      total_size = read_from_buffer_size;
      if (sge_buffer.empty()) {
        co_return std::error_code{};
      }
    }
  }
  ib_buffer_t read_buffer;
  std::vector<ibv_sge> tmp_sges;
  if (ib_socket.get_config().enable_zero_copy) {
    regist<io>(ib_socket, sge_buffer, tmp_buffers, tmp_sges,read_buffer, io_size);
  }
  
  if (p == nullptr) {
    auto [ec, len] = co_await async_io_impl<io>(
        ib_socket, sge_buffer, std::move(tmp_buffers), std::move(read_buffer));
    
    total_size += len;
    if (ec) [[unlikely]] {
      co_return ec;
    }
    else if (len<io_size && !read_some) [[unlikely]] {
      p->setValue(std::make_error_code(std::errc::protocol_error));
    }
  }
  else {
    if (!is_first_call) {
      auto ec = co_await p->getFuture();
      if (ec) [[unlikely]] {
        co_return ec;
      }
      else {
        p=std::make_unique<async_simple::Promise<std::error_code>>();
      }
    }
    
    async_io_impl<io>(ib_socket, sge_buffer, std::move(tmp_buffers),
                      std::move(read_buffer))
        .start([&, io_size, read_some](auto&& result) {
          try {
            total_size+=result.value().second;
            if (!result.value().first && result.value().second < io_size &&
                !read_some) [[unlikely]] {
              p->setValue(std::make_error_code(std::errc::protocol_error));
            }
            else {
              p->setValue(result.value().first);
            }
          } catch (...) {
            p->setException(std::current_exception());
          }
        });
  }
  co_return std::error_code{};
}

template <typename T>
uint32_t make_sge(std::span<ibv_sge> sge, std::span<T> buffer,
                  uint32_t max_once_size) {
  constexpr bool is_ibv_sge = requires { buffer.begin()->lkey; };
  std::size_t total = 0;
  for (int i = 0; i < buffer.size(); ++i) {
    if constexpr (is_ibv_sge) {
      sge[i].addr = buffer[i].addr;
      sge[i].length = buffer[i].length;
      sge[i].lkey = buffer[i].lkey;
    }
    else {
      sge[i].addr = buffer.size();
      sge[i].length = buffer.size();
    }
    if (sge[i].length + total > max_once_size) {
      sge[i].length = max_once_size - total;
      total = max_once_size;
      break;
    }
    else {
      total += sge[i].length;
    }
  }
  return total;
}

template <coro_io::ib_socket_t::io_type io, typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_io_split(coro_io::ib_socket_t& ib_socket, buffer_t buffers,
               std::span<ibv_sge> sge_buffer) {
  uint32_t max_size = ib_socket.get_buffer_size();
  std::size_t total_size = 0;
  uint32_t now_size = 0;
  std::size_t sge_index = 0;
  using promise_t = async_simple::Promise<std::error_code>;
  std::unique_ptr<promise_t> p;
  constexpr bool is_ibv_sge = requires { buffers.begin()->lkey; };
  for (auto& buffer : buffers) {
    uint32_t block_size;
    uintptr_t data;
    std::size_t size;
    if constexpr (is_ibv_sge) {
      data = buffer.addr;
    }
    else {
      data = (uintptr_t)buffer.data();
    }
    if constexpr (is_ibv_sge) {
      size = buffer.length;
    }
    else {
      size = buffer.size();
    }
    for (std::size_t i = 0; i < size; i += block_size) {
      block_size = std::min<uint32_t>(max_size - now_size, size - i);
      sge_buffer[sge_index].addr = data + i;
      sge_buffer[sge_index].length = block_size;
      if constexpr (is_ibv_sge) {
        sge_buffer[sge_index].lkey = buffer.lkey;
      }
      sge_index++;
      now_size += block_size;
      if (now_size == max_size) {
        if (p == nullptr) {
          p = std::make_unique<promise_t>();
        }
        auto ec = co_await async_io_regist<io>(ib_socket,
                                               sge_buffer.subspan(0, sge_index),
                                               now_size, total_size,  p,false, (i==0));
        if (ec) [[unlikely]] {
          co_return std::pair{ec, total_size};
        }
        sge_index = 0;
        now_size = 0;
      }
    }
  }
  if (now_size > 0) {
    auto ec = co_await async_io_regist<io>(
        ib_socket, sge_buffer.subspan(0, sge_index), now_size, total_size, p);
    if (ec) [[unlikely]] {
      co_return std::pair{ec, total_size};
    }
  }
  if (p != nullptr) {
    auto ec = co_await p->getFuture();
    if (ec) [[unlikely]] {
      co_return std::pair{ec, total_size};
    }
  }
  co_return std::pair{std::error_code{}, total_size};
}

}  // namespace detail
template <typename buffer_t, std::size_t n>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_write(
    coro_io::ib_socket_t& ib_socket, std::span<buffer_t, n> buffer) {
  std::size_t total_size_write = 0;
  std::array<ibv_sge, n> sge{};
  co_return co_await detail::async_io_split<ib_socket_t::send>(ib_socket,
                                                               buffer, sge);
}
template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_write(
    coro_io::ib_socket_t& ib_socket, std::span<buffer_t> buffer) {
  std::size_t total_size_write = 0;
  std::vector<ibv_sge> sge;
  sge.resize(buffer.size());
  co_return co_await detail::async_io_split<ib_socket_t::send>(ib_socket,
                                                               buffer, sge);
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_write(
    coro_io::ib_socket_t& ib_socket, buffer_t buffer) {
  std::size_t total_size_write = 0;
  ibv_sge sge{};
  co_return co_await detail::async_io_split<ib_socket_t::send>(
      ib_socket, std::span{&buffer, 1}, std::span{&sge,1});
}

template <typename buffer_t, std::size_t n>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_read(
    coro_io::ib_socket_t& ib_socket, std::span<buffer_t, n> buffer) {
  std::size_t total_size_write = 0;
  std::array<ibv_sge, n> sge{};
  co_return co_await detail::async_io_split<ib_socket_t::recv>(
      ib_socket, buffer, std::span<ibv_sge>{sge});
}
template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_read(
    coro_io::ib_socket_t& ib_socket, std::span<buffer_t> buffer) {
  std::size_t total_size_write = 0;
  std::vector<ibv_sge> sge;
  sge.resize(buffer.size());
  co_return co_await detail::async_io_split<ib_socket_t::recv>(
      ib_socket, buffer, std::span<ibv_sge>{sge});
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_read(
    coro_io::ib_socket_t& ib_socket, buffer_t buffer) {
  std::size_t total_size_write = 0;
  ibv_sge sge{};
  co_return co_await detail::async_io_split<ib_socket_t::recv>(
      ib_socket, std::span{&buffer, 1}, std::span{&sge,1});
}

template <typename buffer_t, std::size_t n>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_read_some(coro_io::ib_socket_t& ib_socket,
                std::span<buffer_t, n> buffer) {
  std::size_t total_size_write = 0;
  std::array<ibv_sge, n> sge{};
  std::unique_ptr<async_simple::Promise<std::error_code>> p;
  uint32_t io_size =
      detail::make_sge(sge, std::span{&buffer, 1}, ib_socket.get_buffer_size());
  auto old = ib_socket.get_config().enable_read_buffer_when_zero_copy = true;
  auto result = co_await detail::async_io_regist<ib_socket_t::recv>(
      ib_socket, std::span<ibv_sge>{sge, n}, io_size, total_size_write, p,
      true);
  ib_socket.get_config().enable_read_buffer_when_zero_copy = old;
  co_return std::pair{result, total_size_write};
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_read_some(coro_io::ib_socket_t& ib_socket, std::span<buffer_t> buffer) {
  std::size_t total_size_write = 0;
  std::vector<ibv_sge> sge;
  sge.resize(buffer.size());
  std::unique_ptr<async_simple::Promise<std::error_code>> p;
  uint32_t io_size =
      detail::make_sge(sge, std::span{&buffer, 1}, ib_socket.get_buffer_size());
  auto old = ib_socket.get_config().enable_read_buffer_when_zero_copy = true;
  auto result = co_await detail::async_io_regist<ib_socket_t::recv>(
      ib_socket, std::span<ibv_sge>{sge, buffer.size()}, io_size,
      total_size_write, p, true);
  ib_socket.get_config().enable_read_buffer_when_zero_copy = old;
  co_return std::pair{result, total_size_write};
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_read_some(coro_io::ib_socket_t& ib_socket, buffer_t buffer) {
  std::size_t total_size_write = 0;
  ibv_sge sge{};
  std::unique_ptr<async_simple::Promise<std::error_code>> p;
  uint32_t io_size =
      detail::make_sge(sge, std::span{&buffer, 1}, ib_socket.get_buffer_size());
  auto old = ib_socket.get_config().enable_read_buffer_when_zero_copy;
  ib_socket.get_config().enable_read_buffer_when_zero_copy = true;
  auto result = co_await detail::async_io_regist<ib_socket_t::recv>(
      ib_socket, std::span{&sge, 1}, io_size, total_size_write, p,
      true);
  ib_socket.get_config().enable_read_buffer_when_zero_copy = old;
  co_return std::pair{result, total_size_write};
}
}  // namespace coro_io