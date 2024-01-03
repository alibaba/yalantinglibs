#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>
#include <doctest.h>

#include <asio/io_context.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
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
template <typename T = coro_rpc::coro_rpc_client>
async_simple::coro::Lazy<bool> event(
    int lim, coro_io::client_pool<T> &pool, ConditionVariable<SpinLock> &cv,
    SpinLock &lock,
    std::function<void(coro_rpc::coro_rpc_client &client)> user_op =
        [](auto &client) {
        }) {
  std::vector<RescheduleLazy<bool>> works;
  int64_t cnt = 0;
  for (int i = 0; i < lim; ++i) {
    auto op = [&cnt, &lock, &cv, &lim,
               &user_op](T &client) -> async_simple::coro::Lazy<void> {
      user_op(client);
      auto l = co_await lock.coScopedLock();
      if (++cnt < lim) {
        co_await cv.wait(lock, [&cnt, &lim] {
          return cnt >= lim;
        });
      }
      else {
        l.unlock();
        cv.notifyAll();
      }
      co_return;
    };
    auto backer = [&cv, &lock, &cnt, &lim](
                      coro_io::client_pool<T> &pool,
                      auto op) -> async_simple::coro::Lazy<bool> {
      async_simple::Promise<bool> p;
      auto res = co_await pool.send_request(op);
      if (!res.has_value()) {
        {
          co_await lock.coScopedLock();
          cnt = lim;
        }
        cv.notifyAll();
        co_return false;
      }
      co_return true;
    };
    works.emplace_back(backer(pool, op).via(coro_io::get_global_executor()));
  }
  auto res = co_await collectAll(std::move(works));
  for (auto &e : res) {
    if (!e.value()) {
      co_return false;
    }
  }
  co_return true;
};
TEST_CASE("test client pool") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    auto is_started = server.async_start();
    REQUIRE(is_started);
    auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
        "127.0.0.1:8801", {.max_connection = 100,
                           .idle_timeout = 300ms,
                           .short_connect_idle_timeout = 50ms});
    SpinLock lock;
    ConditionVariable<SpinLock> cv;
    auto res = co_await event(20, *pool, cv, lock);
    CHECK(res);
    CHECK(pool->free_client_count() == 20);
    res = co_await event(200, *pool, cv, lock);
    CHECK(res);
    auto sz = pool->free_client_count();
    CHECK(sz == 200);
    co_await coro_io::sleep_for(150ms);
    sz = pool->free_client_count();
    CHECK((sz >= 100 && sz <= 105));
    co_await coro_io::sleep_for(550ms);
    CHECK(pool->free_client_count() == 0);
    server.stop();
  }());
  ELOG_DEBUG << "test client pool over.";
}
TEST_CASE("test idle timeout yield") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    auto is_started = server.async_start();
    REQUIRE(is_started);
    auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
        "127.0.0.1:8801", {.max_connection = 100,
                           .idle_queue_per_max_clear_count = 1,
                           .idle_timeout = 300ms});
    SpinLock lock;
    ConditionVariable<SpinLock> cv;
    auto res = co_await event(100, *pool, cv, lock);
    CHECK(res);
    CHECK(pool->free_client_count() == 100);
    co_await coro_io::sleep_for(700ms);
    CHECK(pool->free_client_count() == 0);
    server.stop();
  }());
  ELOG_DEBUG << "test idle timeout yield over.";
}

TEST_CASE("test reconnect") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
        "127.0.0.1:8801",
        {.connect_retry_count = 3, .reconnect_wait_time = 500ms});
    SpinLock lock;
    ConditionVariable<SpinLock> cv;
    coro_rpc::coro_rpc_server server(2, 8801);
    async_simple::Promise<async_simple::Unit> p;
    coro_io::sleep_for(700ms).start([&server, &p](auto &&) {
      auto server_is_started = server.async_start();
      REQUIRE(server_is_started);
    });

    auto res = co_await event(100, *pool, cv, lock);
    CHECK(res);
    CHECK(pool->free_client_count() == 100);
    server.stop();
    co_return;
  }());
  ELOG_DEBUG << "test reconnect over.";
}

struct mock_client : public coro_rpc::coro_rpc_client {
  using coro_rpc::coro_rpc_client::coro_rpc_client;
  async_simple::coro::Lazy<coro_rpc::errc> reconnect(
      const std::string &hostname) {
    auto ec = co_await this->coro_rpc::coro_rpc_client::reconnect(hostname);
    if (ec) {
      co_await coro_io::sleep_for(300ms);
    }
    co_return ec;
  }
};
TEST_CASE("test reconnect retry wait time exclude reconnect cost time") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    auto tp = std::chrono::steady_clock::now();
    auto pool = coro_io::client_pool<mock_client>::create(
        "127.0.0.1:8801",
        {.connect_retry_count = 3, .reconnect_wait_time = 500ms});
    SpinLock lock;
    ConditionVariable<SpinLock> cv;
    coro_rpc::coro_rpc_server server(2, 8801);
    async_simple::Promise<async_simple::Unit> p;
    coro_io::sleep_for(350ms).start([&server, &p](auto &&) {
      auto server_is_started = server.async_start();
      REQUIRE(server_is_started);
    });
    auto res = co_await event<mock_client>(100, *pool, cv, lock);
    CHECK(res);
    CHECK(pool->free_client_count() == 100);
    auto dur = std::chrono::steady_clock::now() - tp;
    std::cout << dur.count() << std::endl;
    CHECK((dur >= 500ms && dur <= 800ms));
    server.stop();
    co_return;
  }());
  ELOG_DEBUG
      << "test reconnect retry wait time exclude reconnect cost time over.";
}

TEST_CASE("test collect_free_client") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    auto is_started = server.async_start();
    REQUIRE(is_started);
    auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
        "127.0.0.1:8801", {.max_connection = 100, .idle_timeout = 300ms});

    SpinLock lock;
    ConditionVariable<SpinLock> cv;
    auto res = co_await event(50, *pool, cv, lock, [](auto &client) {
      client.close();
    });
    CHECK(res);
    CHECK(pool->free_client_count() == 0);
    server.stop();
    co_return;
  }());
  ELOG_DEBUG << "test collect_free_client over.";
}

TEST_CASE("test client pools parallel r/w") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    auto pool = coro_io::client_pools<coro_rpc::coro_rpc_client>{};
    for (int i = 0; i < 10000; ++i) {
      auto cli = pool[std::to_string(i)];
      CHECK(cli->get_host_name() == std::to_string(i));
    }
    auto rw = [&pool](int i) -> Lazy<void> {
      //      ELOG_DEBUG << "start to insert {" << i << "} to hash table.";
      auto cli = pool[std::to_string(i)];
      CHECK(cli->get_host_name() == std::to_string(i));
      //      ELOG_DEBUG << "end to insert {" << i << "} to hash table.";
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
  ELOG_DEBUG << "test client pools parallel r/w over.";
}