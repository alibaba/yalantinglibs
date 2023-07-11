#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>
#include <doctest.h>

#include <asio/io_context.hpp>
#include <atomic>
#include <cassert>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <mutex>
#include <string_view>
#include <system_error>
#include <thread>
#include <ylt/coro_io/channel.hpp>
#include <ylt/coro_io/coro_file.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_io/io_context_pool.hpp>

#include "async_simple/Executor.h"
#include "async_simple/Promise.h"
#include "async_simple/Unit.h"
#include "async_simple/coro/ConditionVariable.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Semaphore.h"
#include "async_simple/coro/Sleep.h"
#include "async_simple/coro/SpinLock.h"
#include "ylt/coro_rpc/impl/coro_rpc_client.hpp"
#include "ylt/coro_rpc/impl/default_config/coro_rpc_config.hpp"
#include "ylt/coro_rpc/impl/expected.hpp"
using namespace std::chrono_literals;
using namespace async_simple::coro;
TEST_CASE("test client pool") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    server.async_start().start([](auto &&) {
    });
    auto is_started = server.wait_for_start(1s);
    REQUIRE(is_started);
    auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
        "127.0.0.1:8801", {.max_connection = 100, .idle_timeout = 300ms});
    std::vector<Lazy<coro_rpc::expected<void, std::errc>>> res;
    auto event = [&res, &pool]() {
      auto op = [](coro_rpc::coro_rpc_client &client)
          -> async_simple::coro::Lazy<void> {
        co_await coro_io::sleep_for(100ms);
      };
      res.emplace_back(pool->send_request(op));
    };
    for (int i = 0; i < 50; ++i) {
      event();
    }
    co_await collectAll(std::move(res));
    CHECK(pool->free_client_count() == 50);
    res.clear();
    for (int i = 0; i < 110; ++i) {
      event();
    }
    co_await collectAll(std::move(res));
    CHECK(pool->free_client_count() == 100);
    co_await coro_io::sleep_for(700ms);
    CHECK(pool->free_client_count() == 0);
    server.stop();
  }());
}
TEST_CASE("test idle timeout yield") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    server.async_start().start([](auto &&) {
    });
    auto is_started = server.wait_for_start(1s);
    REQUIRE(is_started);
    auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
        "127.0.0.1:8801", {.max_connection = 100,
                           .idle_queue_max_clear_count = 1,
                           .idle_timeout = 300ms});
    std::vector<Lazy<coro_rpc::expected<void, std::errc>>> res;
    auto event = [&res, &pool]() {
      auto op = [](coro_rpc::coro_rpc_client &client)
          -> async_simple::coro::Lazy<void> {
        co_await coro_io::sleep_for(100ms);
      };
      res.emplace_back(pool->send_request(op));
    };
    for (int i = 0; i < 100; ++i) {
      event();
    }
    co_await collectAll(std::move(res));
    CHECK(pool->free_client_count() == 100);
    res.clear();
    co_await coro_io::sleep_for(700ms);
    CHECK(pool->free_client_count() == 0);
    server.stop();
  }());
}

TEST_CASE("test reconnect") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
        "127.0.0.1:8801",
        {.connect_retry_count = 3, .reconnect_wait_time = 500ms});
    std::vector<Lazy<coro_rpc::expected<void, std::errc>>> res;
    auto event = [&res, &pool]() {
      auto op = [](coro_rpc::coro_rpc_client &client)
          -> async_simple::coro::Lazy<void> {
        co_await coro_io::sleep_for(100ms);
      };
      res.emplace_back(pool->send_request(op));
    };
    for (int i = 0; i < 100; ++i) {
      event();
    }
    coro_rpc::coro_rpc_server server(1, 8801);
    res.push_back([&server]() -> Lazy<coro_rpc::expected<void, std::errc>> {
      co_await coro_io::sleep_for(700ms);
      server.async_start().start([](auto &&) {
      });
      co_return coro_rpc::expected<void, std::errc>{};
    }());
    co_await collectAll(std::move(res));
    CHECK(pool->free_client_count() == 100);
    server.stop();
  }());
}

TEST_CASE("test collect_free_client") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    server.async_start().start([](auto &&) {
    });
    auto is_started = server.wait_for_start(1s);
    REQUIRE(is_started);
    auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
        "127.0.0.1:8801", {.max_connection = 100, .idle_timeout = 300ms});
    std::vector<Lazy<coro_rpc::expected<void, std::errc>>> res;
    auto event = [&res, &pool]() {
      auto op = [](coro_rpc::coro_rpc_client &client)
          -> async_simple::coro::Lazy<void> {
        co_await coro_io::sleep_for(100ms);
        client.close();
      };
      res.emplace_back(pool->send_request(op));
    };
    for (int i = 0; i < 50; ++i) {
      event();
    }
    co_await collectAll(std::move(res));
    CHECK(pool->free_client_count() == 0);
    server.stop();
  }());
}

TEST_CASE("test client pools parallel r/w") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    auto pool = coro_io::client_pools<coro_rpc::coro_rpc_client>{};
    for (int i = 0; i < 10000; ++i) {
      auto cli = pool[std::to_string(i)];
      CHECK(cli->get_host_name() == std::to_string(i));
    }
    auto rw = [&pool](int i) -> Lazy<void> {
      auto cli = pool[std::to_string(i)];
      CHECK(cli->get_host_name() == std::to_string(i));
      co_return;
    };

    std::vector<RescheduleLazy<void>> works;
    for (int i = 0; i < 20000; ++i) {
      works.emplace_back(rw(i).via(coro_io::get_global_executor()));
    }
    for (int i = 0; i < 10000; ++i) {
      works.emplace_back(rw(i).via(coro_io::get_global_executor()));
    }
    co_await collectAll(std::move(works));
  }());
}