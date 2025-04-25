#include <chrono>
#include <cmath>
#include <string_view>
#include <system_error>
#include <thread>

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

std::size_t buffer_size = 8 * 1024 * 1024;
int concurrency = 10;

async_simple::coro::Lazy<std::error_code> echo_connect(
    coro_io::ib_socket_t soc) {
  char *buffer = new char[buffer_size];
  ELOG_INFO << "start echo connect";
  coro_io::ib_buffer_t ib =
      coro_io::ib_buffer_t::regist(soc.get_device(), buffer, buffer_size);
  ELOG_INFO << "start read from client";
  auto [ec, len] =
      co_await coro_io::async_read_some(soc, ib.subview());

  if (ec) [[unlikely]] {
    ELOG_INFO << "err when read client:" << ec.message();
    co_return ec;
  }
  ELOG_INFO << "read data ok:"<<len;
  for (int i = 0;; i ^= 1) {
    auto r_view = ib.subview();
    auto s_view = ib.subview(0,len);

    ELOG_INFO << "start read from client" << &soc;
    auto [r, s] = co_await async_simple::coro::collectAll(
        coro_io::async_read_some(soc, r_view),
        coro_io::async_write(soc, s_view));
    ELOG_INFO << "server waiting io for r/w from client over" << &soc;

    if (r.hasError() || s.hasError()) [[unlikely]] {
      co_return std::make_error_code(std::errc::io_error);
    }
    if (s.value().first) [[unlikely]] {
      co_return r.value().first;
    }
    if (s.value().first) [[unlikely]] {
      co_return s.value().first;
    }
    len = r.value().second;
    ELOG_INFO << "read data ok:"<<len;
  }
  delete[] buffer;
  co_return std::error_code{};
}

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
  while (true) {
    coro_io::ib_socket_t soc;
    soc.get_config().request_buffer_size = buffer_size;
    auto ec = co_await coro_io::async_accept(acceptor, soc);

    if (ec) [[unlikely]] {
      ELOG_INFO << "accept failed";
      co_return ec;
    }

    ELOG_INFO << "start new connection";
    auto executor = soc.get_executor();
    echo_connect(std::move(soc)).start([](auto &&) {
    });
  }
  co_return ec;
}

std::atomic<std::size_t> cnt;

async_simple::coro::Lazy<std::error_code> echo_client(coro_io::ib_socket_t &soc,
                                                      std::string_view sv) {
  std::string buffer;
  buffer.resize(buffer_size);
  auto ib2 = coro_io::ib_buffer_t::regist(soc.get_device(), buffer.data(),
                                          buffer_size);
  auto ib = coro_io::ib_buffer_t::regist(soc.get_device(), (char *)sv.data(),
                                         sv.size());
  ELOG_INFO << "start echo";
  for (int i = 0;; i ^= 1) {
    ELOG_INFO << "prepare read buffer";
    ibv_sge r_view = ib2.subview();
    ELOG_INFO << "prepare write buffer";
    ibv_sge s_view = ib.subview();
    ELOG_INFO << "echoing";
    auto [result, time_out] = co_await async_simple::coro::collectAll<
        async_simple::SignalType::Terminate>(
        async_simple::coro::collectAll(coro_io::async_read_some(soc, r_view),
                                       coro_io::async_write(soc, s_view)),
        coro_io::sleep_for(std::chrono::seconds{1000}));
    ELOG_INFO << "echoing over";
    if (result.hasError() || time_out.hasError()) [[unlikely]] {
      co_return std::make_error_code(std::errc::io_error);
    }
    if (time_out.value()) [[unlikely]] {
      ELOG_INFO << "Time out!";
      co_return std::make_error_code(std::errc::timed_out);
    }
    auto &&[r, s] = result.value();
    if (r.hasError() || s.hasError()) [[unlikely]] {
      co_return std::make_error_code(std::errc::io_error);
    }
    if (s.value().first) [[unlikely]] {
      co_return r.value().first;
    }
    if (s.value().first) [[unlikely]] {
      co_return s.value().first;
    }
    auto resp = std::string_view{(char *)r_view.addr, r.value().second};
    if (resp.size() != sv.size()) [[unlikely]] {
      ELOG_INFO << "data err";
      co_return std::make_error_code(std::errc::protocol_error);
    }
    else {
      cnt += sv.size();
    }
  }
  co_return std::error_code{};
}

async_simple::coro::Lazy<std::error_code> echo_connect(
    std::size_t buffer_size) {
  coro_io::ib_socket_t soc{};
  soc.get_config().request_buffer_size = buffer_size;
  auto ec = co_await coro_io::async_connect(soc, "127.0.0.1", "58110");
  if (ec) [[unlikely]] {
    co_return ec;
  }
  ELOG_INFO << "connect over";
  std::string s;
  s.resize(buffer_size, 'A');
  co_return co_await echo_client(soc, s);
}

int main() {
  easylog::logger<>::instance().init(easylog::Severity::WARN, false, true,
                                     "1.log", 10000, 1, true);
  ELOG_INFO << "start echo server & client";
  echo_accept().start([](auto &&ec) {
    ELOG_ERROR << ec.value().message();
  });
  for (int i = 0; i < concurrency; ++i)
    echo_connect(buffer_size).start([](auto &&ec) {
      ELOG_ERROR << ec.value().message();
    });
  for (int i = 0; i < 1000; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds{1});
    auto c = cnt.exchange(0);
    std::cout << "Throughput:" << 8.0 * c / 1000'000 << " Mb/s" << std::endl;
  }
  return 0;
}