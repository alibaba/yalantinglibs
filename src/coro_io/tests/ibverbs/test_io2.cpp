#include <array>
#include <chrono>
#include <cmath>
#include <string>
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
bool enable_read_buffer_when_zero_copy = false;
std::atomic<int> port;

template <auto... ServerFunction>
async_simple::coro::Lazy<std::error_code> echo_accept() {
  asio::ip::tcp::acceptor acceptor(
      coro_io::get_global_executor()->get_asio_executor());
  std::error_code ec;
  auto address = asio::ip::address_v4::from_string("0.0.0.0", ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  auto endpoint = asio::ip::tcp::endpoint(address, 0);
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
  port = acceptor.local_endpoint().port();
  ELOG_INFO << "port:" << port;
  acceptor.listen(asio::ip::tcp::socket::max_listen_connections, ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }

  ELOG_INFO << "tcp listening port:" << port;
  coro_io::ib_socket_t soc;
  soc.get_config().request_buffer_size = buffer_size;
  soc.get_config().enable_zero_copy = enable_zero_copy;
  soc.get_config().enable_read_buffer_when_zero_copy =
      enable_read_buffer_when_zero_copy;
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
  soc.get_config().enable_read_buffer_when_zero_copy =
      enable_read_buffer_when_zero_copy;
  ELOG_INFO << "tcp connecting port:" << port;
  auto ec =
      co_await coro_io::async_connect(soc, "127.0.0.1", std::to_string(port));
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

template <std::size_t data_size, bool never_run = false>
async_simple::coro::Lazy<std::error_code> test_read(coro_io::ib_socket_t& soc) {
  std::string buffer;
  buffer.resize(data_size);
  std::error_code ec;
  std::size_t len;
  ELOG_TRACE << "START READ";
  if constexpr (never_run) {
    co_await coro_io::sleep_for(std::chrono::seconds{1000},
                                soc.get_coro_executor());
  }
  std::tie(ec, len) =
      co_await coro_io::async_read(soc, asio::buffer(buffer.data(), data_size));
  if (!ec) {
    CHECK(len == data_size);
    ELOG_TRACE << "len:" << strlen(buffer.data());
    CHECK(strlen(buffer.data()) == data_size);
    CHECK_MESSAGE(buffer == std::string(data_size, 'A'), buffer);
  }
  co_return ec;
}

template <std::size_t data_size>
async_simple::coro::Lazy<std::error_code> test_read_some(
    coro_io::ib_socket_t& soc) {
  std::string buffer;
  buffer.resize(data_size);
  std::error_code ec;
  std::size_t len;
  ELOG_TRACE << "START READ SOME";
  std::tie(ec, len) = co_await coro_io::async_read_some(
      soc, asio::buffer(buffer.data(), buffer.size()));
  if (!ec) {
    auto sz = std::min<std::size_t>(data_size, soc.get_buffer_size());
    CHECK(len == sz);
    CHECK(std::string_view{buffer.data(), len} == std::string(sz, 'A'));
  }
  co_return ec;
}

template <std::size_t data_size, bool never_run = false>
async_simple::coro::Lazy<std::error_code> test_write(
    coro_io::ib_socket_t& soc) {
  std::string buffer;
  buffer.resize(data_size, 'A');
  std::error_code ec;
  std::size_t len;
  ELOG_TRACE << "START WRITE";
  if constexpr (never_run) {
    co_await coro_io::sleep_for(std::chrono::seconds{1000},
                                soc.get_coro_executor());
  }
  std::tie(ec, len) = co_await coro_io::async_write(
      soc, asio::buffer(buffer.data(), data_size));
  if (!ec) {
    CHECK(len == data_size);
  }
  co_return ec;
}

template <std::size_t data_size, std::size_t iov_size>
async_simple::coro::Lazy<std::error_code> test_write_iov(
    coro_io::ib_socket_t& soc) {
  std::array<std::string, iov_size> buffer;
  for (auto& e : buffer) e.resize(data_size, 'A');
  std::array<std::string_view, iov_size> buffer_collect;
  std::error_code ec;
  std::size_t len;
  ELOG_TRACE << "START WRITE iov(size:" << iov_size << ")";
  std::tie(ec, len) = co_await coro_io::async_write(soc, buffer);
  if (!ec) {
    CHECK(len == data_size * iov_size);
  }
  co_return ec;
}

template <std::size_t data_size, std::size_t iov_size>
async_simple::coro::Lazy<std::error_code> test_read_iov(
    coro_io::ib_socket_t& soc) {
  std::array<std::string, iov_size> buffer;
  for (auto& e : buffer) e.resize(data_size);
  std::array<std::string_view, iov_size> buffer_collect;
  std::error_code ec;
  std::size_t len;
  ELOG_TRACE << "START READ iov(size:" << iov_size << ")";
  std::tie(ec, len) = co_await coro_io::async_read(soc, buffer);
  if (!ec) {
    CHECK(len == data_size * iov_size);
    for (auto& e : buffer) {
      CHECK(std::string_view{e.data(), data_size} ==
            std::string(data_size, 'A'));
    }
  }
  co_return ec;
}

TEST_CASE("test socket io") {
  ELOG_INFO << "start echo server & client";
  buffer_size = 8 * 1024;
  std::array<std::array<bool, 2>, 3> config = {std::array{true, false},
                                               std::array{true, true},
                                               std::array{false, false}};

  for (auto& e : config) {
    enable_zero_copy = e[0];
    enable_read_buffer_when_zero_copy = e[1];
    ELOG_WARN << "enable zero copy:" << enable_zero_copy;
    ELOG_WARN << "enable_read_buffer_when_zero_copy:"
              << enable_read_buffer_when_zero_copy;
    {
      ELOG_WARN << "test read/write fix size, least than rdma "
                   "buffer";
      auto result = async_simple::coro::syncAwait(collectAll(
          echo_accept<test_read<16>>(), echo_connect<test_write<16>>()));
      auto& ec1 = std::get<0>(result);
      auto& ec2 = std::get<1>(result);
      CHECK_MESSAGE(!ec1.value(), ec1.value().message());
      CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    }
    // {
    //   ELOG_WARN << "test read/write fix size, bigger than rdma "
    //                "buffer";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read<9 * 1024>>(),
    //                  echo_connect<test_write<9 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read/write fix size, very bigger than rdma "
    //                "buffer";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read<35 * 1024>>(),
    //                  echo_connect<test_write<35 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read bigger than write";
    //   auto result = async_simple::coro::syncAwait(collectAll(
    //       echo_accept<test_read<7 * 1024>>(),
    //       echo_connect<test_write<4 * 1024>, test_write<3 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read bigger than write & bigger than buffer size";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read<35 * 1024>>(),
    //                  echo_connect<test_write<7 * 1024>, test_write<2 * 1024>,
    //                               test_write<26 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read iov bigger than write";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read_iov<1 * 1024, 7>>(),
    //                  echo_connect<test_write<4 * 1000>,
    //                               test_write<7 * 1024 - 4 * 1000>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read iov bigger than write & bigger than buffer size";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read_iov<12 * 1024, 3>>(),
    //                  echo_connect<test_write<7 * 1024>, test_write<2 * 1024>,
    //                               test_write<27 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read_some & write with same size";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read_some<7 * 1024>>(),
    //                  echo_connect<test_write<7 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read_some & read/write";
    //   auto result = async_simple::coro::syncAwait(collectAll(
    //       echo_accept<test_read_some<3 * 1024>, test_read<9 * 1024>>(),
    //       echo_connect<test_write<12 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read_some & read/write with small data";
    //   auto result = async_simple::coro::syncAwait(collectAll(
    //       echo_accept<test_read_some<3 * 1024>, test_read<2 * 1024>>(),
    //       echo_connect<test_write<5 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read_some over size";
    //   auto result = async_simple::coro::syncAwait(collectAll(
    //       echo_accept<test_read_some<11 * 1024>, test_read<9 * 1024>>(),
    //       echo_connect<test_write<17 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test write iov";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read<3 * 1024>>(),
    //                  echo_connect<test_write_iov<1 * 1024, 3>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test write iov multi sge";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read<20 * 1024>>(),
    //                  echo_connect<test_write_iov<5 * 1024, 4>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test write iov over buffer size";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read<36 * 1024>>(),
    //                  echo_connect<test_write_iov<9 * 1024, 4>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }

    // {
    //   ELOG_WARN << "test read iov";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read_iov<1 * 1024, 3>>(),
    //                  echo_connect<test_write<3 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read iov multi sge";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read_iov<5 * 1024, 4>>(),
    //                  echo_connect<test_write<20 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read iov multi sge bigger";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read_iov<9 * 1024, 4>>(),
    //                  echo_connect<test_write<36 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    // }
    // {
    //   ELOG_WARN << "test read smaller than write";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read<7 * 1024>, test_read<1 * 1024>>(),
    //                  echo_connect<test_write<8 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   if (enable_zero_copy && !enable_read_buffer_when_zero_copy) {
    //     CHECK_MESSAGE(ec1.value(), ec1.value().message());
    //     CHECK_MESSAGE(ec2.value(), ec2.value().message());
    //   }
    //   else {
    //     CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //     CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    //   }
    // }
    // {
    //   ELOG_WARN << "test read smaller than write with bigger data";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll(echo_accept<test_read<2 * 1024>, test_read<17 * 1024>,
    //                              test_read<11 * 1024>>(),
    //                  echo_connect<test_write<30 * 1024>>()));
    //   auto& ec1 = std::get<0>(result);
    //   auto& ec2 = std::get<1>(result);
    //   if (enable_zero_copy && !enable_read_buffer_when_zero_copy) {
    //     CHECK_MESSAGE(ec1.value(), ec1.value().message());
    //     CHECK_MESSAGE(ec2.value(), ec2.value().message());
    //   }
    //   else {
    //     CHECK_MESSAGE(!ec1.value(), ec1.value().message());
    //     CHECK_MESSAGE(!ec2.value(), ec2.value().message());
    //   }
    // }
    // {
    //   ELOG_WARN << "test read time out";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll<async_simple::SignalType::Terminate>(
    //           collectAll(echo_accept<test_read<2 * 1024>>(),
    //                      echo_connect<test_write<2 * 1024, true>>()),
    //           coro_io::sleep_for(std::chrono::milliseconds{100})));
    //   auto& ec1 = std::get<0>(std::get<0>(result).value());
    //   auto& ec2 = std::get<1>(std::get<0>(result).value());
    //   auto& ec3 = std::get<1>(result);
    //   CHECK_MESSAGE(ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(ec2.value(), ec2.value().message());
    //   CHECK_MESSAGE(ec3.value(), "time out failed");
    // }
    // {
    //   ELOG_WARN << "test write time out";
    //   auto result = async_simple::coro::syncAwait(
    //       collectAll<async_simple::SignalType::Terminate>(
    //           collectAll(echo_accept<test_read<2 * 1024, true>>(),
    //                      echo_connect<test_write<2 * 1024>>()),
    //           coro_io::sleep_for(std::chrono::milliseconds{100})));

    //   auto& ec1 = std::get<0>(std::get<0>(result).value());
    //   auto& ec2 = std::get<1>(std::get<0>(result).value());
    //   auto& ec3 = std::get<1>(result);
    //   CHECK_MESSAGE(ec1.value(), ec1.value().message());
    //   CHECK_MESSAGE(ec2.value(), ec2.value().message());
    //   CHECK_MESSAGE(ec3.value(), "time out failed");
    // }
  }
  ELOG_WARN << "memory size:" << coro_io::g_ib_buffer_pool()->total_memory();
}
