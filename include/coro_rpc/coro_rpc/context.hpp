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

#include <any>
#include <atomic>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "async_simple/coro/Lazy.h"
#include "coro_connection.hpp"
#include "easylog/easylog.h"
#include "util/type_traits.h"

namespace coro_rpc {
template <typename T, typename Conn>
concept has_get_reserve_size = requires(Conn &&conn) {
  T::get_reserve_size(conn);
};
/*!
 *
 * @tparam return_msg_type
 * @tparam coro_connection
 */
template <typename return_msg_type, typename rpc_protocol>
class context_base {
 protected:
  std::shared_ptr<coro_connection> conn_;
  std::unique_ptr<std::atomic<bool>> has_response_;
  typename rpc_protocol::req_header req_head_;
  bool is_delay = false;

 public:
  /*!
   * Construct a context by a share pointer of context Concept
   * instance
   * @param a share pointer for coro_connection
   */
  context_base(std::shared_ptr<coro_connection> &&conn,
               typename rpc_protocol::req_header &&req_head)
      : conn_(std::move(conn)),
        has_response_(std::make_unique<std::atomic<bool>>(false)),
        req_head_(std::move(req_head)) {
    if (conn_) {
      conn_->set_rpc_call_type(coro_connection::rpc_call_type::callback);
    }
  };
  context_base() = delete;
  context_base(const context_base &conn) = delete;
  context_base(context_base &&conn) = default;
  ~context_base() {
    if (has_response_ && conn_ && !*has_response_)
      AS_UNLIKELY {
        ELOGV(ERROR,
              "We must send reply to client by call response_msg method"
              "before coro_rpc::context<T> destruction!");
        conn_->async_close();
      }
  }

  using return_type = return_msg_type;

  /*!
   * Send response message
   *
   * The `return_msg_type` must be constructed by these `args`.
   *
   * If the connection has already sent response message,
   * an error log will be reported.
   *
   * @tparam Args
   * @param args
   */
  template <typename... Args>
  void response_msg(Args &&...args) {
    if constexpr (std::is_same_v<return_msg_type, void>) {
      static_assert(sizeof...(args) == 0, "illegal args");

      auto old_flag = has_response_->exchange(true);
      if (old_flag != false) {
        ELOGV(ERROR, "response message more than one time");
        return;
      }

      if (has_closed())
        AS_UNLIKELY {
          ELOGV(DEBUG, "response_msg failed: connection has been closed");
          return;
        }
      std::visit(
          [&]<typename serialize_proto>(const serialize_proto &) {
            conn_->response_msg<rpc_protocol>(serialize_proto::serialize(),
                                              req_head_, is_delay);
          },
          *rpc_protocol::get_serialize_protocol(req_head_));
    }
    else {
      static_assert(
          requires { return_msg_type{std::forward<Args>(args)...}; },
          "constructed return_msg_type failed by illegal args");
      return_msg_type ret{std::forward<Args>(args)...};

      auto old_flag = has_response_->exchange(true);
      if (old_flag != false) {
        ELOGV(ERROR, "response message more than one time");
        return;
      }

      if (has_closed())
        AS_UNLIKELY {
          ELOGV(DEBUG, "response_msg failed: connection has been closed");
          return;
        }

      if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
        std::visit(
            [&]<typename serialize_proto>(const serialize_proto &) {
              conn_->response_msg<rpc_protocol>(
                  serialize_proto::serialize(
                      ret, rpc_protocol::get_reserve_size(conn_)),
                  req_head_, is_delay);
            },
            *rpc_protocol::get_serialize_protocol(req_head_));
      }
      else {
        std::visit(
            [&]<typename serialize_proto>(const serialize_proto &) {
              conn_->response_msg<rpc_protocol>(serialize_proto::serialize(ret),
                                                req_head_, is_delay);
            },
            *rpc_protocol::get_serialize_protocol(req_head_));
      }
      // response_handler_(std::move(conn_), std::move(ret));
    }
  }

  /*!
   * Check connection closed or not
   *
   * @return true if closed, otherwise false
   */
  bool has_closed() const { return conn_->has_closed(); }

  void set_delay() {
    is_delay = true;
    conn_->set_rpc_call_type(
        coro_connection::rpc_call_type::callback_with_delay);
  }

  template <typename T>
  void set_tag(T &&tag) {
    conn_->set_tag(std::forward<T>(tag));
  }

  std::any get_tag() { return conn_->get_tag(); }
};

template <typename T>
struct get_type_t {
  using type = T;
};

template <typename T>
struct get_type_t<async_simple::coro::Lazy<T>> {
  using type = T;
};

template <auto func>
inline auto get_return_type() {
  using T = decltype(func);
  using param_type = function_parameters_t<T>;
  using return_type = typename get_type_t<function_return_type_t<T>>::type;
  if constexpr (std::is_void_v<param_type>) {
    if constexpr (std::is_void_v<return_type>) {
      return;
    }
    else {
      return return_type{};
    }
  }
  else {
    using First = std::tuple_element_t<0, param_type>;
    constexpr bool is_conn = is_specialization<First, context_base>::value;

    if constexpr (is_conn) {
      using U = typename First::return_type;
      if constexpr (std::is_void_v<U>) {
        return;
      }
      else {
        return U{};
      }
    }
    else if constexpr (!std::is_same_v<void, return_type>) {
      return return_type{};
    }
    else {
      return;
    }
  }
}
}  // namespace coro_rpc