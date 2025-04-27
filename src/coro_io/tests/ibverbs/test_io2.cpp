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
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/ibverbs/ib_buffer.hpp"
#include "ylt/coro_io/ibverbs/ib_io.hpp"
#include "ylt/coro_io/ibverbs/ib_socket.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/easylog/record.hpp"

#include "doctest.h"

std::size_t buffer_size = 8 * 1024 * 1024;
int concurrency = 10;



template<auto ServerFunction>
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
  ec = co_await coro_io::async_accept(acceptor, soc);

  if (ec) [[unlikely]] {
    ELOG_INFO << "accept failed";
    co_return ec;
  }

  ELOG_INFO << "start new connection";
  auto executor = soc.get_executor();
  ec = co_await ServerFunction(std::move(soc));
  co_return ec;
}

template<auto ClientFunction>
async_simple::coro::Lazy<std::error_code> echo_connect() {
  coro_io::ib_socket_t soc{};
  soc.get_config().request_buffer_size = buffer_size;
  auto ec = co_await coro_io::async_connect(soc, "127.0.0.1", "58110");
  if (ec) [[unlikely]] {
    co_return ec;
  }
  ELOG_INFO << "connect over";
  co_return co_await ClientFunction(soc);
}

template<std::size_t data_size>
async_simple::coro::Lazy<std::error_code> test_read_server(
    coro_io::ib_socket_t soc) {
  std::string buffer;
  buffer.resize(data_size);
  std::error_code ec;
  std::size_t len;
  ELOG_INFO<<"recv buffer:"<<buffer.data();
  // co_await coro_io::sleep_for(std::chrono::seconds{10000});
  std::tie(ec,len) = co_await coro_io::async_read(soc,asio::buffer(buffer.data(),data_size));
  CHECK(len==data_size);
  CHECK_MESSAGE(buffer==std::string(data_size,'A'),buffer);
  co_return ec;
}
template<std::size_t data_size>
async_simple::coro::Lazy<std::error_code> test_write_client(coro_io::ib_socket_t &soc) {
  std::string buffer;
  buffer.resize(data_size, 'A');
  std::error_code ec;
  std::size_t len;
  ELOG_INFO<<"send buffer:"<<buffer.data();
  std::tie(ec,len) = co_await coro_io::async_write(soc,asio::buffer(buffer.data(),data_size));
  CHECK(len==data_size);
  co_return ec;
}

TEST_CASE("test socket io") {
  ELOG_INFO << "start echo server & client";
    // SUBCASE("test read/write fix size no buffer zero copy, least than rdma buffer") {
    //   auto result = async_simple::coro::syncAwait(collectAll(echo_accept<test_read_server<16>>(),echo_connect<test_write_client<16>>()));
    //   auto& ec1=std::get<0>(result);
    //   auto& ec2=std::get<1>(result);
    //   CHECK_MESSAGE(ec1.value()==std::error_code{},ec1.value().message());
    //   CHECK_MESSAGE(ec2.value()==std::error_code{},ec2.value().message());
    // }
    SUBCASE("test read/write fix size no buffer zero copy, bigger than rdma buffer") {
    auto result = async_simple::coro::syncAwait(collectAll(echo_accept<test_read_server<9*1024*1024>>(),echo_connect<test_write_client<9*1024*1024>>()));
    auto& ec1=std::get<0>(result);
    auto& ec2=std::get<1>(result);
    CHECK_MESSAGE(ec1.value()==std::error_code{},ec1.value().message());
    CHECK_MESSAGE(ec2.value()==std::error_code{},ec2.value().message());
  }
}