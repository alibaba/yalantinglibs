#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
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
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include "iguana/json_writer.hpp"
#include "ylt/coro_io/barex/barex_acceptor.hpp"
#include "ylt/coro_io/barex/barex_io.hpp"
#include "ylt/coro_io/barex/barex_socket.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/easylog/record.hpp"
#include "ylt/reflection/user_reflect_macro.hpp"
#include "ylt/standalone/iguana/prettify.hpp"
#include "ylt/struct_json/json_reader.h"
#include "ylt/struct_json/json_writer.h"

struct config_t {
  std::size_t buffer_size = 256 * 1024;
  std::size_t request_size = 20 * 1024 * 1024 + 1;
  int concurrency = 50;
  int enable_log = 0;
  bool enable_server = true;
  std::string enable_client = "127.0.0.1";
  int port = 58110;
  int test_time = 10;
};
YLT_REFL(config_t, buffer_size, request_size, concurrency, enable_log,
         enable_server, enable_client, port, test_time);

config_t config;
std::shared_ptr<coro_io::ib_device_t> g_dev;
std::atomic<uint64_t> cnt[2];

std::atomic<uint64_t> connect_cnt;

using namespace std::chrono_literals;
using namespace std::literals;
async_simple::coro::Lazy<std::error_code> echo_connect(
    coro_io::barex_socket_t soc) {
  std::string buffer;
  buffer.resize(100);
  ELOG_INFO << "start echo connect";
  ELOG_INFO << "start read from client";
  auto [ec, len] =
      co_await coro_io::async_read(soc, std::string_view{buffer.data(), 8});
  if (ec || len != 8) [[unlikely]] {
    ELOG_INFO << "err when read client:" << ec.message();
    co_return ec;
  }
  uint64_t sz = *(uint64_t *)buffer.data();
  buffer.resize(sz);
  std::tie(ec, len) =
      co_await coro_io::async_read(soc, std::string_view{buffer});
  if (ec) [[unlikely]] {
    co_return ec;
  }
  else if (len != sz) {
    co_return std::make_error_code(std::errc::protocol_error);
  }
  ELOG_INFO << "read data ok:" << len;
  char ch = 'A';
  auto s_view = std::string_view{&ch, 1};
  while (true) {
    ELOG_DEBUG << "start read head from client" << &soc;
    auto [r, s] = co_await async_simple::coro::collectAll(
        coro_io::async_read(soc, std::string_view{buffer.data(), 8}),
        coro_io::async_write(soc, s_view));
    ELOG_DEBUG << "server waiting io for r/w from client over:" << &soc;

    if (r.hasError() || s.hasError()) [[unlikely]] {
      co_return std::make_error_code(std::errc::io_error);
    }
    if (r.value().first) [[unlikely]] {
      co_return r.value().first;
    }
    if (s.value().first) [[unlikely]] {
      co_return s.value().first;
    }
    uint64_t sz = *(uint64_t *)buffer.data();
    buffer.resize(sz);
    ELOG_DEBUG << "start read body from client" << &soc;
    auto [ec, len] =
        co_await coro_io::async_read(soc, std::string_view{buffer});
    if (ec) [[unlikely]] {
      co_return r.value().first;
    }
    else if (len != sz) {
      co_return std::make_error_code(std::errc::protocol_error);
    }
    cnt[0] += len;
    ELOG_DEBUG << "read data ok:" << len;
  }
  co_return std::error_code{};
}

async_simple::coro::Lazy<std::error_code> echo_accept() {
  auto dev = coro_io::get_global_barex_device();
  auto ctx = coro_io::get_barex_context(coro_io::get_global_executor(), dev);
  CHECK(ctx != nullptr);
  coro_io::barex_acceptor_t acceptor =
      std::make_shared<coro_io::barex_acceptor_impl_t>(config.port,
                                                       std::vector{ctx});
  auto ec = acceptor->listen();
  if (ec) [[unlikely]] {
    co_return ec;
  }
  ELOG_INFO << "tcp listening";
  while (true) {
    coro_io::barex_socket_t soc;
    auto ec = co_await coro_io::async_accept(acceptor, soc);
    if (ec) [[unlikely]] {
      ELOG_INFO << "accept failed";
      co_return ec;
    }
    soc.get_config().buffer_size = config.buffer_size;
    ELOG_INFO << "start new connection";
    auto executor = soc.get_executor();
    ++connect_cnt;
    echo_connect(std::move(soc)).start([](auto &&) {
      --connect_cnt;
      ELOG_INFO << "connector over";
    });
  }
  co_return ec;
}

async_simple::coro::Lazy<std::error_code> echo_client(
    coro_io::barex_socket_t &soc, std::string_view send_view) {
  std::string recv_buffer;
  recv_buffer.resize(1);
  std::string_view recv_view = std::string_view{recv_buffer};
  ELOG_INFO << "start echo";
  while (true) {
    auto &&[r, s] = co_await async_simple::coro::collectAll(
        coro_io::async_read(soc, recv_view),
        coro_io::async_write(soc, send_view));
    if (r.hasError() || s.hasError()) [[unlikely]] {
      co_return std::make_error_code(std::errc::io_error);
    }
    if (s.value().first) [[unlikely]] {
      co_return r.value().first;
    }
    if (s.value().first) [[unlikely]] {
      co_return s.value().first;
    }
    if (s.value().second != send_view.size() || recv_view != "A") [[unlikely]] {
      ELOG_ERROR << "data err";
      co_return std::make_error_code(std::errc::protocol_error);
    }
    ELOG_INFO << "send ok,len:" << send_view.size();
    cnt[1] += send_view.size();
  }
  co_return std::error_code{};
}

async_simple::coro::Lazy<std::error_code> echo_connect() {
  coro_io::barex_socket_t soc = coro_io::barex_socket_t(
      coro_io::barex_socket_t::config_t{.buffer_size = config.buffer_size});
  co_await coro_io::sleep_for(std::chrono::milliseconds{100},
                              soc.get_coro_executor());
  auto ec = co_await coro_io::async_connect(soc, config.enable_client,
                                            std::to_string(config.port));
  if (ec) [[unlikely]] {
    co_return ec;
  }
  ELOG_INFO << "connect over";
  std::string s;
  s.resize(config.request_size, 'A');
  uint64_t &sz = *(uint64_t *)s.data();
  sz = config.request_size - 8;
  ++connect_cnt;
  ec = co_await echo_client(soc, s);
  --connect_cnt;
  co_return ec;
}
TEST_CASE("ib socket pressure test") {
  try {
    struct_json::from_json_file(config, "ib_bench_config.json");
  } catch (...) {
    std::ofstream fs;
    fs.open("ib_bench_config.json");
    std::string s;
    struct_json::to_json(config, s);
    fs << iguana::prettify(s);
  }
  auto old_s = easylog::logger<>::instance().get_min_severity();
  g_dev = coro_io::ib_device_t::create(
      {.buffer_pool_config = {.buffer_size = config.buffer_size}});
  easylog::Severity s;
  if (config.enable_log) {
    s = easylog::Severity::TRACE;
  }
  else {
    s = easylog::Severity::WARN;
  }
  easylog::logger<>::instance().set_min_severity(s);

  ELOG_INFO << "start echo server & client";
  if (config.enable_server) {
    echo_accept().start([](auto &&ec) {
      ELOG_ERROR << "accept over:" << ec.value().message();
    });
  }
  if (config.enable_client.size()) {
    for (int i = 0; i < config.concurrency; ++i)
      echo_connect().start([](auto &&ec) {
        ELOG_ERROR << "connect over:" << ec.value().message();
      });
  }
  std::atomic<uint64_t> *cnt_p;
  if (config.enable_server) {
    cnt_p = &cnt[0];
  }
  else {
    cnt_p = &cnt[1];
  }
  for (int i = 0; i < config.test_time; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds{1});
    auto c = cnt_p->exchange(0);
    std::cout << "Throughput:" << 8.0 * c / 1000'000'000
              << " Gb/s, alive connection:" << connect_cnt << std::endl;
  }
  // easylog::logger<>::instance().set_min_severity(old_s);
}