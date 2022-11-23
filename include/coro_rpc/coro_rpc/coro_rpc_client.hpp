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
#include <memory>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "asio_util/asio_coro_util.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "common_service.hpp"
#include "coro_rpc/coro_rpc/connection.hpp"
#include "coro_rpc/coro_rpc/rpc_protocol.h"
#include "logging/easylog.hpp"
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
class async_connection;

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
 *   assert(ec == err_ok);
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
 public:
  /*!
   * Create client with io_context
   * @param io_context asio io_context, async event handler
   */
  coro_rpc_client(asio::io_context &io_context)
      : io_context_ptr_(&io_context),
        executor_(io_context),
        socket_(io_context) {
    read_buf_.resize(default_read_buf_size_);
  }

  /*!
   * Create client
   */
  coro_rpc_client()
      : io_context_ptr_(inner_io_context_.get()),
        executor_(*inner_io_context_),
        socket_(*inner_io_context_) {
    std::promise<void> promise;
    thd_ = std::thread([this, &promise] {
      asio::io_context::work work(*inner_io_context_);
      promise.set_value();
      inner_io_context_->run();
    });
    promise.get_future().wait();
  }

  /*!
   * Check the client closed or not
   *
   * @return true if client closed, otherwise false.
   */
  [[nodiscard]] bool has_closed() { return !socket_.is_open(); }

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

    async_simple::Promise<async_simple::Unit> promise;
    asio_util::period_timer timer(get_io_context());
    timeout(timer, timeout_duration, promise, "connect timer canceled")
        .via(&executor_)
        .detach();

    std::error_code ec = co_await asio_util::async_connect(get_io_context(),
                                                           socket_, host, port);
    if (!is_timeout_) {
      std::error_code err_code;
      timer.cancel(err_code);
    }

    co_await promise.getFuture();
    if (ec) {
      if (is_timeout_) {
        co_return std::errc::timed_out;
      }
      co_return std::errc::not_connected;
    }

#ifdef ENABLE_SSL
    if (use_ssl_) {
      assert(ssl_stream_);
      auto shake_ec = co_await asio_util::async_handshake(
          ssl_stream_, asio::ssl::stream_base::client);
      if (shake_ec) {
        easylog::warn("handshake failed:{}", shake_ec.message());
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
      easylog::info("init ssl: {}", domain);
      auto full_cert_file = std::filesystem::path(base_path).append(cert_file);
      easylog::info("current path {}",
                    std::filesystem::current_path().string());
      if (file_exists(full_cert_file)) {
        easylog::info("load {}", full_cert_file.string());
        ssl_ctx_.load_verify_file(full_cert_file);
      }
      else {
        easylog::info("no certificate file {}", full_cert_file.string());
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
      easylog::error("init ssl failed: {}", e.what());
    }
    return ssl_init_ret_;
  }
#endif

  ~coro_rpc_client() { sync_close(); }

  /*!
   * Call RPC function with default timeout (5 second)
   *
   * @tparam func the address of RPC function
   * @tparam Args the type of arguments
   * @param args RPC function arguments
   * @return RPC call result
   */
  template <auto func, typename... Args>
  async_simple::coro::Lazy<rpc_result<decltype(get_return_type<func>())>> call(
      Args &&...args) {
    return call_for<func>(std::chrono::seconds(5), std::forward<Args>(args)...);
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
  async_simple::coro::Lazy<rpc_result<decltype(get_return_type<func>())>>
  call_for(auto duration, Args &&...args) {
    using R = decltype(get_return_type<func>());
    rpc_result<R> ret;
#ifdef ENABLE_SSL
    if (!ssl_init_ret_) {
      ret = rpc_result<R>{unexpect_t{},
                          rpc_error{std::errc::not_connected, "not connected"}};
      co_return ret;
    }
#endif

    static_check<func, Args...>();

    async_simple::Promise<async_simple::Unit> promise;
    asio_util::period_timer timer(get_io_context());
    timeout(timer, duration, promise, "rpc call timer canceled")
        .via(&executor_)
        .detach();

#ifdef ENABLE_SSL
    if (use_ssl_) {
      assert(ssl_stream_);
      ret = co_await call_impl<func>(*ssl_stream_, std::forward<Args>(args)...);
    }
    else {
#endif
      ret = co_await call_impl<func>(socket_, std::forward<Args>(args)...);
#ifdef ENABLE_SSL
    }
#endif
    if (!is_timeout_) {
      std::error_code err_code;
      timer.cancel(err_code);
    }
    else {
      ret = rpc_result<R>{
          unexpect_t{}, rpc_error{std::errc::timed_out, "rpc call timed out"}};
    }

    co_await promise.getFuture();
    co_return ret;
  }

  /*!
   * Get inner executor
   */
  auto &get_executor() { return executor_; }

 private:
  async_simple::coro::Lazy<bool> timeout(auto &timer, auto duration,
                                         auto &promise, std::string err_msg) {
    timer.expires_after(duration);
    bool is_timeout = co_await timer.async_await();

    if (!is_timeout) {
      easylog::debug("{}", err_msg);
      promise.setValue(async_simple::Unit());
      co_return false;
    }

    is_timeout_ = is_timeout;
    sync_close(false);
    promise.setValue(async_simple::Unit());
    co_return true;
  }

  template <auto func, typename... Args>
  void static_check() {
    using Function = decltype(func);
    using param_type = function_parameters_t<Function>;
    if constexpr (!std::is_void_v<param_type>) {
      using First = std::tuple_element_t<0, param_type>;
      constexpr bool is_conn = is_specialization<First, connection>::value;

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
  async_simple::coro::Lazy<rpc_result<decltype(get_return_type<func>())>>
  call_impl(Socket &socket, Args &&...args) {
    using R = decltype(get_return_type<func>());

    auto buffer = prepare_buffer<func>(std::forward<Args>(args)...);
    rpc_result<R> r{};
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
          socket, asio::buffer(buffer.data(), RPC_HEAD_LEN));
      easylog::info("close socket");
      co_await close();
      r = rpc_result<R>{unexpect_t{},
                        rpc_error{std::errc::io_error, ret.first.message()}};
      co_return r;
    }
    else if (g_action ==
             inject_action::client_close_socket_after_send_partial_header) {
      ret = co_await asio_util::async_write(
          socket, asio::buffer(buffer.data(), RPC_HEAD_LEN - 1));
      easylog::info("close socket");
      co_await close();
      r = rpc_result<R>{unexpect_t{},
                        rpc_error{std::errc::io_error, ret.first.message()}};
      co_return r;
    }
    else if (g_action ==
             inject_action::client_shutdown_socket_after_send_header) {
      ret = co_await asio_util::async_write(
          socket, asio::buffer(buffer.data(), RPC_HEAD_LEN));
      easylog::info("shutdown");
      socket_.shutdown(asio::ip::tcp::socket::shutdown_send);
      r = rpc_result<R>{unexpect_t{},
                        rpc_error{std::errc::io_error, ret.first.message()}};
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
        easylog::info("client_close_socket_after_send_payload");
        r = rpc_result<R>{unexpect_t{},
                          rpc_error{std::errc::io_error, ret.first.message()}};
        co_await close();
        co_return r;
      }
#endif
      char head[RESPONSE_HEADER_LEN];
      ret = co_await asio_util::async_read(
          socket, asio::buffer(head, RESPONSE_HEADER_LEN));
      if (!ret.first) {
        uint32_t body_len = *(uint32_t *)head;
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
          file << std::string_view{std::begin(head), RESPONSE_HEADER_LEN};
          file << std::string_view{(char *)read_buf_.data(), body_len};
          file.close();
#endif
          r = handle_response_buffer<R>(read_buf_.data(), ret.second);
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
      r = rpc_result<R>{unexpect_t{},
                        rpc_error{.code = std::errc::timed_out, .msg = {}}};
    }
    else {
      r = rpc_result<R>{unexpect_t{}, rpc_error{.code = std::errc::io_error,
                                                .msg = ret.first.message()}};
    }
    co_await close();
    co_return r;
  }
  /*
   * buffer layout
   * ┌────────────────┬────────────────┬────────────────┐
   * │rpc_header      │func_id         │args            │
   * ├────────────────┼────────────────┼────────────────┤
   * │RPC_HEADER_LEN  │FUNCTION_ID_LEN │variable length │
   * └────────────────┴────────────────┴────────────────┘
   */
  template <auto func, typename... Args>
  std::vector<std::byte> prepare_buffer(Args &&...args) {
    std::vector<std::byte> buffer;
    constexpr auto id = func_id<func>();
    std::size_t offset = RPC_HEAD_LEN + FUNCTION_ID_LEN;
    if constexpr (sizeof...(Args) > 0) {
      using arg_types = function_parameters_t<decltype(func)>;
      pack_to<arg_types>(buffer, offset, std::forward<Args>(args)...);
    }
    else {
      buffer.resize(offset);
    }
    std::memcpy(buffer.data() + RPC_HEAD_LEN, &id, FUNCTION_ID_LEN);

    rpc_header header{magic_number};
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::client_send_bad_magic_num) {
      header.magic = magic_number + 1;
    }
    if (g_action == inject_action::client_send_header_length_0) {
      header.length = 0;
    }
    else {
#endif
      header.length = buffer.size() - RPC_HEAD_LEN;
#ifdef UNIT_TEST_INJECT
    }
#endif
    [[maybe_unused]] auto sz =
        struct_pack::serialize_to(buffer.data(), RPC_HEAD_LEN, header);
    assert(sz == RPC_HEAD_LEN);
    return buffer;
  }

  template <typename T>
  rpc_result<T> handle_response_buffer(const std::byte *buffer,
                                       std::size_t len) {
    rpc_return_type_t<T> ret;
    auto ec = struct_pack::deserialize_to(ret, buffer, len);
    if (ec == struct_pack::errc::ok) {
      if constexpr (std::is_same_v<T, void>) {
        return {};
      }
      else {
        return ret;
      }
    }
    else {
      rpc_error err;
      auto ec = struct_pack::deserialize_to(err, buffer, len);
      if (ec != struct_pack::errc::ok) {
        // if deserialize failed again,
        // we can do nothing but give an error code.
        err.code = std::errc::invalid_argument;
      }
      return rpc_result<T>{unexpect_t{}, std::move(err)};
    }
  }

  template <typename FuncArgs>
  auto get_func_args() {
    using First = std::tuple_element_t<0, FuncArgs>;
    constexpr bool has_conn_v = is_specialization<First, connection>::value;
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

  async_simple::coro::Lazy<void> close(bool close_ssl = true) {
#ifdef ENABLE_SSL
    if (close_ssl) {
      close_ssl_stream();
    }
#endif
    if (!socket_.is_open()) {
      co_return;
    }

    co_await asio_util::async_close(socket_);
  }

#ifdef ENABLE_SSL
  void close_ssl_stream() {
    if (ssl_stream_) {
      asio::error_code ec;
      ssl_stream_->shutdown(ec);
      ssl_stream_ = nullptr;
    }
  }
#endif

  void sync_close(bool close_ssl = true) {
#ifdef ENABLE_SSL
    if (close_ssl) {
      close_ssl_stream();
      stop_inner_io_context();
    }
#endif
    if (close_ssl && !socket_.is_open()) {
      stop_inner_io_context();
      return;
    }

    asio::error_code ignored_ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_.close(ignored_ec);

    if (close_ssl) {
      stop_inner_io_context();
    }
  }

  void stop_inner_io_context() {
    if (thd_.joinable()) {
      if (thd_.get_id() == std::this_thread::get_id()) {
        easylog::warn("avoid join self");
        std::thread([thd = std::move(thd_),
                     io_ctx = std::move(inner_io_context_)]() mutable {
          io_ctx->stop();
          if (thd.joinable())
            thd.join();
        }).detach();
      }
      else {
        inner_io_context_->stop();
        thd_.join();
      }
    }

    return;
  }

  asio::io_context &get_io_context() {
    assert(io_context_ptr_);
    return *io_context_ptr_;
  }

#ifdef UNIT_TEST_INJECT
 public:
  std::errc sync_connect(const std::string &host, const std::string &port) {
    return async_simple::coro::syncAwait(connect(host, port));
  }

  template <auto func, typename... Args>
  rpc_result<decltype(get_return_type<func>())> sync_call(Args &&...args) {
    return async_simple::coro::syncAwait(
        call<func>(std::forward<Args>(args)...));
  }
#endif
 private:
  std::shared_ptr<asio::io_context> inner_io_context_ =
      std::make_shared<asio::io_context>();
  asio::io_context *io_context_ptr_ = nullptr;
  std::thread thd_;
  asio_util::AsioExecutor executor_;
  asio::ip::tcp::socket socket_;
  std::vector<std::byte> read_buf_;
  std::size_t default_read_buf_size_ = 256;

#ifdef ENABLE_SSL
  asio::ssl::context ssl_ctx_{asio::ssl::context::sslv23};
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_;
  bool ssl_init_ret_ = true;
  bool use_ssl_ = false;
#endif
  bool is_timeout_ = false;
};
}  // namespace coro_rpc
