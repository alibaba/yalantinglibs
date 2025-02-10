#include <chrono>
#include <cstdio>
#include <ylt/coro_http/coro_http_server.hpp>
#include <ylt/metric.hpp>

#include "cmdline.h"
#include "ylt/coro_io/io_context_pool.hpp"

using namespace coro_http;

struct bench_config {
  std::string url;
  uint32_t client_concurrency;
  size_t data_len;
  size_t max_request_count;
  uint32_t thd_num;
  unsigned short port;
};

bench_config init_conf(const cmdline::parser& parser) {
  bench_config conf{};
  conf.url = parser.get<std::string>("url");
  conf.client_concurrency = parser.get<uint32_t>("client_concurrency");
  conf.data_len = parser.get<size_t>("data_len");
  conf.max_request_count = parser.get<size_t>("max_request_count");
  conf.thd_num = parser.get<uint32_t>("thd_num");
  conf.port = parser.get<unsigned short>("port");

  std::string method = (conf.data_len == 0) ? "GET" : "POST";

  ELOG_INFO << "url: " << conf.url << ", "
            << "client concurrency: " << conf.client_concurrency << ", "
            << "data_len: " << conf.data_len << ", "
            << "max_request_count: " << conf.max_request_count << ", "
            << "thd_num: " << conf.thd_num << ", "
            << "port: " << conf.port << ", method: " << method << ", ";

  return conf;
}

/*
start clients with get:
./coro_http_benchmark -c 64 -t 32 -u http://0.0.0.0:8090/plaintext
start clients with post:
./coro_http_benchmark -c 64 -t 32 -u http://0.0.0.0:8090/plaintext -l 12
only start server:
./coro_http_benchmark -t 10

test with local server:
./coro_http_benchmark -c 64 -t 32 -u http://0.0.0.0:8090/plaintext
*/
int main(int argc, char** argv) {
  easylog::set_min_severity(easylog::Severity::INFO);
  cmdline::parser parser;

  parser.add<std::string>("url", 'u', "url", false,
                          "http://0.0.0.0:8090/plaintext");
  parser.add<size_t>("data_len", 'l', "data length", false, 0);
  parser.add<uint32_t>("client_concurrency", 'c',
                       "total number of http clients", false, 0);
  parser.add<size_t>("max_request_count", 'm', "max request count", false,
                     100000);
  parser.add<uint32_t>("thd_num", 't', "server thread number", false,
                       std::thread::hardware_concurrency());
  parser.add<unsigned short>("port", 'p', "server port", false, 8090);

  parser.parse_check(argc, argv);
  auto conf = init_conf(parser);

  bool no_server = conf.thd_num == 0;
  bool no_client = conf.client_concurrency == 0;
  if (no_server && no_client) {
    ELOG_INFO << parser.usage();
    return 0;
  }

  coro_io::get_global_executor(conf.thd_num);

  std::shared_ptr<coro_http_server> server = nullptr;

  if (!no_server) {
    // init server
    server =
        std::make_shared<coro_http_server>(conf.thd_num, conf.port, "0.0.0.0");
    server->set_http_handler<GET, POST>(
        "/plaintext", [](request& req, response& resp) {
          resp.set_delay(false);
          resp.need_date_head(false);
          resp.set_status_and_content(status_type::ok, "Hello, world!");
        });

    if (no_client) {
      server->sync_start();
      return 0;
    }

    server->async_start();
  }

  // benchmark
  auto pool = coro_io::client_pool<coro_http_client>::create(conf.url);

  bool is_get = conf.data_len == 0;
  std::string send_data(conf.data_len, 'A');
  std::string_view url = conf.url;
  std::string_view data = send_data;

  ylt::metric::counter_t req_count("total_request", "");
  ylt::metric::counter_t latency("latency", "");

  auto lazy = [&]() -> async_simple::coro::Lazy<void> {
    for (size_t i = 0; i < conf.max_request_count; i++) {
      co_await pool->send_request(
          [&](coro_http_client& client) -> async_simple::coro::Lazy<void> {
            resp_data result;
            auto start = std::chrono::system_clock::now();
            if (is_get) {
              req_context<> ctx{};
              result = co_await client.async_request(url, http_method::GET,
                                                     std::move(ctx));
            }
            else {
              req_context<std::string_view> ctx{};
              ctx.content = data;
              result = co_await client.async_request(url, http_method::POST,
                                                     std::move(ctx));
            }
            auto end = std::chrono::system_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
                           end - start)
                           .count();
            latency.inc(dur);
            assert(result.status == 200);
            assert(result.resp_body == "Hello, world!");
            req_count.inc();
          });
    }
  };

  for (size_t i = 0; i < conf.client_concurrency; i++) {
    lazy().start([](auto&&) {
    });
  }

  size_t last = 0;
  int64_t last_latence = 0;
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto value = req_count.value();
    auto lat = latency.value();
    ELOG_INFO << "qps: " << value - last
              << ", latency: " << (lat - last_latence) / value << "us";
    last = value;
    last_latence = lat;
    if (value == conf.client_concurrency * conf.max_request_count) {
      ELOG_INFO << "benchmark finished";
      break;
    }
  }

  return 0;
}