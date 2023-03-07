/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "asio/io_context.hpp"
#include "asio_util/asio_coro_util.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "common_service.hpp"
#include "coro_rpc/coro_rpc/context.hpp"
#include "coro_rpc/coro_rpc/protocol/coro_rpc_protocol.hpp"
#include "easylog/easylog.h"
#include "expected.hpp"
#include "struct_pack/struct_pack.hpp"
#include "util/function_name.h"
#include "util/type_traits.h"
#include "util/utils.hpp"
#ifdef UNIT_TEST_INJECT
#include "asio_util/asio_util.hpp"
#include "inject_action.hpp"
#endif

#ifdef GENERATE_BENCHMARK_DATA
#include <fstream>
#endif

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
 * #include <coro_rpc/coro_rpc_client.hpp>
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
  /*!
   * Create client with io_context
   * @param io_context asio io_context, async event handler
   */
  coro_rpc_client(asio::io_context::executor_type executor,
                  uint32_t client_id = 0)
      : executor_wrapper_(executor), socket_(executor), client_id_(client_id) {
    read_buf_.resize(default_read_buf_size_);
  }

  /*!
   * Create client
   */
  coro_rpc_client(uint32_t client_id = 0)
      : inner_io_context_(std::make_unique<asio::io_context>()),
        executor_wrapper_(inner_io_context_->get_executor()),
        socket_(inner_io_context_->get_executor()),
        client_id_(client_id) {
    std::promise<void> promise;
    thd_ = std::thread([this, &promise] {
      work_ = std::make_unique<asio::io_context::work>(*inner_io_context_);
      asio::post(executor_wrapper_.get_executor(), [&] {
        promise.set_value();
      });
      inner_io_context_->run();
    });
    promise.get_future().wait();
  }

  /*!
   * Check the client closed or not
   *
   * @return true if client closed, otherwise false.
   */
  [[nodiscard]] bool has_closed() { return has_closed_; }

  /*!
   * Connect server
   *
   * If connect fail, an error code returned.
   * Either timeout or connection error can result in a connection failure.
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
#ifdef ENABLE_SSL
    if (!ssl_init_ret_) {
      co_return std::errc::not_connected;
    }
#endif
    if (has_closed_)
      AS_UNLIKELY {
        ELOGV(
            ERROR,
            "a closed client is not allowed connect again, please create a new "
            "client");
        co_return std::errc::io_error;
      }

    ELOGV(INFO, "client_id %d begin to connect %s", client_id_, port.data());
    async_simple::Promise<async_simple::Unit> promise;
    asio_util::period_timer timer(executor_wrapper_.get_executor());
    timeout(timer, timeout_duration, promise, "connect timer canceled")
        .via(&executor_wrapper_)
        .detach();

    std::error_code ec = co_await asio_util::async_connect(
        executor_wrapper_.get_executor(), socket_, host, port);
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
      ELOGV(WARN, "client_id %d connect timeout", client_id_);
      co_return std::errc::timed_out;
    }

#ifdef ENABLE_SSL
    if (use_ssl_) {
      assert(ssl_stream_);
      auto shake_ec = co_await asio_util::async_handshake(
          ssl_stream_, asio::ssl::stream_base::client);
      if (shake_ec) {
        ELOGV(WARN, "client_id %d handshake failed: %s", client_id_,
              shake_ec.message().data());
        co_return std::errc::not_connected;
      }
    }
#endif

    co_return std::errc{};
  }
#ifdef ENABLE_SSL
  [[nodiscard]] bool init_ssl(const std::string &base_path,
                              const std::string &cert_file,
                              const std::string &domain = "localhost") {
    try {
      ssl_init_ret_ = false;
      ELOGV(INFO, "init ssl: %s", domain.data());
      auto full_cert_file = std::filesystem::path(base_path).append(cert_file);
      ELOGV(INFO, "current path %s",
            std::filesystem::current_path().string().data());
      if (file_exists(full_cert_file)) {
        ELOGV(INFO, "load %s", full_cert_file.string().data());
        ssl_ctx_.load_verify_file(full_cert_file);
      }
      else {
        ELOGV(INFO, "no certificate file %s", full_cert_file.string().data());
        return ssl_init_ret_;
      }

      ssl_ctx_.set_verify_mode(asio::ssl::verify_peer);

      // ssl_ctx_.add_certificate_authority(asio::buffer(CA_PEM));

      ssl_ctx_.set_verify_callback(asio::ssl::host_name_verification(domain));
      ssl_stream_ =
          std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
              socket_, ssl_ctx_);
      use_ssl_ = true;
      ssl_init_ret_ = true;
    } catch (std::exception &e) {
      ELOGV(ERROR, "init ssl failed: %s", e.what());
    }
    return ssl_init_ret_;
  }
#endif

  ~coro_rpc_client() {
    close();
    stop_inner_io_context();
  }

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
        ELOGV(ERROR,
              "a closed client is not allowed call again, please create a new "
              "client");
        auto ret = rpc_result<R, coro_rpc_protocol>{
            unexpect_t{},
            coro_rpc_protocol::rpc_error{
                std::errc::io_error,
                "client has been closed, please create a new client"}};
        co_return ret;
      }

    rpc_result<R, coro_rpc_protocol> ret;
#ifdef ENABLE_SSL
    if (!ssl_init_ret_) {
      ret = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::not_connected,
                                                     "not connected"}};
      co_return ret;
    }
#endif

    static_check<func, Args...>();

    async_simple::Promise<async_simple::Unit> promise;
    asio_util::period_timer timer(executor_wrapper_.get_executor());
    timeout(timer, duration, promise, "rpc call timer canceled")
        .via(&executor_wrapper_)
        .detach();

#ifdef ENABLE_SSL
    if (use_ssl_) {
      assert(ssl_stream_);
      ret = co_await call_impl<func>(*ssl_stream_, std::move(args)...);
    }
    else {
#endif
      ret = co_await call_impl<func>(socket_, std::move(args)...);
#ifdef ENABLE_SSL
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
    ELOGV(INFO, "client_id %d call %s %s", client_id_,
          get_func_name<func>().data(), ret ? "ok" : "failed");
#endif
    co_return ret;
  }

  /*!
   * Get inner executor
   */
  auto &get_executor() { return executor_wrapper_; }

  uint32_t get_client_id() const { return client_id_; }

 private:
  async_simple::coro::Lazy<bool> timeout(auto &timer, auto duration,
                                         auto &promise, std::string err_msg) {
    timer.expires_after(duration);
    bool is_timeout = co_await timer.async_await();
#ifdef UNIT_TEST_INJECT
    ELOGV(INFO, "client_id %d %s, is_timeout_ %d, %d , duration %d ms",
          client_id_, err_msg.data(), is_timeout_, is_timeout,
          std::chrono::duration_cast<std::chrono::milliseconds>(duration)
              .count());
#endif
    if (!is_timeout) {
      promise.setValue(async_simple::Unit());
      co_return false;
    }

    is_timeout_ = is_timeout;
    close_socket();
    promise.setValue(async_simple::Unit());
    co_return true;
  }

  template <auto func, typename... Args>
  void static_check() {
    using Function = decltype(func);
    using param_type = function_parameters_t<Function>;
    if constexpr (!std::is_void_v<param_type>) {
      using First = std::tuple_element_t<0, param_type>;
      constexpr bool is_conn = requires { typename First::return_type; };

      if constexpr (std::is_member_function_pointer_v<Function>) {
        using Self = class_type_t<Function>;
        if constexpr (is_conn) {
          static_assert(is_invocable<Function, Self, First, Args...>::value,
                        "called rpc function and arguments are not match");
        }
        else {
          static_assert(is_invocable<Function, Self, Args...>::value,
                        "called rpc function and arguments are not match");
        }
      }
      else {
        if constexpr (is_conn) {
          static_assert(is_invocable<Function, First, Args...>::value,
                        "called rpc function and arguments are not match");
        }
        else {
          static_assert(is_invocable<Function, Args...>::value,
                        "called rpc function and arguments are not match");
        }
      }
    }
    else {
      if constexpr (std::is_member_function_pointer_v<Function>) {
        using Self = class_type_t<Function>;
        static_assert(is_invocable<Function, Self, Args...>::value,
                      "called rpc function and arguments are not match");
      }
      else {
        static_assert(is_invocable<Function, Args...>::value,
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
      ret = co_await asio_util::async_write(
          socket, asio::buffer(buffer.data(), coro_rpc_protocol::REQ_HEAD_LEN));
      ELOGV(INFO, "client_id %d close socket", client_id_);
      close();
      r = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::io_error,
                                                     ret.first.message()}};
      co_return r;
    }
    else if (g_action ==
             inject_action::client_close_socket_after_send_partial_header) {
      ret = co_await asio_util::async_write(
          socket,
          asio::buffer(buffer.data(), coro_rpc_protocol::REQ_HEAD_LEN - 1));
      ELOGV(INFO, "client_id %d close socket", client_id_);
      close();
      r = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::io_error,
                                                     ret.first.message()}};
      co_return r;
    }
    else if (g_action ==
             inject_action::client_shutdown_socket_after_send_header) {
      ret = co_await asio_util::async_write(
          socket, asio::buffer(buffer.data(), coro_rpc_protocol::REQ_HEAD_LEN));
      ELOGV(INFO, "client_id %d shutdown", client_id_);
      socket_.shutdown(asio::ip::tcp::socket::shutdown_send);
      r = rpc_result<R, coro_rpc_protocol>{
          unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::io_error,
                                                     ret.first.message()}};
      co_return r;
    }
    else {
      ret = co_await asio_util::async_write(
          socket, asio::buffer(buffer.data(), buffer.size()));
    }
#else
    ret = co_await asio_util::async_write(
        socket, asio::buffer(buffer.data(), buffer.size()));
#endif
    if (!ret.first) {
#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::client_close_socket_after_send_payload) {
        ELOGV(INFO, "client_id %d client_close_socket_after_send_payload",
              client_id_);
        r = rpc_result<R, coro_rpc_protocol>{
            unexpect_t{}, coro_rpc_protocol::rpc_error{std::errc::io_error,
                                                       ret.first.message()}};
        close();
        co_return r;
      }
#endif
      coro_rpc_protocol::resp_header header;
      ret = co_await asio_util::async_read(
          socket,
          asio::buffer((char *)&header, coro_rpc_protocol::RESP_HEAD_LEN));
      if (!ret.first) {
        uint32_t body_len = header.length;
        if (body_len > read_buf_.size()) {
          read_buf_.resize(body_len);
        }
        ret = co_await asio_util::async_read(
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
      using arg_types = function_parameters_t<decltype(func)>;
      pack_to<arg_types>(buffer, offset, std::forward<Args>(args)...);
    }
    else {
      buffer.resize(offset);
    }

    auto &header = *(coro_rpc_protocol::req_header *)buffer.data();
    header.magic = coro_rpc_protocol::magic_number;
    header.function_id = func_id<func>();
#ifdef UNIT_TEST_INJECT
    header.seq_num = client_id_;
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
    return get_args<has_conn_v, FuncArgs>();
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

  void close_socket() {
    asio::error_code ignored_ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_.close(ignored_ec);
  }

  void close() {
    if (has_closed_) {
      return;
    }

    ELOGV(INFO, "client_id %d close", client_id_);
    close_socket();

    has_closed_ = true;
  }

  void stop_inner_io_context() {
    if (thd_.joinable()) {
      work_ = nullptr;
      if (thd_.get_id() == std::this_thread::get_id()) {
        // we are now running in inner_io_context_, so destruction it in another
        // thread
        std::thread thrd{[ioc = std::move(inner_io_context_),
                          thd = std::move(thd_)]() mutable {
          thd.join();
        }};
        thrd.detach();
      }
      else {
        thd_.join();
      }
    }

    return;
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
  std::unique_ptr<asio::io_context> inner_io_context_;
  std::unique_ptr<asio::io_context::work> work_;
  std::thread thd_;
  asio_util::ExecutorWrapper<asio::io_context::executor_type> executor_wrapper_;
  asio::ip::tcp::socket socket_;
  std::vector<std::byte> read_buf_;
  constexpr static std::size_t default_read_buf_size_ = 256;

#ifdef ENABLE_SSL
  asio::ssl::context ssl_ctx_{asio::ssl::context::sslv23};
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_;
  bool ssl_init_ret_ = true;
  bool use_ssl_ = false;
#endif
  bool is_timeout_ = false;

  uint32_t client_id_ = 0;
  std::atomic<bool> has_closed_ = false;
};
}  // namespace coro_rpc
