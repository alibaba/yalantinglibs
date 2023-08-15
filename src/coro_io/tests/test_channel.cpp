#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>
#include <doctest.h>

#include <asio/io_context.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <system_error>
#include <thread>
#include <ylt/coro_io/channel.hpp>
#include <ylt/coro_io/coro_file.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_io/io_context_pool.hpp>

#include "async_simple/coro/Lazy.h"
#include "ylt/coro_rpc/impl/coro_rpc_client.hpp"
#include "ylt/coro_rpc/impl/default_config/coro_rpc_config.hpp"

TEST_CASE("test RR") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    auto res = server.async_start();
    REQUIRE_MESSAGE(res, "server start failed");
    auto hosts =
        std::vector<std::string_view>{"127.0.0.1:8801", "localhost:8801"};
    auto channel = coro_io::channel<coro_rpc::coro_rpc_client>::create(hosts);
    for (int i = 0; i < 100; ++i) {
      auto res = co_await channel.send_request(
          [&i, &hosts](
              coro_rpc::coro_rpc_client &client,
              std::string_view host) -> async_simple::coro::Lazy<void> {
            CHECK(host == hosts[i % 2]);
            co_return;
          });
      CHECK(res.has_value());
    }
    server.stop();
  }());
}

TEST_CASE("test Random") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    auto res = server.async_start();
    REQUIRE_MESSAGE(res, "server start failed");
    auto hosts =
        std::vector<std::string_view>{"127.0.0.1:8801", "localhost:8801"};
    auto channel = coro_io::channel<coro_rpc::coro_rpc_client>::create(
        hosts, {.lba = coro_io::load_blance_algorithm::random});
    int host0_cnt = 0, hostRR_cnt = 0;
    for (int i = 0; i < 100; ++i) {
      auto res = co_await channel.send_request(
          [&i, &hosts, &host0_cnt, &hostRR_cnt](
              coro_rpc::coro_rpc_client &client,
              std::string_view host) -> async_simple::coro::Lazy<void> {
            if (host == hosts[i % 2])
              ++hostRR_cnt;
            if (host == hosts[0])
              ++host0_cnt;
            co_return;
          });
      CHECK(res.has_value());
    }
    CHECK(host0_cnt < 100);
    CHECK(hostRR_cnt < 100);
    server.stop();
  }());
}

TEST_CASE("test single host") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8801);
    auto res = server.async_start();
    REQUIRE_MESSAGE(res, "server start failed");
    auto hosts = std::vector<std::string_view>{"127.0.0.1:8801"};
    auto channel = coro_io::channel<coro_rpc::coro_rpc_client>::create(hosts);
    for (int i = 0; i < 100; ++i) {
      auto res = co_await channel.send_request(
          [&hosts](coro_rpc::coro_rpc_client &client,
                   std::string_view host) -> async_simple::coro::Lazy<void> {
            CHECK(host == hosts[0]);
            co_return;
          });
      CHECK(res.has_value());
    }
    server.stop();
  }());
}

TEST_CASE("test send_request config") {
  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    coro_rpc::coro_rpc_server server(1, 8802);
    auto res = server.async_start();
    REQUIRE_MESSAGE(res, "server start failed");
    auto hosts = std::vector<std::string_view>{"127.0.0.1:8802"};
    auto channel = coro_io::channel<coro_rpc::coro_rpc_client>::create(hosts);
    for (int i = 0; i < 100; ++i) {
      auto config = coro_rpc::coro_rpc_client::config{.client_id = 114514};
      auto res = co_await channel.send_request(
          [&i, &hosts](coro_rpc::coro_rpc_client &client, std::string_view host)
              -> async_simple::coro::Lazy<void> {
            CHECK(client.get_client_id() == 114514);
            co_return;
          },
          config);
      CHECK(res.has_value());
    }
    server.stop();
  }());
}