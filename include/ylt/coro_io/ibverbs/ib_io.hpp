#include <infiniband/verbs.h>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <system_error>
#include "async_simple/Signal.h"
#include "ib_socket.hpp"
#include "ib_buffer.hpp"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Collect.h"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/ibverbs/ib_device.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/struct_pack.hpp"
#include "ylt/struct_pack/reflection.hpp"
namespace coro_io {

// unlike tcp socket, client won't connnected util server first read ib_socket.
inline async_simple::coro::Lazy<std::error_code> async_accept(
    asio::ip::tcp::acceptor &acceptor, coro_io::ib_socket_t& ib_socket) noexcept {
  auto soc = std::make_unique<asio::ip::tcp::socket>(ib_socket.get_executor()->get_asio_executor());
  auto ec = co_await async_io<std::error_code>(
      [&](auto &&cb) {
        acceptor.async_accept(*soc, std::move(cb));
      },
      acceptor);
  
  if (ec) [[unlikely]] {
    co_return std::move(ec);
  }
  //TODO: SSL?
  auto ret= co_await ib_socket.accept(std::move(soc));
  ELOGV(INFO, "accept over:%s", ret.message().data());
  co_return ret;
}

inline async_simple::coro::Lazy<std::error_code> async_connect(coro_io::ib_socket_t& ib_socket, const std::string &host, const std::string &port) noexcept {
  // todo: get socket from client_pool, not connect each time?

  asio::ip::tcp::socket soc{ib_socket.get_executor()->get_asio_executor()};
  auto ec = co_await async_connect(ib_socket.get_executor(),soc,host,port);
  if (ec) [[unlikely]] {
    co_return std::move(ec);
  }
  co_return co_await ib_socket.connect(soc);
}

inline async_simple::coro::Lazy<std::pair<std::error_code,std::size_t>> async_read(coro_io::ib_socket_t& ib_socket, std::span<char> buffer) noexcept {
  std::size_t total_size_read = 0;
  ib_buffer_t ib_buffer[2];
  std::size_t block_size =
          std::min<std::size_t>(ib_socket.get_remote_buffer_size(), buffer.size());
  if (!ib_socket.get_config().enable_zero_copy){
    do {
      ib_buffer[0] = ib_socket.buffer_pool().get_buffer();
      if (!ib_buffer[0]) {
        break;
      } 
      if (ib_socket.get_remote_buffer_size()<buffer.size()) {
        ib_buffer[1] = ib_socket.buffer_pool().get_buffer();
        if (!ib_buffer[1]) {
          break;
        }
      }
      auto [ec,len] =co_await ib_socket.async_io<ib_socket_t::read>(ib_buffer_view_t{ib_buffer[0]});
      if (ec) {
        co_return std::pair{ec,0};
      }
      int buffer_index = 1;
      for (; total_size_read + len < buffer.size(); buffer_index^=1) {
          auto [result,result2] = co_await async_simple::coro::collectAll(ib_socket.async_io<ib_socket_t::read>(ib_buffer_view_t{ib_buffer[buffer_index]}),[address= ib_buffer[buffer_index]->addr,len=len,dst=buffer.data()+total_size_read]()->async_simple::coro::Lazy<void>{
            memcpy(dst, address, len);
            co_return;
          }());
          if (result.hasError()||result2.hasError()) [[unlikely]] {
            co_return std::pair{std::make_error_code(std::errc::io_error),total_size_read};
          }
          total_size_read += len;
          len=result.value().second;
          if (result.value().first) [[unlikely]] {
            co_return std::pair{result.value().first,total_size_read};
          }
      }
      memcpy(buffer.data()+total_size_read, ib_buffer[buffer_index]->addr,len);
      total_size_read+=len;
      co_return std::pair{std::error_code{},total_size_read};
    } while (false);
  }
  ib_buffer[0] = ib_buffer_t::regist(ib_socket.get_device(), buffer.data(), std::min(buffer.size(),block_size));
  for (; total_size_read < buffer.size();) {
    std::span<char> next_buffer = {(char*)buffer.data()+total_size_read+ib_buffer[0]->length,std::min(buffer.size()-total_size_read-ib_buffer[0]->length,block_size)};
    auto [ec,size] = co_await ib_socket.async_io<ib_socket_t::read>(ib_buffer[0], next_buffer);
    if (ec) [[unlikely]] {
      co_return std::pair{ec,total_size_read};
    }
    total_size_read += size;
  }
  co_return std::pair{std::error_code{},total_size_read};
}

inline async_simple::coro::Lazy<std::pair<std::error_code,std::size_t>> async_read(coro_io::ib_socket_t& ib_socket, ib_buffer_view_t buffer) noexcept {
  std::size_t total_size_read = 0;
  std::size_t block_size =
          std::min<std::size_t>(ib_socket.get_remote_buffer_size(), buffer.length());
  for (std::size_t i = 0; i < buffer.length(); i += block_size) {
    auto [ec, size] = co_await ib_socket.async_io<ib_socket_t::read>(
        buffer.subview(i, std::min(buffer.length() - i, block_size)));
    total_size_read += size;
    if (ec) [[unlikely]] {
      co_return std::pair{ec,total_size_read};
    }
  }
  co_return std::pair{std::error_code{},total_size_read};
}

inline async_simple::coro::Lazy<std::pair<std::error_code,std::size_t>> async_read_some(coro_io::ib_socket_t& ib_socket, ib_buffer_view_t buffer) noexcept {
  if (ib_socket.get_remote_buffer_size()>buffer.length()) {
    ELOG_INFO<<"buffer len short";
    co_return std::pair{std::make_error_code(std::errc::no_buffer_space),0};
  }
  ELOG_INFO<<"START IO";
  co_return co_await ib_socket.async_io<ib_socket_t::read>(buffer);
}

inline async_simple::coro::Lazy<std::pair<std::error_code,std::size_t>> async_write(coro_io::ib_socket_t& ib_socket, std::span<char> buffer) noexcept {
  std::size_t block_size = std::min<std::size_t>(ib_socket.get_remote_buffer_size(), buffer.size());
  ib_buffer_t ib_buffer[2];
  int buffer_index=0;
  if (!ib_socket.get_config().enable_zero_copy) {
    do {
      std::size_t total_size_write = block_size;
      ib_buffer[0] = ib_socket.buffer_pool().get_buffer();
      if (!ib_buffer[0]) {
        break;
      } 
      if (ib_socket.get_remote_buffer_size()<buffer.size()) {
        ib_buffer[1] = ib_socket.buffer_pool().get_buffer();
        if (!ib_buffer[1]) {
          break;
        }
      }
      memcpy(ib_buffer[buffer_index]->addr,buffer.data(),block_size);
      for (; total_size_write < buffer.size(); buffer_index^=1, total_size_write+=block_size) {
          auto view=ib_buffer_view_t{ib_buffer[buffer_index]->lkey,ib_buffer[buffer_index]->addr,block_size};
          block_size = std::min<std::size_t>(ib_socket.get_remote_buffer_size(), buffer.size()-total_size_write);
          auto [result,result2] = co_await async_simple::coro::collectAll(ib_socket.async_io<ib_socket_t::write>(view),[address = ib_buffer[buffer_index]->addr,block_size,src=buffer.data()+total_size_write]()->async_simple::coro::Lazy<void>{
            memcpy(address,src, block_size);
            co_return;
          }());
          if (result.hasError()||result2.hasError()) [[unlikely]] {
            co_return std::pair{std::make_error_code(std::errc::io_error),total_size_write};
          }
          if (result.value().first) [[unlikely]] {
            co_return std::pair{result.value().first,total_size_write};
          }
      }
      auto view=ib_buffer_view_t{ib_buffer[buffer_index]->lkey,ib_buffer[buffer_index]->addr,block_size};
      auto [ec,len] =co_await ib_socket.async_io<ib_socket_t::write>(view);
      if (ec) {
        co_return std::pair{ec,0};
      }
      total_size_write+=len;
      co_return std::pair{std::error_code{},total_size_write};
    } while (false);
  }
  std::size_t total_size_write = 0;
  ib_buffer[0] = ib_buffer_t::regist(ib_socket.get_device(),buffer.data(),block_size);
  for (;total_size_write<buffer.size();) {
    std::span<char> next_buffer = {(char*)buffer.data()+total_size_write,std::min(buffer.size()-total_size_write,block_size)};
    auto [ec,len] = co_await ib_socket.async_io<ib_socket_t::write>(ib_buffer[0],next_buffer);
    if (ec) [[unlikely]] {
      co_return std::pair{ec,total_size_write};
    }
    total_size_write+=block_size;
    block_size=next_buffer.size();
  }
  co_return std::pair{std::error_code{},total_size_write};
}

inline async_simple::coro::Lazy<std::pair<std::error_code,std::size_t>> async_write(coro_io::ib_socket_t& ib_socket, ib_buffer_view_t buffer) noexcept {
  std::size_t block_size = std::min<std::size_t>(ib_socket.get_remote_buffer_size(), buffer.length());
  std::size_t total_size_write = 0;
  for (;total_size_write<buffer.length();) {
    std::span<char> next_buffer = {(char*)buffer.address()+total_size_write,std::min(buffer.length()-total_size_write,block_size)};
    auto [ec,len] = co_await ib_socket.async_io<ib_socket_t::write>(buffer);
    if (ec) [[unlikely]] {
      co_return std::pair{ec,total_size_write};
    }
    total_size_write+=block_size;
    block_size=next_buffer.size();
  }
  co_return std::pair{std::error_code{},total_size_write};
}

}