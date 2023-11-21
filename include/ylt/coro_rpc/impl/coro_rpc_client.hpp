/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <async_simple/Future.h>
#include <async_simple/coro/FutureAwaiter.h>
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/SyncAwait.h>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <ylt/easylog.hpp>

#include "asio/dispatch.hpp"
#include "common_service.hpp"
#include "context.hpp"
#include "expected.hpp"
#include "protocol/coro_rpc_protocol.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/struct_pack.hpp"
#include "ylt/util/function_name.h"
#include "ylt/util/type_traits.h"
#include "ylt/util/utils.hpp"
#ifdef UNIT_TEST_INJECT
#include "inject_action.hpp"
#endif

#ifdef GENERATE_BENCHMARK_DATA
#include <fstream>
#endif
namespace coro_io {
template <typename T, typename U>
class client_pool;
}

namespace coro_rpc {

#ifdef GENERATE_BENCHMARK_DATA
std::string benchmark_file_path = "./";
#endif

class coro_connection;

template <typename T>
struct rpc_return_type {
  using type = T;
};
template <>
struct rpc_return_type<void> {
  using type = std::monostate;
};
template <typename T>
using rpc_return_type_t = typename rpc_return_type<T>::type;
/*!
 * ```cpp
 * #include <ylt/coro_rpc/coro_rpc_client.hpp>
 *
 * using namespace coro_rpc;
 * using namespace async_simple::coro;
 *
 * Lazy<void> show_rpc_call(coro_rpc_client &client) {
 *   auto ec = co_await client.connect("127.0.0.1", "8801");
 *   assert(ec == std::errc{});
 *   auto result = co_await client.call<hello_coro_rpc>();
 *   if (!result) {
 *     std::cout << "err: " << result.error().msg << std::endl;
 *   }
 *   assert(result.value() == "hello coro_rpc"s);
 * }
 *
 * int main() {
 *   coro_rpc_client client;
 *   syncAwait(show_rpc_call(client));
 * }
 * ```
 */
class coro_rpc_client {
  using coro_rpc_protocol = coro_rpc::protocol::coro_rpc_protocol;

 public:
  const inline static coro_rpc_protocol::rpc_error connect_error = {
      std::errc::io_error, "client has been closed"};
  struct config {
    uint32_t client_id = 0;
    std::chrono::milliseconds timeout_duration =
        std::chrono::milliseconds{5000};
    std::string host;
    std::string port;
#ifdef YLT_ENABLE_SSL
    std::filesystem::path ssl_cert_path;
    std::string ssl_domain;
#endif
  };

  /*!
   * Create client with io_context
   * @param io_context asio io_context, async event handler
   */
  coro_rpc_client(asio::io_context::executor_type executor,
                  uint32_t client_id = 0)
      : executor(executor),
        socket_(std::make_shared<asio::ip::tcp::socket>(executor)) {
    config_.client_id = client_id;
    read_buf_.resize(default_read_buf_size_);
  }

  /*!
   * Create client with io_context
   * @param io_context asio io_context, async event handler
   */
  coro_rpc_client(
      coro_io::ExecutorWrapper<> &executor = *coro_io::get_global_executor(),
      uint32_t client_id = 0)
      : executor(executor.get_asio_executor()),
        socket_(std::make_shared<asio::ip::tcp::socket>(
            executor.get_asio_executor())) {
    config_.client_id = client_id;
    read_buf_.resize(default_read_buf_size_);
  }

  std::string_view get_host() const { return config_.host; }

  std::string_view get_port() const { return config_.port; }

  [[nodiscard]] bool init_config(const config &conf) {
    config_ = conf;
#ifdef YLT_ENABLE_SSL
    if (!config_.ssl_cert_path.empty())
      return init_ssl_impl();
    else
#endif
      return true;
  };

  /*!
   * Check the client closed or not
   *
   * @return true if client closed, otherwise false.
   */
  [[nodiscard]] bool has_closed() { return has_closed_; }

  /*!
   * Reconnect server
   *
   * If connect hasn't been closed, it will be closed first then connect to
   * server, else the client will connect to server directly
   *
   * @param host server address
   * @param port server port
   * @param timeout_duration RPC call timeout
   * @return error code
   */
  [[nodiscard]] async_simple::coro::Lazy<std::errc> reconnect(
      std::string host, std::string port,
      std::chrono::steady_clock::duration timeout_duration =
          std::chrono::seconds(5)) {
    config_.host = std::move(host);
    config_.port = std::move(port);
    config_.timeout_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    reset();
    return connect(is_reconnect_t{true});
  }

  [[nodiscard]] async_simple::coro::Lazy<std::errc> reconnect(
      std::string endpoint,
      std::chrono::steady_clock::duration timeout_duration =
          std::chrono::seconds(5)) {
    auto pos = endpoint.find(':');
    config_.host = endpoint.substr(0, pos);
    config_.port = endpoint.substr(pos + 1);
    config_.timeout_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    reset();
    return connect(is_reconnect_t{true});
  }
  /*!
   * Connect server
   *
   * If connect hasn't been closed, it will be closed first then connect to
   * server, else the client will connect to server directly
   *
   * @param host server address
   * @param port server port
   * @param timeout_duration RPC call timeout
   * @return error code
   */
  [[nodiscard]] async_simple::coro::Lazy<std::errc> connect(
      std::string host, std::string port,
      std::chrono::steady_clock::duration timeout_duration =
          std::chrono::seconds(5)) {
    config_.host = std::move(host);
    config_.port = std::move(port);
    config_.timeout_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    return connect();
  }
  [[nodiscard]] async_simple::coro::Lazy<std::errc> connect(
      std::string_view endpoint,
      std::chrono::steady_clock::duration timeout_duration =
          std::chrono::seconds(5)) {
    auto pos = endpoint.find(':');
    config_.host = endpoint.substr(0, pos);
    config_.port = endpoint.substr(pos + 1);
    config_.timeout_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    return connect();
  }

#ifdef YLT_ENABLE_SSL

  [[nodiscard]] bool init_ssl(std::string_view cert_base_path,
                              std::string_view cert_file_name,
                              std::string_view domain = "localhost") {
    config_.ssl_cert_path =
        std::filesystem::path(cert_base_path).append(cert_file_name);
    config_.ssl_domain = domain;
    return init_ssl_impl();
  }
#endif

  ~coro_rpc_client() { close(); }

  /*!
   * Call RPC function with default timeout (5 second)
   *
   * @tparam func the address of RPC function
   * @tparam Args the type of arguments
   * @param args RPC function arguments
   * @return RPC call result
   */
  template <auto func, typename... Args>
  async_simple::coro::Lazy<
      rpc_result<decltype(get_return_type<func>()), coro_rpc_protocol>>
  call(Args... args) {
    return call_for<func>(std::chrono::seconds(5), std::move(args)...);
  }

  /*!
   * Call RPC function
   *
   * Timeout must be set explicitly.
   *
   * @tparam func the address of RPC function
   * @tparam Args the type of arguments
   * @param duration RPC call timeout
   * @param args RPC function arguments
   * @return RPC call result
   */
  template <auto func, typename... Args>
  async_simple::coro::Lazy<
      rpc_result<decltype(get_return_type<func>()), coro_rpc_protocol>>
  call_for(auto duration, Args... args) {
    using R = decltype(get_return_type<func>());

    if (has_closed_)
      AS_UNLIKELY {
        ELOGV(ERROR, "client has been closed, please re-connect");
        auto ret = rpc_result<R, coro_rpc_protocol>{
            unexpect_t{}, coro_rpc_protocol::rpc_error{
                              std::errc::io_error,
                              "client has been closed, please re-connect"}};
        co_return ret;
      }

    rpc_result<R, coro_rpc_protocol> ret;
#ifdef YLT_ENABLE_SSL
    if (!ssl_init_ret_) {
      ret = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::not_connected,
                                                     "not connected"}};
      co_return ret;
    }
#endif

    static_check<func, Args...>();

    async_simple::Promise<async_simple::Unit> promise;
    coro_io::period_timer timer(&executor);
    timeout(timer, duration, promise, "rpc call timer canceled")
        .via(&executor)
        .detach();

#ifdef YLT_ENABLE_SSL
    if (!config_.ssl_cert_path.empty()) {
      assert(ssl_stream_);
      ret = co_await call_impl<func>(*ssl_stream_, std::move(args)...);
    }
    else {
#endif
      ret = co_await call_impl<func>(*socket_, std::move(args)...);
#ifdef YLT_ENABLE_SSL
    }
#endif

    std::error_code err_code;
    timer.cancel(err_code);

    if (is_timeout_) {
      ret = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::timed_out,
                                                     "rpc call timed out"}};
    }

    co_await promise.getFuture();
#ifdef UNIT_TEST_INJECT
    ELOGV(INFO, "client_id %d call %s %s", config_.client_id,
          get_func_name<func>().data(), ret ? "ok" : "failed");
#endif
    co_return ret;
  }

  /*!
   * Get inner executor
   */
  auto &get_executor() { return executor; }

  uint32_t get_client_id() const { return config_.client_id; }

  void close() {
    if (has_closed_) {
      return;
    }

    ELOGV(INFO, "client_id %d close", config_.client_id);

    close_socket(socket_);

    has_closed_ = true;
  }

  template <typename T, typename U>
  friend class coro_io::client_pool;

 private:
  // the const char * will convert to bool instead of std::string_view
  // use this struct to prevent it.
  struct is_reconnect_t {
    bool value = false;
  };

  void reset() {
    close_socket(socket_);
    socket_ =
        std::make_shared<asio::ip::tcp::socket>(executor.get_asio_executor());
    is_timeout_ = false;
    has_closed_ = false;
  }

  static bool is_ok(std::errc ec) noexcept { return ec == std::errc{}; }
  [[nodiscard]] async_simple::coro::Lazy<std::errc> connect(
      is_reconnect_t is_reconnect = is_reconnect_t{false}) {
#ifdef YLT_ENABLE_SSL
    if (!ssl_init_ret_) {
      std::cout << "ssl_init_ret_: " << ssl_init_ret_ << std::endl;
      co_return std::errc::not_connected;
    }
#endif
    if (!is_reconnect.value && has_closed_)
      AS_UNLIKELY {
        ELOGV(ERROR,
              "a closed client is not allowed connect again, please use "
              "reconnect function or create a new "
              "client");
        co_return std::errc::io_error;
      }
    has_closed_ = false;

    ELOGV(INFO, "client_id %d begin to connect %s", config_.client_id,
          config_.port.data());
    async_simple::Promise<async_simple::Unit> promise;
    coro_io::period_timer timer(&executor);
    timeout(timer, config_.timeout_duration, promise, "connect timer canceled")
        .via(&executor)
        .detach();

    std::error_code ec = co_await coro_io::async_connect(
        &executor, *socket_, config_.host, config_.port);
    std::error_code err_code;
    timer.cancel(err_code);

    co_await promise.getFuture();
    if (ec) {
      if (is_timeout_) {
        co_return std::errc::timed_out;
      }
      co_return std::errc::not_connected;
    }

    if (is_timeout_) {
      ELOGV(WARN, "client_id %d connect timeout", config_.client_id);
      co_return std::errc::timed_out;
    }

#ifdef YLT_ENABLE_SSL
    if (!config_.ssl_cert_path.empty()) {
      assert(ssl_stream_);
      auto shake_ec = co_await coro_io::async_handshake(
          ssl_stream_, asio::ssl::stream_base::client);
      if (shake_ec) {
        ELOGV(WARN, "client_id %d handshake failed: %s", config_.client_id,
              shake_ec.message().data());
        co_return std::errc::not_connected;
      }
    }
#endif

    co_return std::errc{};
  };
#ifdef YLT_ENABLE_SSL
  [[nodiscard]] bool init_ssl_impl() {
    try {
      ssl_init_ret_ = false;
      ELOGV(INFO, "init ssl: %s", config_.ssl_domain.data());
      auto &cert_file = config_.ssl_cert_path;
      ELOGV(INFO, "current path %s",
            std::filesystem::current_path().string().data());
      if (file_exists(cert_file)) {
        ELOGV(INFO, "load %s", cert_file.string().data());
        ssl_ctx_.load_verify_file(cert_file);
      }
      else {
        ELOGV(INFO, "no certificate file %s", cert_file.string().data());
        return ssl_init_ret_;
      }
      ssl_ctx_.set_verify_mode(asio::ssl::verify_peer);
      ssl_ctx_.set_verify_callback(
          asio::ssl::host_name_verification(config_.ssl_domain));
      ssl_stream_ =
          std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
              *socket_, ssl_ctx_);
      ssl_init_ret_ = true;
    } catch (std::exception &e) {
      ELOGV(ERROR, "init ssl failed: %s", e.what());
    }
    return ssl_init_ret_;
  }
#endif
  async_simple::coro::Lazy<bool> timeout(auto &timer, auto duration,
                                         auto &promise, std::string err_msg) {
    timer.expires_after(duration);
    bool is_timeout = co_await timer.async_await();
#ifdef UNIT_TEST_INJECT
    ELOGV(INFO, "client_id %d %s, is_timeout_ %d, %d , duration %d ms",
          config_.client_id, err_msg.data(), is_timeout_, is_timeout,
          std::chrono::duration_cast<std::chrono::milliseconds>(duration)
              .count());
#endif
    if (!is_timeout) {
      promise.setValue(async_simple::Unit());
      co_return false;
    }

    is_timeout_ = is_timeout;
    close_socket(socket_);
    promise.setValue(async_simple::Unit());
    co_return true;
  }

  template <auto func, typename... Args>
  void static_check() {
    using Function = decltype(func);
    using param_type = util::function_parameters_t<Function>;
    if constexpr (!std::is_void_v<param_type>) {
      using First = std::tuple_element_t<0, param_type>;
      constexpr bool is_conn = requires { typename First::return_type; };

      if constexpr (std::is_member_function_pointer_v<Function>) {
        using Self = util::class_type_t<Function>;
        if constexpr (is_conn) {
          static_assert(
              util::is_invocable<Function, Self, First, Args...>::value,
              "called rpc function and arguments are not match");
        }
        else {
          static_assert(util::is_invocable<Function, Self, Args...>::value,
                        "called rpc function and arguments are not match");
        }
      }
      else {
        if constexpr (is_conn) {
          static_assert(util::is_invocable<Function, First, Args...>::value,
                        "called rpc function and arguments are not match");
        }
        else {
          static_assert(util::is_invocable<Function, Args...>::value,
                        "called rpc function and arguments are not match");
        }
      }
    }
    else {
      if constexpr (std::is_member_function_pointer_v<Function>) {
        using Self = util::class_type_t<Function>;
        static_assert(util::is_invocable<Function, Self, Args...>::value,
                      "called rpc function and arguments are not match");
      }
      else {
        static_assert(util::is_invocable<Function, Args...>::value,
                      "called rpc function and arguments are not match");
      }
    }
  }

  template <auto func, typename Socket, typename... Args>
  async_simple::coro::Lazy<
      rpc_result<decltype(get_return_type<func>()), coro_rpc_protocol>>
  call_impl(Socket &socket, Args... args) {
    using R = decltype(get_return_type<func>());

    auto buffer = prepare_buffer<func>(std::move(args)...);
    rpc_result<R, coro_rpc_protocol> r{};
#ifdef GENERATE_BENCHMARK_DATA
    std::ofstream file(
        benchmark_file_path + std::string{get_func_name<func>()} + ".in",
        std::ofstream::binary | std::ofstream::out);
    file << std::string_view{(char *)buffer.data(), buffer.size()};
    file.close();
#endif
    std::pair<std::error_code, size_t> ret;
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::client_send_bad_header) {
      buffer[0] = (std::byte)(uint8_t(buffer[0]) + 1);
    }
    if (g_action == inject_action::client_close_socket_after_send_header) {
      ret = co_await coro_io::async_write(
          socket, asio::buffer(buffer.data(), coro_rpc_protocol::REQ_HEAD_LEN));
      ELOGV(INFO, "client_id %d close socket", config_.client_id);
      close();
      r = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::io_error,
                                                     ret.first.message()}};
      co_return r;
    }
    else if (g_action ==
             inject_action::client_close_socket_after_send_partial_header) {
      ret = co_await coro_io::async_write(
          socket,
          asio::buffer(buffer.data(), coro_rpc_protocol::REQ_HEAD_LEN - 1));
      ELOGV(INFO, "client_id %d close socket", config_.client_id);
      close();
      r = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::io_error,
                                                     ret.first.message()}};
      co_return r;
    }
    else if (g_action ==
             inject_action::client_shutdown_socket_after_send_header) {
      ret = co_await coro_io::async_write(
          socket, asio::buffer(buffer.data(), coro_rpc_protocol::REQ_HEAD_LEN));
      ELOGV(INFO, "client_id %d shutdown", config_.client_id);
      socket_->shutdown(asio::ip::tcp::socket::shutdown_send);
      r = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::io_error,
                                                     ret.first.message()}};
      co_return r;
    }
    else {
      ret = co_await coro_io::async_write(
          socket, asio::buffer(buffer.data(), buffer.size()));
    }
#else
    ret = co_await coro_io::async_write(
        socket, asio::buffer(buffer.data(), buffer.size()));
#endif
    if (!ret.first) {
#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::client_close_socket_after_send_payload) {
        ELOGV(INFO, "client_id %d client_close_socket_after_send_payload",
              config_.client_id);
        r = rpc_result<R, coro_rpc_protocol>{
            unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::io_error,
                                                       ret.first.message()}};
        close();
        co_return r;
      }
#endif
      coro_rpc_protocol::resp_header header;
      ret = co_await coro_io::async_read(
          socket,
          asio::buffer((char *)&header, coro_rpc_protocol::RESP_HEAD_LEN));
      if (!ret.first) {
        uint32_t body_len = header.length;
        if (body_len > read_buf_.size()) {
          read_buf_.resize(body_len);
        }
        ret = co_await coro_io::async_read(
            socket, asio::buffer(read_buf_.data(), body_len));
        if (!ret.first) {
#ifdef GENERATE_BENCHMARK_DATA
          std::ofstream file(
              benchmark_file_path + std::string{get_func_name<func>()} + ".out",
              std::ofstream::binary | std::ofstream::out);
          file << std::string_view{(char *)&header,
                                   coro_rpc_protocol::RESP_HEAD_LEN};
          file << std::string_view{(char *)read_buf_.data(), body_len};
          file.close();
#endif
          r = handle_response_buffer<R>(read_buf_.data(), ret.second,
                                        std::errc{header.err_code});
          if (!r) {
            close();
          }
          co_return r;
        }
      }
    }
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::force_inject_client_write_data_timeout) {
      is_timeout_ = true;
    }
#endif
    if (is_timeout_) {
      r = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{
                            .code = std::errc::timed_out, .msg = {}}};
    }
    else {
      r = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{},
          coro_rpc_protocol::rpc_error{.code = std::errc::io_error,
                                       .msg = ret.first.message()}};
    }
    close();
    co_return r;
  }
  /*
   * buffer layout
   * ┌────────────────┬────────────────┐
   * │req_header      │args            │
   * ├────────────────┼────────────────┤
   * │REQ_HEADER_LEN  │variable length │
   * └────────────────┴────────────────┘
   */
  template <auto func, typename... Args>
  std::vector<std::byte> prepare_buffer(Args &&...args) {
    std::vector<std::byte> buffer;
    std::size_t offset = coro_rpc_protocol::REQ_HEAD_LEN;
    if constexpr (sizeof...(Args) > 0) {
      using arg_types = util::function_parameters_t<decltype(func)>;
      pack_to<arg_types>(buffer, offset, std::forward<Args>(args)...);
    }
    else {
      buffer.resize(offset);
    }

    auto &header = *(coro_rpc_protocol::req_header *)buffer.data();
    header = {};
    header.magic = coro_rpc_protocol::magic_number;
    header.function_id = func_id<func>();
#ifdef UNIT_TEST_INJECT
    header.seq_num = config_.client_id;
    if (g_action == inject_action::client_send_bad_magic_num) {
      header.magic = coro_rpc_protocol::magic_number + 1;
    }
    if (g_action == inject_action::client_send_header_length_0) {
      header.length = 0;
    }
    else {
#endif
      header.length = buffer.size() - coro_rpc_protocol::REQ_HEAD_LEN;
#ifdef UNIT_TEST_INJECT
    }
#endif
    return buffer;
  }

  template <typename T>
  rpc_result<T, coro_rpc_protocol> handle_response_buffer(
      const std::byte *buffer, std::size_t len, std::errc rpc_errc) {
    rpc_return_type_t<T> ret;
    struct_pack::errc ec;
    coro_rpc_protocol::rpc_error err;
    if (rpc_errc == std::errc{}) {
      ec = struct_pack::deserialize_to(ret, (const char *)buffer, len);
      if (ec == struct_pack::errc::ok) {
        if constexpr (std::is_same_v<T, void>) {
          return {};
        }
        else {
          return std::move(ret);
        }
      }
    }
    else {
      err.code = rpc_errc;
      ec = struct_pack::deserialize_to(err.msg, (const char *)buffer, len);
      if (ec == struct_pack::errc::ok) {
        return rpc_result<T, coro_rpc_protocol>{unexpect_t{}, std::move(err)};
      }
    }
    // deserialize failed.
    err = {std::errc::invalid_argument,
           "failed to deserialize rpc return value"};
    return rpc_result<T, coro_rpc_protocol>{unexpect_t{}, std::move(err)};
  }

  template <typename FuncArgs>
  auto get_func_args() {
    using First = std::tuple_element_t<0, FuncArgs>;
    constexpr bool has_conn_v = requires { typename First::return_type; };
    return util::get_args<has_conn_v, FuncArgs>();
  }

  template <typename... FuncArgs, typename Buffer, typename... Args>
  void pack_to_impl(Buffer &buffer, std::size_t offset, Args &&...args) {
    struct_pack::serialize_to_with_offset(
        buffer, offset, std::forward<FuncArgs>(std::forward<Args>(args))...);
  }

  template <typename Tuple, size_t... Is, typename Buffer, typename... Args>
  void pack_to_helper(std::index_sequence<Is...>, Buffer &buffer,
                      std::size_t offset, Args &&...args) {
    pack_to_impl<std::tuple_element_t<Is, Tuple>...>(
        buffer, offset, std::forward<Args>(args)...);
  }

  template <typename FuncArgs, typename Buffer, typename... Args>
  void pack_to(Buffer &buffer, std::size_t offset, Args &&...args) {
    using tuple_pack = decltype(get_func_args<FuncArgs>());
    pack_to_helper<tuple_pack>(
        std::make_index_sequence<std::tuple_size_v<tuple_pack>>{}, buffer,
        offset, std::forward<Args>(args)...);
  }

  void close_socket(std::shared_ptr<asio::ip::tcp::socket> socket) {
    asio::dispatch(
        executor.get_asio_executor(), [socket = std::move(socket)]() {
          asio::error_code ignored_ec;
          socket->shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
          socket->close(ignored_ec);
        });
  }

#ifdef UNIT_TEST_INJECT
 public:
  std::errc sync_connect(const std::string &host, const std::string &port) {
    return async_simple::coro::syncAwait(connect(host, port));
  }

  template <auto func, typename... Args>
  rpc_result<decltype(get_return_type<func>()), coro_rpc_protocol> sync_call(
      Args &&...args) {
    return async_simple::coro::syncAwait(
        call<func>(std::forward<Args>(args)...));
  }
#endif
 private:
  coro_io::ExecutorWrapper<> executor;
  std::shared_ptr<asio::ip::tcp::socket> socket_;
  std::vector<std::byte> read_buf_;
  config config_;
  constexpr static std::size_t default_read_buf_size_ = 256;
#ifdef YLT_ENABLE_SSL
  asio::ssl::context ssl_ctx_{asio::ssl::context::sslv23};
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_;
  bool ssl_init_ret_ = true;
#endif
  bool is_timeout_ = false;
  std::atomic<bool> has_closed_ = false;
};
}  // namespace coro_rpc
