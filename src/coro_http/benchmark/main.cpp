#include <ylt/coro_http/coro_http_server.hpp>
#include <ylt/metric.hpp>

#include "cmdline.h"

using namespace coro_http;

struct bench_config {
  int client_num;
  std::string url;
  int data_size;
  int server_thd_num;
  int port;
};

bench_config init_conf(const cmdline::parser& parser) {
  bench_config conf{};
  conf.client_num = parser.get<int>("client_num");
  conf.url = parser.get<std::string>("url");
  conf.data_size = parser.get<int>("data_size");
  conf.server_thd_num = parser.get<int>("server_thd_num");
  conf.port = parser.get<int>("port");

  CINATRA_LOG_INFO << "client_num: " << conf.client_num << ", "
                   << "url: " << conf.url << ", "
                   << "data_size: " << conf.data_size << ", "
                   << "server_thd_num: " << conf.server_thd_num << ", "
                   << "port: " << conf.port;

  return conf;
}

/*
start clients with get:
./coro_http_benchmark -c 100 -u http://0.0.0.0:8090/plaintext
start clients with post:
./coro_http_benchmark -c 100 -u http://0.0.0.0:8090/plaintext -d 12
only start server:
./coro_http_benchmark -s 10

test with local server:
./coro_http_benchmark -c 100 -u http://0.0.0.0:8090/plaintext -s 10
*/
int main(int argc, char** argv) {
  cmdline::parser parser;
  parser.add<int>("client_num", 'c', "total number of http clients", false, 0);
  parser.add<std::string>("url", 'u', "url", false,
                          "http://0.0.0.0:8090/plaintext");
  parser.add<std::string>("method", 'm', "http method", false, "GET");
  parser.add<int>("data_size", 'd', "data size", false, 0);
  parser.add<int>("server_thd_num", 's', "server thread number", false, 0);
  parser.add<int>("port", 'p', "server port", false, 8090);

  parser.parse_check(argc, argv);
  auto conf = init_conf(parser);

  bool no_server = conf.server_thd_num == 0;
  bool no_client = conf.client_num == 0;
  if (no_server && no_client) {
    CINATRA_LOG_INFO << parser.usage();
    return 0;
  }

  std::shared_ptr<coro_http_server> server = nullptr;

  if (!no_server) {
    // init server
    server = std::make_shared<coro_http_server>(
        conf.server_thd_num, (unsigned short)conf.port, "0.0.0.0");
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

  return 0;
}