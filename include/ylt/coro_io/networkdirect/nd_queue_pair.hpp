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

#include "asio/async_result.hpp"
#include "asio/io_context.hpp"
#include "asio/system_executor.hpp"
#include "nd_completion_queue.hpp"
#include "nd_error.hpp"
#include "detail/nd_service_device.hpp"
#include "detail/nd_service_io_completion.hpp"
#include "detail/nd_service_verbs.hpp"

namespace coro_io {

// Queue pair IO object. Two mutually-exclusive modes, distinguished by io_ctx_:
//
//   nd_queue_pair(io_context&)        event mode -- completions via use_device's
//                                      shared CQ (IOCP).
//   nd_queue_pair(completion_queue&)  poll mode  -- io_context-free data plane;
//                                      completions reaped by the user's poll().
//
// No io_object_impl: the QP owns the verbs-service implementation_type directly
// and selects the static (poll) or member (event) service entry points per call.
// Unlike ibv, nd creates and OWNS the native QP at construction (impl_.qp_); the
// connector borrows it via native_handle(). Rule of five = move impl_ + io_ctx_
// (the nd2 smart ptr in impl_ moves/frees the QP). Config is read from the holder
// (use_device / completion_queue), not passed here.
class nd_queue_pair {
public:
  using service_type = detail::nd_verbs_service;

  nd_queue_pair() = default;
  ~nd_queue_pair() = default;

  nd_queue_pair(nd_queue_pair&& other) noexcept
      : impl_(std::move(other.impl_))
      , io_ctx_(other.io_ctx_)
      , verbs_svc_(other.verbs_svc_) {
    other.impl_ = impl_type{};
    other.io_ctx_ = nullptr;
    other.verbs_svc_ = nullptr;
  }
  nd_queue_pair& operator=(nd_queue_pair&& other) noexcept {
    if (this != &other) {
      impl_ = std::move(other.impl_);
      io_ctx_ = other.io_ctx_;
      verbs_svc_ = other.verbs_svc_;
      other.impl_ = impl_type{};
      other.io_ctx_ = nullptr;
      other.verbs_svc_ = nullptr;
    }
    return *this;
  }
  nd_queue_pair(nd_queue_pair const&) = delete;
  nd_queue_pair& operator=(nd_queue_pair const&) = delete;

  // event mode
  explicit nd_queue_pair(asio::io_context& io_ctx) {
    asio::error_code ec;
    bind(io_ctx, ec);
    asio::detail::throw_error(ec);
  }

  // poll mode (no io_context)
  explicit nd_queue_pair(nd_completion_queue& cq) {
    asio::error_code ec;
    bind(cq, ec);
    asio::detail::throw_error(ec);
  }

  // deferred bind -- event mode (the io_context's managed CQ)
  void bind(asio::io_context& io_ctx) {
    asio::error_code ec;
    bind(io_ctx, ec);
    asio::detail::throw_error(ec);
  }

  void bind(asio::io_context& io_ctx, asio::error_code& ec) {
    if (is_bound()) {
      ec = asio::error::already_open;
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    auto& dev_svc = asio::use_service<detail::nd_device_service>(io_ctx);
    if (!dev_svc.is_registered()) {
      ec = rdma_errc::device_not_registered;
      ASIO_ERROR_LOCATION(ec);
      return;
    }
    auto& io_svc =
        asio::use_service<detail::nd_io_completion_service>(io_ctx);
    io_ctx_ = &io_ctx;
    impl_.device_ = dev_svc.get_device();
    impl_.cq_ = io_svc.get_cq();
    impl_.config_ = dev_svc.get_effective_config();
    impl_.poll_cq_ = nullptr;
    // Cache the verbs service once -- the event-mode async_* path uses it per op.
    verbs_svc_ = &asio::use_service<detail::nd_verbs_service>(io_ctx);
    ec = detail::nd_verbs_service::create_qp(impl_);
    if (!ec) {
      // Start the shared-CQ poller (idempotent); the data plane never arms again.
      io_svc.ensure_poller_started();
    }
  }

  // deferred bind -- poll mode (a user-owned completion_queue)
  void bind(nd_completion_queue& cq) {
    asio::error_code ec;
    bind(cq, ec);
    asio::detail::throw_error(ec);
  }

  void bind(nd_completion_queue& cq, asio::error_code& ec) {
    if (is_bound()) {
      ec = asio::error::already_open;
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    io_ctx_ = nullptr;
    verbs_svc_ = nullptr;  // poll mode uses the static service entry points
    impl_.device_ = cq.device();
    impl_.cq_ = cq.native_handle();
    impl_.config_ = cq.effective_config();
    impl_.poll_cq_ = &cq;
    ec = detail::nd_verbs_service::create_qp(impl_);
  }

  // Bound to a completion mechanism?
  bool is_bound() const noexcept {
    return detail::nd_verbs_service::is_bound(impl_);
  }

  // Which completion mechanism this QP is bound to.
  completion_mode bound_type() const noexcept {
    if (!is_bound()) return completion_mode::none;
    return io_ctx_ ? completion_mode::event : completion_mode::poll;
  }

  detail::native_qp_t* native_handle() const noexcept {
    return detail::nd_verbs_service::native_handle(impl_);
  }

  template <typename ConstBufferSequence, typename WriteToken>
  auto async_send(ConstBufferSequence const& buffers, WriteToken&& token) {
    return asio::async_initiate<WriteToken,
                                void(asio::error_code, std::size_t)>(
        [this](auto handler, auto const& bufs) {
          if (io_ctx_) {
            (*verbs_svc_)
                .async_send(impl_, bufs, handler, io_ctx_->get_executor());
          } else {
            detail::nd_verbs_service::async_send_static(
                impl_, bufs, handler, asio::system_executor{});
          }
        },
        token, buffers);
  }

  template <typename MutableBufferSequence, typename ReadToken>
  auto async_recv(MutableBufferSequence const& buffers, ReadToken&& token) {
    return asio::async_initiate<ReadToken,
                                void(asio::error_code, std::size_t)>(
        [this](auto handler, auto const& bufs) {
          if (io_ctx_) {
            (*verbs_svc_)
                .async_recv(impl_, bufs, handler, io_ctx_->get_executor());
          } else {
            detail::nd_verbs_service::async_recv_static(
                impl_, bufs, handler, asio::system_executor{});
          }
        },
        token, buffers);
  }

  template <typename ConstBufferSequence, typename WriteToken>
  auto async_write(ConstBufferSequence const& buffers,
                   nd_remote_addr_t const& remote_addr, WriteToken&& token) {
    return asio::async_initiate<WriteToken,
                                void(asio::error_code, std::size_t)>(
        [this, &remote_addr](auto handler, auto const& bufs) {
          if (io_ctx_) {
            (*verbs_svc_)
                .async_write(impl_, bufs, remote_addr, handler,
                             io_ctx_->get_executor());
          } else {
            detail::nd_verbs_service::async_write_static(
                impl_, bufs, remote_addr, handler, asio::system_executor{});
          }
        },
        token, buffers);
  }

  template <typename MutableBufferSequence, typename ReadToken>
  auto async_read(MutableBufferSequence const& buffers,
                  nd_remote_addr_t const& remote_addr, ReadToken&& token) {
    return asio::async_initiate<ReadToken,
                                void(asio::error_code, std::size_t)>(
        [this, &remote_addr](auto handler, auto const& bufs) {
          if (io_ctx_) {
            (*verbs_svc_)
                .async_read(impl_, bufs, remote_addr, handler,
                            io_ctx_->get_executor());
          } else {
            detail::nd_verbs_service::async_read_static(
                impl_, bufs, remote_addr, handler, asio::system_executor{});
          }
        },
        token, buffers);
  }

private:
  using impl_type = detail::nd_verbs_service::implementation_type;
  impl_type impl_;
  asio::io_context* io_ctx_ = nullptr;             // null => poll mode
  detail::nd_verbs_service* verbs_svc_ = nullptr;  // cached in bind(io) (event mode)
};

}  // namespace coro_io
