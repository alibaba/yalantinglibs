#include <chrono>
#include <cmath>
#include <string_view>
#include <system_error>
#include <thread>

#include "asio/buffer.hpp"
#include "asio/ip/address.hpp"
#include "asio/ip/address_v4.hpp"
#include "asio/ip/tcp.hpp"
#include "async_simple/Signal.h"
#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include "async_simple/coro/SyncAwait.h"
#include "doctest.h"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/ibverbs/ib_buffer.hpp"
#include "ylt/coro_io/ibverbs/ib_io.hpp"
#include "ylt/coro_io/ibverbs/ib_socket.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/easylog/record.hpp"

std::size_t buffer_size = 8 * 1024 * 1024;
int concurrency = 10;
bool enable_zero_copy = false;

template <auto... ServerFunction>
async_simple::coro::Lazy<std::error_code> echo_accept() {
  asio::ip::tcp::acceptor acceptor(
      coro_io::get_global_executor()->get_asio_executor());
  std::error_code ec;
  auto address = asio::ip::address_v4::from_string("0.0.0.0", ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  auto endpoint = asio::ip::tcp::endpoint(address, 58110);
  acceptor.open(endpoint.protocol(), ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  acceptor.bind(endpoint, ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  acceptor.listen(asio::ip::tcp::socket::max_listen_connections, ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }

  ELOG_INFO << "tcp listening";
  coro_io::ib_socket_t soc;
  soc.get_config().request_buffer_size = buffer_size;
  soc.get_config().enable_zero_copy = enable_zero_copy;
  ec = co_await coro_io::async_accept(acceptor, soc);

  if (ec) [[unlikely]] {
    ELOG_INFO << "accept failed";
    co_return ec;
  }

  ELOG_INFO << "start new connection";
  auto executor = soc.get_executor();
  (co_await [&]() -> async_simple::coro::Lazy<bool> {
    ec = co_await ServerFunction(soc);
    if (ec) {
      co_return true;
    }
    else {
      co_return false;
    }
  }() || ...);
  co_return ec;
}

template <auto... ClientFunction>
async_simple::coro::Lazy<std::error_code> echo_connect() {
  coro_io::ib_socket_t soc{};
  soc.get_config().request_buffer_size = buffer_size;
  soc.get_config().enable_zero_copy = enable_zero_copy;
  auto ec = co_await coro_io::async_connect(soc, "127.0.0.1", "58110");
  if (ec) [[unlikely]] {
    co_return ec;
  }
  ELOG_INFO << "connect over";
  (co_await [&]() -> async_simple::coro::Lazy<bool> {
    ec = co_await ClientFunction(soc);
    if (ec) {
      co_return true;
    }
    else {
      co_return false;
    }
  }() || ...);
  co_return ec;
}

template <std::size_t data_size>
async_simple::coro::Lazy<std::error_code> test_read_server(
    coro_io::ib_socket_t& soc) {
  std::string buffer;
  buffer.resize(data_size);
  std::error_code ec;
  std::size_t len;
  ELOG_TRACE << "START READ";
  std::tie(ec, len) =
      co_await coro_io::async_read(soc, asio::buffer(buffer.data(), data_size));
  CHECK(len == data_size);
  CHECK_MESSAGE(buffer == std::string(data_size, 'A'), buffer);
  co_return ec;
}

template <std::size_t data_size>
async_simple::coro::Lazy<std::error_code> test_read_some_server(
    coro_io::ib_socket_t& soc) {
  std::string buffer;
  buffer.resize(data_size);
  std::error_code ec;
  std::size_t len;
  ELOG_TRACE << "START READ SOME";
  std::tie(ec, len) = co_await coro_io::async_read_some(
      soc, asio::buffer(buffer.data(), buffer.size()));
  auto sz = std::min<std::size_t>(data_size, soc.get_buffer_size());
  CHECK(len == sz);
  CHECK(std::string_view{buffer.data(), len} == std::string(sz, 'A'));
  co_return ec;
}

template <std::size_t data_size>
async_simple::coro::Lazy<std::error_code> test_write_client(
    coro_io::ib_socket_t& soc) {
  std::string buffer;
  buffer.resize(data_size, 'A');
  std::error_code ec;
  std::size_t len;
  ELOG_TRACE << "START WRITE";
  // co_await coro_io::sleep_for(std::chrono::seconds{1000});
  std::tie(ec, len) = co_await coro_io::async_write(
      soc, asio::buffer(buffer.data(), data_size));
  CHECK(len == data_size);
  co_return ec;
}

TEST_CASE("test socket io") {
  ELOG_INFO << "start echo server & client";
  buffer_size = 8 * 1024;
  for (int i = 0; i < 1; ++i) {
    enable_zero_copy = !!i;
    ELOG_INFO << "enable zero copy:" << enable_zero_copy;
    SUBCASE(
        "test read/write fix size no buffer zero copy, least than rdma "
        "buffer") {
      auto result = async_simple::coro::syncAwait(
          collectAll(echo_accept<test_read_server<16>>(),
                     echo_connect<test_write_client<16>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
    SUBCASE(
        "test read/write fix size no buffer zero copy, bigger than rdma "
        "buffer") {
      auto result = async_simple::coro::syncAwait(
          collectAll(echo_accept<test_read_server<9 * 1024>>(),
                     echo_connect<test_write_client<9 * 1024>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
    SUBCASE(
        "test read/write fix size no buffer zero copy, very bigger than rdma "
        "buffer") {
      auto result = async_simple::coro::syncAwait(
          collectAll(echo_accept<test_read_server<35 * 1024>>(),
                     echo_connect<test_write_client<35 * 1024>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
    SUBCASE("test read_some & write with same size") {
      auto result = async_simple::coro::syncAwait(
          collectAll(echo_accept<test_read_some_server<7 * 1024>>(),
                     echo_connect<test_write_client<7 * 1024>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
    SUBCASE("test read_some & read/write") {
      auto result = async_simple::coro::syncAwait(
          collectAll(echo_accept<test_read_some_server<3 * 1024>,
                                 test_read_server<9 * 1024>>(),
                     echo_connect<test_write_client<12 * 1024>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
    SUBCASE("test read_some & read/write with small data") {
      auto result = async_simple::coro::syncAwait(
          collectAll(echo_accept<test_read_some_server<3 * 1024>,
                                 test_read_server<2 * 1024>>(),
                     echo_connect<test_write_client<5 * 1024>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
    SUBCASE("test read_some over size") {
      auto result = async_simple::coro::syncAwait(
          collectAll(echo_accept<test_read_some_server<11 * 1024>,
                                 test_read_server<9 * 1024>>(),
                     echo_connect<test_write_client<17 * 1024>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
    SUBCASE("test read_some over size") {
      auto result = async_simple::coro::syncAwait(
          collectAll(echo_accept<test_read_some_server<11 * 1024>,
                                 test_read_server<9 * 1024>>(),
                     echo_connect<test_write_client<17 * 1024>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
  }

  ELOG_INFO << "memory size:" << coro_io::g_ib_buffer_pool()->total_memory();
}
