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

TEST_CASE("test WRR") {
  SUBCASE(
      "exception tests: empty hosts, empty weights test or count not equal") {
    CHECK_THROWS_AS(
        coro_io::channel<coro_rpc::coro_rpc_client>::create(
            {}, {.lba = coro_io::load_blance_algorithm::WRR}, {2, 1}),
        std::invalid_argument);

    CHECK_THROWS_AS(coro_io::channel<coro_rpc::coro_rpc_client>::create(
                        {"127.0.0.1:8801", "127.0.0.1:8802"},
                        {.lba = coro_io::load_blance_algorithm::WRR}),
                    std::invalid_argument);

    CHECK_THROWS_AS(coro_io::channel<coro_rpc::coro_rpc_client>::create(
                        {"127.0.0.1:8801", "127.0.0.1:8802"},
                        {.lba = coro_io::load_blance_algorithm::WRR}, {1}),
                    std::invalid_argument);
  }

  coro_rpc::coro_rpc_server server1(1, 8801);
  auto res = server1.async_start();
  REQUIRE_MESSAGE(res, "server start failed");
  coro_rpc::coro_rpc_server server2(1, 8802);
  auto res2 = server2.async_start();
  REQUIRE_MESSAGE(res2, "server start failed");

  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    auto hosts =
        std::vector<std::string_view>{"127.0.0.1:8801", "127.0.0.1:8802"};
    auto channel = coro_io::channel<coro_rpc::coro_rpc_client>::create(
        hosts, {.lba = coro_io::load_blance_algorithm::WRR}, {2, 1});
    for (int i = 0; i < 6; ++i) {
      auto res = co_await channel.send_request(
          [&i, &hosts](
              coro_rpc::coro_rpc_client &client,
              std::string_view host) -> async_simple::coro::Lazy<void> {
            if (i == 0 || i == 1) {
              CHECK(host == hosts[0]);
            }
            else if (i == 2 || i == 5) {
              CHECK(host == hosts[1]);
            }
            else if (i == 3 || i == 4) {
              CHECK(host == hosts[0]);
            }
            co_return;
          });
      CHECK(res.has_value());
    }
  }());

  async_simple::coro::syncAwait([]() -> async_simple::coro::Lazy<void> {
    auto hosts =
        std::vector<std::string_view>{"127.0.0.1:8801", "127.0.0.1:8802"};
    auto channel = coro_io::channel<coro_rpc::coro_rpc_client>::create(
        hosts, {.lba = coro_io::load_blance_algorithm::WRR}, {0, 0});
    for (int i = 0; i < 6; ++i) {
      auto res = co_await channel.send_request(
          [&i, &hosts](
              coro_rpc::coro_rpc_client &client,
              std::string_view host) -> async_simple::coro::Lazy<void> {
            if (i % 2 == 0) {
              CHECK(host == hosts[0]);
            }
            else {
              CHECK(host == hosts[1]);
            }
            co_return;
          });
      CHECK(res.has_value());
    }
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
    coro_rpc::coro_rpc_server server(1, 9813);
    auto res = server.async_start();
    REQUIRE_MESSAGE(res, "server start failed");
    auto hosts = std::vector<std::string_view>{"127.0.0.1:9813"};
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