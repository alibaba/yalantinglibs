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

#include <array>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <ylt/easylog.hpp>

#include "asio/buffer.hpp"
#include "asio/dispatch.hpp"
#include "asio/registered_buffer.hpp"
#include "async_simple/Executor.h"
#include "async_simple/Promise.h"
#include "async_simple/coro/Mutex.h"
#include "async_simple/coro/SpinLock.h"
#include "common_service.hpp"
#include "context.hpp"
#include "expected.hpp"
#include "protocol/coro_rpc_protocol.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/coro_rpc/impl/errno.h"
#include "ylt/struct_pack.hpp"
#include "ylt/struct_pack/util.h"
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

inline uint64_t get_global_client_id() {
  static std::atomic<uint64_t> cid = 0;
  return cid.fetch_add(1, std::memory_order::relaxed);
}

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

struct resp_body {
  std::string read_buf_;
  std::string resp_attachment_buf_;
};

template <typename T>
struct async_rpc_result_value_t {
 private:
  T result_;
  resp_body buffer_;

 public:
  async_rpc_result_value_t(T &&result, resp_body &&buffer)
      : result_(std::move(result)), buffer_(std::move(buffer)) {}
  async_rpc_result_value_t(T &&result) : result_(std::move(result)) {}
  T &result() noexcept { return result_; }
  const T &result() const noexcept { return result_; }
  std::string_view get_attachment() const noexcept {
    return buffer_.resp_attachment_buf_;
  }
  resp_body release_buffer() { return std::move(buffer_); }
};

template <>
struct async_rpc_result_value_t<void> {
  resp_body buffer_;
  std::string_view attachment() const noexcept {
    return buffer_.resp_attachment_buf_;
  }
  resp_body release_buffer() { return std::move(buffer_); }
};

template <typename T>
using async_rpc_result = expected<async_rpc_result_value_t<T>, rpc_error>;

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
 *   assert(!ec);
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
  const inline static rpc_error connect_error = {errc::io_error,
                                                 "client has been closed"};
  struct config {
    uint64_t client_id = get_global_client_id();
    std::chrono::milliseconds timeout_duration =
        std::chrono::milliseconds{30000};
    std::string host;
    std::string port;
    bool enable_tcp_no_delay = true;
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
                  uint64_t client_id = get_global_client_id())
      : control_(std::make_shared<control_t>(executor, false)),
        timer_(std::make_unique<coro_io::period_timer>(executor)) {
    config_.client_id = client_id;
  }

  /*!
   * Create client with io_context
   * @param io_context asio io_context, async event handler
   */
  coro_rpc_client(
      coro_io::ExecutorWrapper<> *executor = coro_io::get_global_executor(),
      uint64_t client_id = get_global_client_id())
      : control_(
            std::make_shared<control_t>(executor->get_asio_executor(), false)),
        timer_(std::make_unique<coro_io::period_timer>(
            executor->get_asio_executor())) {
    config_.client_id = client_id;
  }

  std::string_view get_host() const { return config_.host; }

  std::string_view get_port() const { return config_.port; }

  const config &get_config() const { return config_; }

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
  [[nodiscard]] bool has_closed() { return control_->has_closed_; }

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
  [[nodiscard]] async_simple::coro::Lazy<coro_rpc::err_code> connect(
      std::string host, std::string port,
      std::chrono::steady_clock::duration timeout_duration =
          std::chrono::seconds(30)) {
    auto lock_ok = connect_mutex_.tryLock();
    if (!lock_ok) {
      co_await connect_mutex_.coScopedLock();
      co_return err_code{};
      // do nothing, someone has reconnect the client
    }
    config_.host = std::move(host);
    config_.port = std::move(port);
    config_.timeout_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    auto ret = co_await connect_impl();
    connect_mutex_.unlock();
    co_return std::move(ret);
  }
  [[nodiscard]] async_simple::coro::Lazy<coro_rpc::err_code> connect(
      std::string_view endpoint,
      std::chrono::steady_clock::duration timeout_duration =
          std::chrono::seconds(30)) {
    auto pos = endpoint.find(':');
    auto lock_ok = connect_mutex_.tryLock();
    if (!lock_ok) {
      co_await connect_mutex_.coScopedLock();
      co_return err_code{};
      // do nothing, someone has reconnect the client
    }
    config_.host = endpoint.substr(0, pos);
    config_.port = endpoint.substr(pos + 1);
    config_.timeout_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    auto ret = co_await connect_impl();
    connect_mutex_.unlock();
    co_return std::move(ret);
  }

  [[nodiscard]] async_simple::coro::Lazy<coro_rpc::err_code> connect() {
    auto lock_ok = connect_mutex_.tryLock();
    if (!lock_ok) {
      co_await connect_mutex_.coScopedLock();
      co_return err_code{};
      // do nothing, someone has reconnect the client
    }
    auto ret = co_await connect_impl();
    connect_mutex_.unlock();
    co_return std::move(ret);
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
   * Call RPC function with default timeout (30 second)
   *
   * @tparam func the address of RPC function
   * @tparam Args the type of arguments
   * @param args RPC function arguments
   * @return RPC call result
   */
  template <auto func, typename... Args>
  async_simple::coro::Lazy<rpc_result<decltype(get_return_type<func>())>> call(
      Args &&...args) {
    return call_for<func>(std::chrono::seconds(30),
                          std::forward<Args>(args)...);
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
    using return_type = decltype(get_return_type<func>());
    auto async_result =
        co_await co_await send_request_for_with_attachment<func, Args...>(
            duration, req_attachment_, std::forward<Args>(args)...);
    req_attachment_ = {};
    if (async_result) {
      control_->resp_buffer_ = async_result->release_buffer();
      if constexpr (std::is_same_v<return_type, void>) {
        co_return expected<return_type, rpc_error>{};
      }
      else {
        co_return expected<return_type, rpc_error>{
            std::move(async_result->result())};
      }
    }
    else {
      co_return expected<return_type, rpc_error>{
          unexpect_t{}, std::move(async_result.error())};
    }
  }

  /*!
   * Get inner executor
   */
  auto &get_executor() { return control_->executor_; }

  uint32_t get_client_id() const { return config_.client_id; }

  void close() {
    // ELOGV(INFO, "client_id %d close", config_.client_id);
    close_socket(control_);
  }

  bool set_req_attachment(std::string_view attachment) {
    if (attachment.size() > UINT32_MAX) {
      ELOGV(ERROR, "too large rpc attachment");
      return false;
    }
    req_attachment_ = attachment;
    return true;
  }

  std::string_view get_resp_attachment() const {
    return control_->resp_buffer_.resp_attachment_buf_;
  }

  std::string release_resp_attachment() {
    return std::move(control_->resp_buffer_.resp_attachment_buf_);
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
    close_socket(control_);
    control_->socket_ =
        asio::ip::tcp::socket(control_->executor_.get_asio_executor());
    control_->is_timeout_ = false;
    control_->has_closed_ = false;
  }
  static bool is_ok(coro_rpc::err_code ec) noexcept { return !ec; }

  [[nodiscard]] async_simple::coro::Lazy<coro_rpc::err_code> connect_impl() {
    if (should_reset_) {
      reset();
    }
    else {
      should_reset_ = true;
    }
#ifdef YLT_ENABLE_SSL
    if (!ssl_init_ret_) {
      std::cout << "ssl_init_ret_: " << ssl_init_ret_ << std::endl;
      co_return errc::not_connected;
    }
#endif
    control_->has_closed_ = false;

    ELOGV(INFO, "client_id %d begin to connect %s", config_.client_id,
          config_.port.data());
    timeout(*this->timer_, config_.timeout_duration, "connect timer canceled")
        .start([](auto &&) {
        });

    std::error_code ec = co_await coro_io::async_connect(
        &control_->executor_, control_->socket_, config_.host, config_.port);
    std::error_code err_code;
    timer_->cancel(err_code);

    if (ec) {
      if (control_->is_timeout_) {
        co_return errc::timed_out;
      }
      co_return errc::not_connected;
    }

    if (control_->is_timeout_) {
      ELOGV(WARN, "client_id %d connect timeout", config_.client_id);
      co_return errc::timed_out;
    }
    if (config_.enable_tcp_no_delay == true) {
      control_->socket_.set_option(asio::ip::tcp::no_delay(true), ec);
    }

#ifdef YLT_ENABLE_SSL
    if (!config_.ssl_cert_path.empty()) {
      assert(control_->ssl_stream_);
      auto shake_ec = co_await coro_io::async_handshake(
          control_->ssl_stream_, asio::ssl::stream_base::client);
      if (shake_ec) {
        ELOGV(WARN, "client_id %d handshake failed: %s", config_.client_id,
              shake_ec.message().data());
        co_return errc::not_connected;
      }
    }
#endif

    co_return coro_rpc::err_code{};
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
      control_->ssl_stream_ =
          std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
              control_->socket_, ssl_ctx_);
      ssl_init_ret_ = true;
    } catch (std::exception &e) {
      ELOGV(ERROR, "init ssl failed: %s", e.what());
    }
    return ssl_init_ret_;
  }
#endif

  async_simple::coro::Lazy<bool> timeout(coro_io::period_timer &timer,
                                         auto duration, std::string err_msg) {
    timer.expires_after(duration);
    std::weak_ptr socket_watcher = control_;
    bool is_timeout = co_await timer.async_await();
    if (!is_timeout) {
      co_return false;
    }
    if (auto self = socket_watcher.lock()) {
      self->is_timeout_ = is_timeout;
      close_socket(self);
      co_return true;
    }
    co_return false;
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

  /*
   * buffer layout
   * ┌────────────────┬────────────────┐
   * │req_header      │args            │
   * ├────────────────┼────────────────┤
   * │REQ_HEADER_LEN  │variable length │
   * └────────────────┴────────────────┘
   */
  template <auto func, typename... Args>
  std::vector<std::byte> prepare_buffer(uint32_t &id, Args &&...args) {
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
    header.attach_length = req_attachment_.size();
    id = request_id_++;
    ELOG_TRACE << "send request ID:" << id << ".";
    header.seq_num = id;

#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::client_send_bad_magic_num) {
      header.magic = coro_rpc_protocol::magic_number + 1;
    }
    if (g_action == inject_action::client_send_header_length_0) {
      header.length = 0;
    }
    else {
#endif
      auto sz = buffer.size() - coro_rpc_protocol::REQ_HEAD_LEN;
      if (sz > UINT32_MAX) {
        ELOGV(ERROR, "too large rpc body");
        return {};
      }
      header.length = sz;
#ifdef UNIT_TEST_INJECT
    }
#endif
    return buffer;
  }

  template <typename T>
  static rpc_result<T> handle_response_buffer(std::string_view buffer,
                                              uint8_t rpc_errc,
                                              bool &has_error) {
    rpc_return_type_t<T> ret;
    struct_pack::err_code ec;
    rpc_error err;
    if (rpc_errc == 0)
      AS_LIKELY {
        ec = struct_pack::deserialize_to(ret, buffer);
        if SP_LIKELY (!ec) {
          if constexpr (std::is_same_v<T, void>) {
            return {};
          }
          else {
            return std::move(ret);
          }
        }
      }
    else {
      if (rpc_errc != UINT8_MAX) {
        err.val() = rpc_errc;
        ec = struct_pack::deserialize_to(err.msg, buffer);
        if SP_LIKELY (!ec) {
          has_error = true;
          return rpc_result<T>{unexpect_t{}, std::move(err)};
        }
      }
      else {
        ec = struct_pack::deserialize_to(err, buffer);
        if SP_LIKELY (!ec) {
          return rpc_result<T>{unexpect_t{}, std::move(err)};
        }
      }
    }
    has_error = true;
    // deserialize failed.
    ELOGV(WARNING, "deserilaize rpc result failed");
    err = {errc::invalid_rpc_result, "failed to deserialize rpc return value"};
    return rpc_result<T>{unexpect_t{}, std::move(err)};
  }

  template <typename FuncArgs>
  auto get_func_args() {
    using First = std::tuple_element_t<0, FuncArgs>;
    constexpr bool has_conn_v = requires { typename First::return_type; };
    return util::get_args<has_conn_v, FuncArgs>();
  }
  template <typename T, typename U>
  static decltype(auto) add_const(U &&u) {
    if constexpr (std::is_const_v<std::remove_reference_t<U>>) {
      return struct_pack::detail::declval<const T>();
    }
    else {
      return struct_pack::detail::declval<T>();
    }
  }

  template <typename... FuncArgs, typename Buffer, typename... Args>
  void pack_to_impl(Buffer &buffer, std::size_t offset, Args &&...args) {
    struct_pack::serialize_to_with_offset(
        buffer, offset,
        std::forward<decltype(add_const<FuncArgs>(args))>(
            (std::forward<Args>(args)))...);
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

  struct async_rpc_raw_result_value_type {
    resp_body buffer_;
    uint8_t errc_;
  };

  using async_rpc_raw_result =
      std::variant<async_rpc_raw_result_value_type, std::error_code>;

  struct control_t;

  struct handler_t {
    std::unique_ptr<coro_io::period_timer> timer_;
    async_simple::Promise<async_rpc_raw_result> promise_;
    handler_t(std::unique_ptr<coro_io::period_timer> &&timer,
              async_simple::Promise<async_rpc_raw_result> &&promise)
        : timer_(std::move(timer)), promise_(std::move(promise)) {}
    void operator()(resp_body &&buffer, uint8_t rpc_errc) {
      timer_->cancel();
      promise_.setValue(async_rpc_raw_result{
          async_rpc_raw_result_value_type{std::move(buffer), rpc_errc}});
    }
    void local_error(std::error_code &ec) {
      timer_->cancel();
      promise_.setValue(async_rpc_raw_result{ec});
    }
  };
  struct control_t {
#ifdef YLT_ENABLE_SSL
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_;
#endif
#ifdef GENERATE_BENCHMARK_DATA
    std::string func_name_;
#endif
    bool is_timeout_;
    std::atomic<bool> has_closed_ = false;
    coro_io::ExecutorWrapper<> executor_;
    std::unordered_map<uint32_t, handler_t> response_handler_table_;
    resp_body resp_buffer_;
    asio::ip::tcp::socket socket_;
    std::atomic<uint32_t> recving_cnt_ = 0;
    control_t(asio::io_context::executor_type executor, bool is_timeout)
        : socket_(executor),
          is_timeout_(is_timeout),
          has_closed_(false),
          executor_(executor) {}
  };

  static void close_socket(
      std::shared_ptr<coro_rpc_client::control_t> control) {
    bool expected = false;
    if (!control->has_closed_.compare_exchange_strong(expected, true)) {
      return;
    }
    control->executor_.schedule([control]() {
      asio::error_code ignored_ec;
      control->socket_.shutdown(asio::ip::tcp::socket::shutdown_both,
                                ignored_ec);
      control->socket_.close(ignored_ec);
    });
  }

#ifdef UNIT_TEST_INJECT
 public:
  coro_rpc::err_code sync_connect(const std::string &host,
                                  const std::string &port) {
    return async_simple::coro::syncAwait(connect(host, port));
  }

  template <auto func, typename... Args>
  rpc_result<decltype(get_return_type<func>())> sync_call(Args &&...args) {
    return async_simple::coro::syncAwait(
        call<func>(std::forward<Args>(args)...));
  }
#endif
 private:
  template <auto func, typename... Args>
  async_simple::coro::Lazy<rpc_error> send_request_for_impl(
      auto duration, uint32_t &id, coro_io::period_timer &timer,
      std::string_view attachment, Args &&...args) {
    using R = decltype(get_return_type<func>());

    if (control_->has_closed_)
      AS_UNLIKELY {
        ELOGV(ERROR, "client has been closed, please re-connect");
        co_return rpc_error{errc::io_error,
                            "client has been closed, please re-connect"};
      }

#ifdef YLT_ENABLE_SSL
    if (!ssl_init_ret_) {
      co_return rpc_error{errc::not_connected};
    }
#endif

    static_check<func, Args...>();

    if (duration.count() > 0) {
      timeout(timer, duration, "rpc call timer canceled").start([](auto &&) {
      });
    }

#ifdef YLT_ENABLE_SSL
    if (!config_.ssl_cert_path.empty()) {
      assert(control_->ssl_stream_);
      co_return co_await send_impl<func>(*control_->ssl_stream_, id, attachment,
                                         std::forward<Args>(args)...);
    }
    else {
#endif
      co_return co_await send_impl<func>(control_->socket_, id, attachment,
                                         std::forward<Args>(args)...);
#ifdef YLT_ENABLE_SSL
    }
#endif
  }

  static void send_err_response(control_t *controller, std::error_code &errc) {
    if (controller->is_timeout_) {
      errc = std::make_error_code(std::errc::timed_out);
    }
    for (auto &e : controller->response_handler_table_) {
      e.second.local_error(errc);
    }
    controller->response_handler_table_.clear();
  }
  template <typename Socket>
  static async_simple::coro::Lazy<void> recv(
      std::shared_ptr<control_t> controller, Socket &socket) {
    std::pair<std::error_code, std::size_t> ret;
    do {
      coro_rpc_protocol::resp_header header;
      ret = co_await coro_io::async_read(
          socket,
          asio::buffer((char *)&header, coro_rpc_protocol::RESP_HEAD_LEN));
      if (ret.first) {
        ELOG_ERROR << "read rpc head failed, error msg:" << ret.first.message()
                   << ". close the socket.value=" << ret.first.value();
        break;
      }
      uint32_t body_len = header.length;
      struct_pack::detail::resize(
          controller->resp_buffer_.read_buf_,
          std::max<uint32_t>(body_len, sizeof(std::string)));
      if (body_len < sizeof(std::string)) { /* this strange code just disable
                                             any SSO optimize so that rpc result
                                             wont point to illegal address*/
        controller->resp_buffer_.read_buf_.resize(body_len);
      }
      if (header.attach_length == 0) {
        ret = co_await coro_io::async_read(
            socket,
            asio::buffer(controller->resp_buffer_.read_buf_.data(), body_len));
        controller->resp_buffer_.resp_attachment_buf_.clear();
      }
      else {
        struct_pack::detail::resize(
            controller->resp_buffer_.resp_attachment_buf_,
            header.attach_length);
        std::array<asio::mutable_buffer, 2> iov{
            asio::mutable_buffer{controller->resp_buffer_.read_buf_.data(),
                                 body_len},
            asio::mutable_buffer{
                controller->resp_buffer_.resp_attachment_buf_.data(),
                controller->resp_buffer_.resp_attachment_buf_.size()}};
        ret = co_await coro_io::async_read(socket, iov);
      }
      if (ret.first) {
        ELOG_ERROR << "read rpc body failed, error msg:" << ret.first.message()
                   << ". close the socket.";
        break;
      }
#ifdef GENERATE_BENCHMARK_DATA
      std::ofstream file(benchmark_file_path + controller->func_name_ + ".out",
                         std::ofstream::binary | std::ofstream::out);
      file << std::string_view{(char *)&header,
                               coro_rpc_protocol::RESP_HEAD_LEN};
      file << controller->resp_buffer_.read_buf_;
      file << controller->resp_buffer_.resp_attachment_buf_;
      file.close();
#endif
      --controller->recving_cnt_;
      if (auto iter = controller->response_handler_table_.find(header.seq_num);
          iter != controller->response_handler_table_.end()) {
        ELOG_TRACE << "find request ID:" << header.seq_num
                   << ". start notify response handler";
        iter->second(std::move(controller->resp_buffer_), header.err_code);
        controller->response_handler_table_.erase(iter);
        if (controller->response_handler_table_.empty()) {
          co_return;
        }
      }
      else {
        ELOG_ERROR << "unexists request ID:" << header.seq_num
                   << ". close the socket.";
        break;
      }
    } while (true);
    close_socket(controller);
    send_err_response(controller.get(), ret.first);
    co_return;
  }

  template <typename T>
  static async_simple::coro::Lazy<async_rpc_result<T>> deserialize_rpc_result(
      async_simple::Future<async_rpc_raw_result> future,
      std::weak_ptr<control_t> watcher) {
    auto ret_ = co_await std::move(future);

    if (ret_.index() == 1) [[unlikely]] {  // local error
      auto &ret = std::get<1>(ret_);
      if (ret.value() == static_cast<int>(std::errc::operation_canceled) ||
          ret.value() == static_cast<int>(std::errc::timed_out)) {
        co_return coro_rpc::unexpected<rpc_error>{
            rpc_error{errc::timed_out, ret.message()}};
      }
      else {
        co_return coro_rpc::unexpected<rpc_error>{
            rpc_error{errc::io_error, ret.message()}};
      }
    }

    bool has_error = false;
    auto &ret = std::get<0>(ret_);
    auto result =
        handle_response_buffer<T>(ret.buffer_.read_buf_, ret.errc_, has_error);
    if (has_error) {
      if (auto w = watcher.lock(); w) {
        close_socket(std::move(w));
      }
    }
    if (result) {
      if constexpr (std::is_same_v<T, void>) {
        co_return async_rpc_result<T>{
            async_rpc_result_value_t<T>{std::move(ret.buffer_)}};
      }
      else {
        co_return async_rpc_result<T>{async_rpc_result_value_t<T>{
            std::move(result.value()), std::move(ret.buffer_)}};
      }
    }
    else {
      co_return coro_rpc::unexpected<rpc_error>{result.error()};
    }
  }

 public:
  template <auto func, typename... Args>
  async_simple::coro::Lazy<async_simple::coro::Lazy<
      async_rpc_result<decltype(get_return_type<func>())>>>
  send_request(Args &&...args) {
    return send_request_for_with_attachment<func>(std::chrono::seconds{30}, {},
                                                  std::forward<Args>(args)...);
  }

  template <auto func, typename... Args>
  async_simple::coro::Lazy<async_simple::coro::Lazy<
      async_rpc_result<decltype(get_return_type<func>())>>>
  send_request_with_attachment(std::string_view request_attachment,
                               Args &&...args) {
    return send_request_for_with_attachment<func>(std::chrono::seconds{30},
                                                  request_attachment,
                                                  std::forward<Args>(args)...);
  }

  template <auto func, typename... Args>
  async_simple::coro::Lazy<async_simple::coro::Lazy<
      async_rpc_result<decltype(get_return_type<func>())>>>
  send_request_for(Args &&...args) {
    return send_request_for_with_attachment<func>(std::chrono::seconds{30},
                                                  std::string_view{},
                                                  std::forward<Args>(args)...);
  }

  struct recving_guard {
    recving_guard(control_t *ctrl) : ctrl_(ctrl) { ctrl_->recving_cnt_++; }
    void release() { ctrl_ = nullptr; }
    ~recving_guard() {
      if (ctrl_) {
        --ctrl_->recving_cnt_;
      }
    }
    control_t *ctrl_;
  };

 private:
  template <typename T>
  static async_simple::coro::Lazy<async_rpc_result<T>> build_failed_rpc_result(
      rpc_error err) {
    co_return unexpected<coro_rpc::rpc_error>{err};
  }

 public:
  template <auto func, typename... Args>
  async_simple::coro::Lazy<async_simple::coro::Lazy<
      async_rpc_result<decltype(get_return_type<func>())>>>
  send_request_for_with_attachment(auto time_out_duration,
                                   std::string_view request_attachment,
                                   Args &&...args) {
    using rpc_return_t = decltype(get_return_type<func>());
    recving_guard guard(control_.get());
    uint32_t id;
    auto timer = std::make_unique<coro_io::period_timer>(
        control_->executor_.get_asio_executor());
    auto result = co_await send_request_for_impl<func>(
        time_out_duration, id, *timer, request_attachment,
        std::forward<Args>(args)...);
    auto &control = *control_;
    if (!result) {
      async_simple::Promise<async_rpc_raw_result> promise;
      auto future = promise.getFuture();
      bool is_empty = control.response_handler_table_.empty();
      auto &&[_, is_ok] = control.response_handler_table_.try_emplace(
          id, std::move(timer), std::move(promise));
      if (!is_ok) [[unlikely]] {
        close();
        co_return build_failed_rpc_result<rpc_return_t>(
            rpc_error{coro_rpc::errc::serial_number_conflict});
      }
      else {
        if (is_empty) {
#ifdef YLT_ENABLE_SSL
          if (!config_.ssl_cert_path.empty()) {
            assert(control.ssl_stream_);
            recv(control_, *control.ssl_stream_).start([](auto &&) {
            });
          }
          else {
#endif
            recv(control_, control.socket_).start([](auto &&) {
            });
#ifdef YLT_ENABLE_SSL
          }
#endif
        }
        guard.release();
        co_return deserialize_rpc_result<rpc_return_t>(
            std::move(future), std::weak_ptr<control_t>{control_});
      }
    }
    else {
      auto i = build_failed_rpc_result<rpc_return_t>(std::move(result));
      co_return build_failed_rpc_result<rpc_return_t>(std::move(result));
    }
  }

  uint32_t get_pipeline_size() { return control_->recving_cnt_; }

 private:
  template <auto func, typename Socket, typename... Args>
  async_simple::coro::Lazy<rpc_error> send_impl(Socket &socket, uint32_t &id,
                                                std::string_view req_attachment,
                                                Args &&...args) {
    auto buffer = prepare_buffer<func>(id, std::forward<Args>(args)...);
    if (buffer.empty()) {
      co_return rpc_error{errc::message_too_large};
    }
#ifdef GENERATE_BENCHMARK_DATA
    control_->func_name_ = get_func_name<func>();
    std::ofstream file(benchmark_file_path + control_->func_name_ + ".in",
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
      co_return rpc_error{errc::io_error, ret.first.message()};
    }
    else if (g_action ==
             inject_action::client_close_socket_after_send_partial_header) {
      ret = co_await coro_io::async_write(
          socket,
          asio::buffer(buffer.data(), coro_rpc_protocol::REQ_HEAD_LEN - 1));
      ELOGV(INFO, "client_id %d close socket", config_.client_id);
      close();
      co_return rpc_error{errc::io_error, ret.first.message()};
    }
    else if (g_action ==
             inject_action::client_shutdown_socket_after_send_header) {
      ret = co_await coro_io::async_write(
          socket, asio::buffer(buffer.data(), coro_rpc_protocol::REQ_HEAD_LEN));
      ELOGV(INFO, "client_id %d shutdown", config_.client_id);
      control_->socket_.shutdown(asio::ip::tcp::socket::shutdown_send);
      co_return rpc_error{errc::io_error, ret.first.message()};
    }
    else {
#endif
      if (req_attachment.empty()) {
        while (true) {
          bool expected = false;
          if (write_mutex_.compare_exchange_weak(expected, true)) {
            break;
          }
          co_await coro_io::post(
              []() {
              },
              &control_->executor_);
        }
        ret = co_await coro_io::async_write(
            socket, asio::buffer(buffer.data(), buffer.size()));
        write_mutex_ = false;
      }
      else {
        std::array<asio::const_buffer, 2> iov{
            asio::const_buffer{buffer.data(), buffer.size()},
            asio::const_buffer{req_attachment.data(), req_attachment.size()}};
        while (true) {
          bool expected = false;
          if (write_mutex_.compare_exchange_weak(expected, true)) {
            break;
          }
          co_await coro_io::post(
              []() {
              },
              &control_->executor_);
        }
        ret = co_await coro_io::async_write(socket, iov);
        write_mutex_ = false;
      }
#ifdef UNIT_TEST_INJECT
    }
#endif
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::force_inject_client_write_data_timeout) {
      control_->is_timeout_ = true;
    }
#endif
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::client_close_socket_after_send_payload) {
      ELOGV(INFO, "client_id %d client_close_socket_after_send_payload",
            config_.client_id);
      close();
      co_return rpc_error{errc::io_error, ret.first.message()};
    }
#endif
    if (ret.first) {
      close();
      if (control_->is_timeout_) {
        co_return rpc_error{errc::timed_out};
      }
      else {
        co_return rpc_error{errc::io_error, ret.first.message()};
      }
    }
    co_return rpc_error{};
  }

 private:
  bool should_reset_ = false;
  async_simple::coro::Mutex connect_mutex_;
  std::atomic<bool> write_mutex_ = false;
  std::atomic<uint32_t> request_id_{0};
  std::unique_ptr<coro_io::period_timer> timer_;
  std::shared_ptr<control_t> control_;
  std::string_view req_attachment_;
  config config_;
  constexpr static std::size_t default_read_buf_size_ = 256;
#ifdef YLT_ENABLE_SSL
  asio::ssl::context ssl_ctx_{asio::ssl::context::sslv23};
  bool ssl_init_ret_ = true;
#endif
};
}  // namespace coro_rpc
