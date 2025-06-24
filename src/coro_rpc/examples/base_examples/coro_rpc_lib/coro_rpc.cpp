#include "coro_rpc.h"

#include <asio/ip/host_name.hpp>
#include <ylt/coro_io/load_balancer.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

inline char *create_copy_cstr(std::string msg) {
  char *buf = (char *)malloc(msg.size() + 1);
  memcpy(buf, msg.data(), msg.size());
  buf[msg.size()] = '\0';
  return buf;
}

inline void set_rpc_result(rpc_result &ret, coro_rpc::rpc_error ec) {
  ret.ec = ec.val();
  ret.err_msg = create_copy_cstr(ec.msg);
}

// rpc server
inline void ylt_load_service(coro_rpc::context<void> context, uint64_t req_id) {
  auto ctx_ptr = std::make_unique<coro_rpc::context<void>>(std::move(context));
  load_service(ctx_ptr.release(), req_id);
}

void response_error(void *ctx, uint16_t err_code, const char *err_msg) {
  std::unique_ptr<coro_rpc::context<void>> context(
      (coro_rpc::context<void> *)ctx);

  context->response_error(coro_rpc::err_code(err_code), err_msg);
}

void *response_msg(void *ctx, char *msg, uint64_t size) {
  std::unique_ptr<coro_rpc::context<void>> context(
      (coro_rpc::context<void> *)ctx);
  // wait for response finish;
  auto promise = std::make_unique<std::promise<std::error_code>>();
  context->get_context_info()->set_complete_handler(
      [p = promise.get()](const std::error_code &ec, std::size_t) {
        p->set_value(ec);
      });

  context->get_context_info()->set_response_attachment(
      std::string_view(msg, size));
  context->response_msg();
  return promise.release();
}

rpc_result wait_response_finish(void *p) {
  auto promis = (std::promise<std::error_code> *)p;
  auto ec = promis->get_future().get();
  delete promis;
  if (!ec) {
    return {};
  }

  char *buf = create_copy_cstr(ec.message());
  return rpc_result{0, buf, 0};
}

void *start_rpc_server(char *addr, int parallel, bool enable_ib) {
  auto server = std::make_unique<coro_rpc::coro_rpc_server>(
      parallel, addr, std::chrono::seconds(600));
  server->register_handler<ylt_load_service>();
#ifdef YLT_ENABLE_IBV
  if (enable_ib)
    server->init_ibv();
#endif
  auto res = server->async_start();
  if (res.hasResult()) {
    ELOG_ERROR << "start server failed";
    return nullptr;
  }

  return server.release();
}

void stop_rpc_server(void *server) {
  if (server != nullptr) {
    auto s = (coro_rpc::coro_rpc_server *)server;
    s->stop();
    delete s;
  }
}

// rpc client
void *create_client_pool(char *addr, client_config conf) {
  std::vector<std::string_view> hosts{std::string_view(addr)};
  coro_io::client_pool<coro_rpc::coro_rpc_client>::pool_config pool_conf{};
#ifdef YLT_ENABLE_IBV
  if (conf.enable_ib) {
    coro_io::ib_socket_t::config_t ib_conf{};
    pool_conf.client_config.socket_config = ib_conf;
  }
#endif
  if (conf.connect_timeout_sec != 0) {
    pool_conf.client_config.connect_timeout_duration =
        std::chrono::seconds{conf.connect_timeout_sec};
  }

  if (conf.req_timeout_sec != 0) {
    pool_conf.client_config.request_timeout_duration =
        std::chrono::seconds{conf.req_timeout_sec};
  }

  if (conf.local_ip == nullptr) {
    pool_conf.client_config.local_ip = "localhost";
  }
  else {
    pool_conf.client_config.local_ip = conf.local_ip;
  }

  ELOG_INFO << "client config connect timeout seconds: "
            << conf.connect_timeout_sec
            << ", request timeout seconds: " << conf.req_timeout_sec
            << ", local_ip: " << pool_conf.client_config.local_ip
            << ", enable ibverbs: " << conf.enable_ib;

  auto ld = coro_io::load_balancer<coro_rpc::coro_rpc_client>::create(
      hosts, {pool_conf});
  auto ld_ptr = std::make_unique<decltype(ld)>(std::move(ld));
  return ld_ptr.release();
}

void free_client_pool(void *pool) {
  auto ptr = (coro_io::load_balancer<coro_rpc::coro_rpc_client> *)pool;
  delete ptr;
}

rpc_result load(void *pool, uint64_t req_id, char *dest, uint64_t dest_len) {
  using namespace async_simple::coro;
  rpc_result ret{};
  if (dest == nullptr || dest_len == 0) {
    set_rpc_result(ret,
                   coro_rpc::rpc_error(coro_rpc::errc::invalid_rpc_arguments));
    return ret;
  }

  auto ld = (coro_io::load_balancer<coro_rpc::coro_rpc_client> *)pool;
  auto lazy = [&]() -> Lazy<void> {
    auto result =
        co_await ld->send_request([&](coro_rpc::coro_rpc_client &client,
                                      std::string_view hostname) -> Lazy<void> {
          client.set_resp_attachment_buf(std::span<char>(dest, dest_len));
          auto result = co_await client.call<ylt_load_service>(req_id);
          if (!result) {
            set_rpc_result(ret, result.error());
            co_return;
          }

          if (!client.is_resp_attachment_in_external_buf()) {
            set_rpc_result(
                ret, coro_rpc::rpc_error(coro_rpc::errc::message_too_large));
            co_return;
          }
          ret.len = client.get_resp_attachment().size();
        });
  };

  syncAwait(lazy());

  return ret;
}

// log
void init_rpc_log(char *log_filename, int log_level, uint64_t max_file_size,
                  uint64_t max_file_num, bool async) {
  if (log_filename == nullptr) {
    easylog::set_min_severity((easylog::Severity)log_level);
    ELOG_INFO << "init log, log_level: " << log_level;
    return;
  }

  std::string filename(log_filename);
  easylog::init_log((easylog::Severity)log_level, filename, async, true,
                    max_file_size, max_file_num, false);
  ELOG_INFO << "init log " << log_level << ", " << filename << ","
            << max_file_size << "," << max_file_num;
}

void flush_rpc_log() { easylog::flush(); }

char *get_first_local_ip() {
  std::string local_ip = "localhost";
  using asio::ip::tcp;
  tcp::resolver resolver(coro_io::get_global_executor()->get_asio_executor());
  tcp::resolver::query query(asio::ip::host_name(), "");
  tcp::resolver::iterator iter = resolver.resolve(query);
  tcp::resolver::iterator end;  // End marker.
  while (iter != end) {
    tcp::endpoint ep = *iter++;
    auto addr = ep.address();
    if (addr.is_v4()) {
      local_ip = addr.to_string();
      break;
    }
  }

  return create_copy_cstr(local_ip);
}