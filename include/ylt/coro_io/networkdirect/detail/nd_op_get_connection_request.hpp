/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
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

#include <algorithm>
#include <array>
#include <span>

#include "asio/detail/bind_handler.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_work.hpp"
#include "asio/detail/memory.hpp"
#include "nd_op_base.hpp"
#include "nd_service_connector.hpp"

namespace coro_io::detail {

template <typename Handler, typename IoExecutor>
class nd_get_connection_request_op final : public nd_op_base {
public:
  ASIO_DEFINE_HANDLER_PTR(nd_get_connection_request_op);

  nd_get_connection_request_op(IND2Listener* listener,
                               nd_connector_handle_t&& connector_handle,
                               Handler& handler, const IoExecutor& io_ex)
      : nd_op_base(listener, &nd_op_base::default_process,
                   &nd_get_connection_request_op::do_complete)
      , connector_handle_(std::move(connector_handle))
      , handler_(ASIO_MOVE_CAST(Handler)(handler))
      , work_(handler_, io_ex) {
  }

  nd_connector_handle_t& get_connector_handle() {
    return connector_handle_;
  }

private:
  nd_connector_handle_t connector_handle_;
  std::array<std::byte, max_private_data_size> private_data_buf_{};
  std::size_t private_data_len_ = 0;
  Handler handler_;
  asio::detail::handler_work<Handler, IoExecutor> work_;

  static void do_complete(void* owner, asio::detail::operation* base,
                          const asio::error_code& result_ec,
                          std::size_t /*bytes_transferred*/) {
    asio::error_code ec = result_ec;
    auto* o = static_cast<nd_get_connection_request_op*>(base);

    // Retrieve private data from the connector before moving it to the wrapper.
    if (!ec && o->connector_handle_.connector_) {
      auto* connector = o->connector_handle_.connector_.Get();
      ULONG pd_size = static_cast<ULONG>(o->private_data_buf_.size());
      auto const hr = connector->GetPrivateData(o->private_data_buf_.data(),
                                                &pd_size);
      if (SUCCEEDED(hr) || hr == ND_BUFFER_OVERFLOW) {
        o->private_data_len_ =
            (std::min)(static_cast<std::size_t>(pd_size),
                       o->private_data_buf_.size());
      }
    }

    auto pd_buf = o->private_data_buf_;
    std::size_t const pd_len = o->private_data_len_;

    nd_connector_handle_t connector = std::move(o->connector_handle_);

    ptr p = {asio::detail::addressof(o->handler_), o, o};

    ASIO_HANDLER_COMPLETION((*o));

    asio::detail::handler_work<Handler, IoExecutor> w(
        ASIO_MOVE_CAST2(asio::detail::handler_work<Handler, IoExecutor>)(
            o->work_));

    ASIO_ERROR_LOCATION(ec);

    Handler handler(ASIO_MOVE_CAST(Handler)(o->handler_));
    p.h = asio::detail::addressof(handler);
    p.reset();

    if (owner) {
      std::span<const std::byte> private_data(pd_buf.data(), pd_len);
      asio::detail::fenced_block b(asio::detail::fenced_block::half);
      ASIO_HANDLER_INVOCATION_BEGIN((ec));
      handler(ec, std::move(connector), private_data);
      ASIO_HANDLER_INVOCATION_END;
    }
  }
};

}  // namespace coro_io::detail
