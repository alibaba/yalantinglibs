#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

#include "cmdline.h"
#include "ylt/coro_rpc/impl/protocol/coro_rpc_protocol.hpp"

struct bench_config {
  std::string url;
  uint32_t client_concurrency;
  size_t data_len;
  size_t max_request_count;
  unsigned short port;
  size_t resp_len;
  uint32_t buffer_size;
  int log_level;
  bool enable_ib;
};

bench_config init_conf(const cmdline::parser& parser) {
  bench_config conf{};
  conf.url = parser.get<std::string>("url");
  conf.client_concurrency = parser.get<uint32_t>("client_concurrency");
  conf.data_len = parser.get<size_t>("data_len");
  conf.max_request_count = parser.get<size_t>("max_request_count");
  conf.port = parser.get<unsigned short>("port");
  conf.resp_len = parser.get<size_t>("resp_len");
  conf.buffer_size = parser.get<uint32_t>("buffer_size");
  conf.log_level = parser.get<int>("log_level");
  conf.enable_ib = parser.get<bool>("enable_ib");

  ELOG_WARN << "url: " << conf.url << ", "
            << "client concurrency: " << conf.client_concurrency << ", "
            << "data_len: " << conf.data_len << ", "
            << "max_request_count: " << conf.max_request_count << ", "
            << "port: " << conf.port << ", "
            << "buffer_size: " << conf.buffer_size << ", "
            << "log level: " << conf.log_level << ", "
            << "enable ibverbs: " << conf.enable_ib << ", "
            << "resp_len: " << conf.resp_len;

  return conf;
}

size_t g_resp_len = 0;
std::string_view g_resp_str;
std::atomic<size_t> g_count = 0;

inline std::string_view echo() {
  auto str = coro_rpc::get_context()->get_request_attachment();
  if (g_resp_len == 0) {
    return str;
  }

  return g_resp_str;
}

async_simple::coro::Lazy<std::error_code> request(const bench_config& conf) {
  std::string send_str(conf.data_len, 'A');
  std::string_view send_str_view(send_str);

  ELOG_INFO << "bench_config buffer size " << conf.buffer_size;

  coro_io::client_pool<coro_rpc::coro_rpc_client>::pool_config pool_conf{};
#ifdef YLT_ENABLE_IBV
  if (conf.enable_ib) {
    coro_io::ibverbs_config ib_conf{};
    ib_conf.request_buffer_size = conf.buffer_size;
    pool_conf.client_config.socket_config = ib_conf;
  }
#endif

  auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
      conf.url, pool_conf);
  auto lazy = [&]() -> async_simple::coro::Lazy<void> {
    for (size_t i = 0; i < conf.max_request_count; i++) {
      co_await pool->send_request([&](coro_rpc::coro_rpc_client& client)
                                      -> async_simple::coro::Lazy<void> {
        client.set_req_attachment(send_str_view);
        auto result = co_await client.call<echo>();
        if (!result.has_value()) {
          ELOG_ERROR << result.error().msg;
          co_return;
        }
        g_count += send_str_view.length();
      });
    }
  };

  for (size_t i = 0; i < conf.client_concurrency; i++) {
    lazy().via(coro_io::get_global_executor()).start([](auto&&) {
    });
  }

  std::cout << std::fixed << std::setprecision(2);

  while (true) {
    auto start = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds{1});
    auto c = g_count.exchange(0);
    auto end = std::chrono::system_clock::now();
    auto dur = (end - start) / std::chrono::milliseconds(1);
    double val = (8.0 * c * 1000) / (1000'000'000ll * dur);
    std::cout << "Throughput:" << val << " Gb/s\n";
  }
}

int main(int argc, char** argv) {
  cmdline::parser parser;

  parser.add<std::string>("url", 'u', "url", false, "0.0.0.0:9000");
  parser.add<size_t>("data_len", 'l', "data length", false, 7340032);  // 7MB
  parser.add<uint32_t>("client_concurrency", 'c',
                       "total number of http clients", false, 0);
  parser.add<size_t>("max_request_count", 'm', "max request count", false,
                     100000000);

  parser.add<unsigned short>("port", 'p', "server port", false, 9000);
  parser.add<size_t>("resp_len", 'r', "response data length", false, 0);
  parser.add<uint32_t>("buffer_size", 'b', "buffer size", false, 8388608);
  parser.add<int>("log_level", 'o', "Severity::INFO 1 as default, WARN is 4",
                  false, 1);
  parser.add<bool>("enable_ib", 'i', "response data length", false, true);

  parser.parse_check(argc, argv);
  auto conf = init_conf(parser);

  easylog::set_min_severity((easylog::Severity)conf.log_level);

  easylog::set_async(true);

  if (conf.client_concurrency == 0) {
    std::cout << "start server\n";
    std::string resp_str;
    if (conf.resp_len != 0) {
      resp_str = std::string(conf.resp_len, 'A');
      g_resp_str = resp_str;
      g_resp_len = conf.resp_len;
    }
    coro_rpc::coro_rpc_server server(std::thread::hardware_concurrency(),
                                     conf.port, "0.0.0.0",
                                     std::chrono::seconds(10));
    server.register_handler<echo>();
#ifdef YLT_ENABLE_IBV
    if (conf.enable_ib)
      server.init_ibv();
#endif
    [[maybe_unused]] auto ret = server.start();
  }
  else {
    std::cout << "will start client\n";
    request(conf).start([](auto&& ec) {
      ELOG_ERROR << ec.value().message();
    });
  }
}
