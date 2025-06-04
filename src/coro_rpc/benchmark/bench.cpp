#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>
#include <ylt/metric/summary.hpp>

#include "cmdline.h"
#include "ylt/coro_rpc/impl/protocol/coro_rpc_protocol.hpp"

struct bench_config {
  std::string url;
  uint32_t client_concurrency;
  size_t send_data_len;
  size_t max_request_count;
  unsigned short port;
  size_t resp_len;
  uint32_t buffer_size;
  int log_level;
  bool enable_ib;
  uint32_t duration;
};

bench_config init_conf(const cmdline::parser& parser) {
  bench_config conf{};
  conf.url = parser.get<std::string>("url");
  conf.client_concurrency = parser.get<uint32_t>("client_concurrency");
  conf.max_request_count = parser.get<size_t>("max_request_count");
  conf.resp_len = parser.get<size_t>("resp_len");
  conf.send_data_len = parser.get<size_t>("send_data_len");
  conf.port = parser.get<unsigned short>("port");
  conf.buffer_size = parser.get<uint32_t>("buffer_size");
  conf.log_level = parser.get<int>("log_level");
  conf.enable_ib = parser.get<bool>("enable_ib");
  conf.duration = parser.get<uint32_t>("duration");

  if (conf.client_concurrency == 0) {
    ELOG_WARN << "port: " << conf.port << ", "
              << "buffer_size: " << conf.buffer_size << ", "
              << "resp_len: " << conf.resp_len << ", "
              << "test duration: " << conf.duration << "s, "
              << "enable ibverbs: " << conf.enable_ib << ", "
              << "log level: " << conf.log_level << ", ";
  }
  else {
    ELOG_WARN << "url: " << conf.url << ", "
              << "buffer_size: " << conf.buffer_size << ", "
              << "client concurrency: " << conf.client_concurrency << ", "
              << "send data_len: " << conf.send_data_len << ", "
              << "max_request_count: " << conf.max_request_count << ", "
              << "enable ibverbs: " << conf.enable_ib << ", "
              << "log level: " << conf.log_level << ", ";
  }
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

ylt::metric::summary_t g_latency{"rpc call latency(us)", "help",
                                 std::vector{0.5, 0.9, 0.95, 0.99},
                                 std::chrono::seconds{60}};

async_simple::coro::Lazy<std::error_code> request(const bench_config& conf) {
  ELOG_INFO << "bench_config buffer size " << conf.buffer_size;

  coro_io::client_pool<coro_rpc::coro_rpc_client>::pool_config pool_conf{};
#ifdef YLT_ENABLE_IBV
  if (conf.enable_ib) {
    pool_conf.client_config.socket_config = coro_io::ibverbs_config{};
  }
#endif

  std::shared_ptr<std::atomic<bool>> finish =
      std::make_shared<std::atomic<bool>>();
  auto pool = coro_io::client_pool<coro_rpc::coro_rpc_client>::create(
      conf.url, pool_conf);
  auto lazy = [pool, conf, finish]() -> async_simple::coro::Lazy<void> {
    std::string send_str(conf.send_data_len, 'A');
    std::string_view send_str_view(send_str);
    for (size_t i = 0; i < conf.max_request_count; i++) {
      if (finish->load(std::memory_order_acquire)) {
        break;
      }
      co_await pool->send_request([&](coro_rpc::coro_rpc_client& client)
                                      -> async_simple::coro::Lazy<void> {
        client.set_req_attachment(send_str_view);
        auto start = std::chrono::steady_clock::now();
        auto result = co_await client.call<echo>();
        if (!result.has_value()) {
          ELOG_ERROR << result.error().msg;
          co_return;
        }
        auto now = std::chrono::steady_clock::now();
        g_latency.observe(
            std::chrono::duration_cast<std::chrono::microseconds>(now - start)
                .count());
        g_count += send_str_view.length();
      });
    }
  };

  for (size_t i = 0; i < conf.client_concurrency; i++) {
    lazy().start([](auto&&) {
    });
  }

  std::cout << std::fixed << std::setprecision(2);

  size_t total = 0;
  for (uint32_t i = 0; i < conf.duration; i++) {
    auto start = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds{1});
    auto c = g_count.exchange(0);
    total += c;
    auto end = std::chrono::system_clock::now();
    auto dur = (end - start) / std::chrono::milliseconds(1);
    double val = (8.0 * c * 1000) / (1000'000'000ll * dur);

    std::cout << "qps " << c / conf.send_data_len << ", Throughput:" << val
              << " Gb/s\n";
  }

  finish->store(true, std::memory_order_release);

  double val = (8.0 * total) / (1000'000'000ll * conf.duration);
  std::cout << "avg qps " << total / (conf.send_data_len * conf.duration)
            << " and throughput:" << val << " Gb/s in duration "
            << conf.duration << "\n";
  std::string str_rate;
  g_latency.serialize(str_rate);
  std::cout << "latency: " << str_rate << "\n";

  co_return std::error_code{};
}

int main(int argc, char** argv) {
  cmdline::parser parser;

  parser.add<std::string>("url", 'u', "url", false, "0.0.0.0:9000");
  parser.add<size_t>("send_data_len", 's', "send data length", false,
                     1024 * 1024 * 8);  // 8MB
  parser.add<uint32_t>("client_concurrency", 'c',
                       "total number of http clients", false, 0);
  parser.add<size_t>("max_request_count", 'm', "max request count", false,
                     100000000);

  parser.add<unsigned short>("port", 'p', "server port", false, 9000);
  parser.add<size_t>("resp_len", 'r', "response data length", false, 13);
  parser.add<uint32_t>("buffer_size", 'b', "buffer size", false, 1024 * 1024 * 2);
  parser.add<int>("log_level", 'o', "Severity::INFO 1 as default, WARN is 4",
                  false, 1);
  parser.add<bool>("enable_ib", 'i', "enable ib", false, true);
  parser.add<uint32_t>("duration", 'd', "duration seconds", false, 100000);

  parser.parse_check(argc, argv);
  auto conf = init_conf(parser);

  easylog::set_min_severity((easylog::Severity)conf.log_level);

  easylog::set_async(true);

#ifdef YLT_ENABLE_IBV
  coro_io::g_ib_buffer_pool({.buffer_size = conf.buffer_size});
#endif

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
    async_simple::coro::syncAwait(request(conf));
  }
}