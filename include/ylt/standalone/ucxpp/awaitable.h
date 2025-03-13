#pragma once

#include <ucp/api/ucp.h>
#include <ucs/type/status.h>

#include <cassert>
#include <coroutine>
#include <cstddef>
#include <cstdint>

#include "ucxpp/error.h"

namespace ucxpp {

class base_awaitable {
 protected:
  std::coroutine_handle<> h_;
  ucs_status_t status_;
  base_awaitable() : h_(nullptr), status_(UCS_OK) {}
  bool check_request_ready(ucs_status_ptr_t request) {
    if (UCS_PTR_IS_PTR(request)) [[unlikely]] {
      status_ = UCS_INPROGRESS;
      return false;
    }
    else if (UCS_PTR_IS_ERR(request)) [[unlikely]] {
      status_ = UCS_PTR_STATUS(request);
      return true;
    }

    status_ = UCS_OK;
    return true;
  }
};

/* Common awaitable class for send-like callbacks */
template <class Derived>
class send_awaitable : public base_awaitable {
 public:
  static void send_cb(void *request, ucs_status_t status, void *user_data) {
    auto self = reinterpret_cast<Derived *>(user_data);
    self->status_ = status;
    ::ucp_request_free(request);
    self->h_.resume();
  }

  ucp_request_param_t build_param() {
    ucp_request_param_t send_param;
    send_param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                              UCP_OP_ATTR_FIELD_USER_DATA |
                              UCP_OP_ATTR_FLAG_MULTI_SEND;
    send_param.cb.send = &send_cb;
    send_param.user_data = this;
    return send_param;
  }

  bool await_suspend(std::coroutine_handle<> h) {
    h_ = h;
    return status_ == UCS_INPROGRESS;
  }

  void await_resume() const { check_ucs_status(status_, "operation failed"); }
};

class stream_send_awaitable : public send_awaitable<stream_send_awaitable> {
  ucp_ep_h ep_;
  void const *buffer_;
  size_t length_;
  friend class send_awaitable;

 public:
  stream_send_awaitable(ucp_ep_h ep, void const *buffer, size_t length)
      : ep_(ep), buffer_(buffer), length_(length) {}

  bool await_ready() noexcept {
    auto send_param = build_param();
    auto request = ::ucp_stream_send_nbx(ep_, buffer_, length_, &send_param);
    return check_request_ready(request);
  }
};

class tag_send_awaitable : public send_awaitable<tag_send_awaitable> {
  ucp_ep_h ep_;
  ucp_tag_t tag_;
  void const *buffer_;
  size_t length_;
  friend class send_awaitable;

 public:
  tag_send_awaitable(ucp_ep_h ep, void const *buffer, size_t length,
                     ucp_tag_t tag)
      : ep_(ep), tag_(tag), buffer_(buffer), length_(length) {}

  bool await_ready() noexcept {
    auto send_param = build_param();
    auto request = ::ucp_tag_send_nbx(ep_, buffer_, length_, tag_, &send_param);
    return check_request_ready(request);
  }
};

class rma_put_awaitable : public send_awaitable<rma_put_awaitable> {
  ucp_ep_h ep_;
  void const *buffer_;
  size_t length_;
  uint64_t remote_addr_;
  ucp_rkey_h rkey_;
  friend class send_awaitable;

 public:
  rma_put_awaitable(ucp_ep_h ep, void const *buffer, size_t length,
                    uint64_t remote_addr, ucp_rkey_h rkey)
      : ep_(ep),
        buffer_(buffer),
        length_(length),
        remote_addr_(remote_addr),
        rkey_(rkey) {}

  bool await_ready() noexcept {
    auto send_param = build_param();
    auto request =
        ::ucp_put_nbx(ep_, buffer_, length_, remote_addr_, rkey_, &send_param);
    return check_request_ready(request);
  }
};

class rma_get_awaitable : public send_awaitable<rma_get_awaitable> {
  ucp_ep_h ep_;
  void *buffer_;
  size_t length_;
  uint64_t remote_addr_;
  ucp_rkey_h rkey_;
  friend class send_awaitable;

 public:
  rma_get_awaitable(ucp_ep_h ep, void *buffer, size_t length,
                    uint64_t remote_addr, ucp_rkey_h rkey)
      : ep_(ep),
        buffer_(buffer),
        length_(length),
        remote_addr_(remote_addr),
        rkey_(rkey) {}

  bool await_ready() noexcept {
    auto send_param = build_param();
    auto request =
        ::ucp_get_nbx(ep_, buffer_, length_, remote_addr_, rkey_, &send_param);
    return check_request_ready(request);
  }
};

template <class T>
class rma_atomic_awaitable : public send_awaitable<rma_atomic_awaitable<T>> {
  static_assert(sizeof(T) == 4 || sizeof(T) == 8,
                "Only 4-byte and 8-byte "
                "integers are supported");
  ucp_ep_h ep_;
  ucp_atomic_op_t const op_;
  void const *buffer_;
  uint64_t remote_addr_;
  ucp_rkey_h rkey_;
  void *reply_buffer_;
  friend class send_awaitable<rma_atomic_awaitable<T>>;

 public:
  rma_atomic_awaitable(ucp_ep_h ep, ucp_atomic_op_t const op,
                       void const *buffer, uint64_t remote_addr,
                       ucp_rkey_h rkey, void *reply_buffer = nullptr)
      : ep_(ep),
        op_(op),
        buffer_(buffer),
        remote_addr_(remote_addr),
        rkey_(rkey),
        reply_buffer_(reply_buffer) {
    if (op == UCP_ATOMIC_OP_SWAP || op == UCP_ATOMIC_OP_CSWAP) {
      assert(reply_buffer != nullptr);
    }
  }

  bool await_ready() noexcept {
    auto send_param = this->build_param();
    send_param.op_attr_mask |= UCP_OP_ATTR_FIELD_DATATYPE;
    send_param.datatype = ucp_dt_make_contig(sizeof(T));
    if (reply_buffer_ != nullptr) {
      send_param.op_attr_mask |= UCP_OP_ATTR_FIELD_REPLY_BUFFER;
      send_param.reply_buffer = reply_buffer_;
    }
    auto request = ::ucp_atomic_op_nbx(ep_, op_, buffer_, 1, remote_addr_,
                                       rkey_, &send_param);
    return this->check_request_ready(request);
  }
};

class worker;
using worker_ptr = worker *;

class worker_flush_awaitable : public send_awaitable<worker_flush_awaitable> {
  worker_ptr worker_;
  friend class send_awaitable;

 public:
  worker_flush_awaitable(worker_ptr worker);

  bool await_ready() noexcept;
};

/* Common awaitable class for stream-recv-like callbacks */
class stream_recv_awaitable : base_awaitable {
 private:
  ucp_ep_h ep_;
  ucp_worker_h worker_;
  size_t received_;
  void *buffer_;
  size_t length_;
  void *request_;

 public:
  stream_recv_awaitable(ucp_ep_h ep, void *buffer, size_t length)
      : ep_(ep), received_(0), buffer_(buffer), length_(length) {}

  stream_recv_awaitable(ucp_ep_h ep, ucp_worker_h worker, void *buffer,
                        size_t length, stream_recv_awaitable *&cancel)
      : ep_(ep),
        worker_(worker),
        received_(0),
        buffer_(buffer),
        length_(length) {
    cancel = this;
  }

  static void stream_recv_cb(void *request, ucs_status_t status,
                             size_t received, void *user_data) {
    auto self = reinterpret_cast<stream_recv_awaitable *>(user_data);
    self->status_ = status;
    self->received_ = received;
    ::ucp_request_free(request);
    self->h_.resume();
  }

  bool await_ready() noexcept {
    ucp_request_param_t stream_recv_param;
    stream_recv_param.op_attr_mask =
        UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA;
    stream_recv_param.cb.recv_stream = &stream_recv_cb;
    stream_recv_param.user_data = this;
    auto request = ::ucp_stream_recv_nbx(ep_, buffer_, length_, &received_,
                                         &stream_recv_param);

    if (!check_request_ready(request)) {
      request_ = request;
      return false;
    }

    return true;
  }

  bool await_suspend(std::coroutine_handle<> h) {
    h_ = h;
    return status_ == UCS_INPROGRESS;
  }

  size_t await_resume() const {
    check_ucs_status(status_, "operation failed");
    return received_;
  }

  void cancel() {
    if (request_ != nullptr) {
      ::ucp_request_cancel(worker_, request_);
    }
  }
};

/* Common awaitable class for tag-recv-like callbacks */
class tag_recv_awaitable : public base_awaitable {
 private:
  ucp_worker_h worker_;
  void *request_;
  void *buffer_;
  size_t length_;
  ucp_tag_t tag_;
  ucp_tag_t tag_mask_;
  ucp_tag_recv_info_t recv_info_;

 public:
  tag_recv_awaitable(ucp_worker_h worker, void *buffer, size_t length,
                     ucp_tag_t tag, ucp_tag_t tag_mask)
      : worker_(worker),
        request_(nullptr),
        buffer_(buffer),
        length_(length),
        tag_(tag),
        tag_mask_(tag_mask) {}

  tag_recv_awaitable(ucp_worker_h worker, void *buffer, size_t length,
                     ucp_tag_t tag, ucp_tag_t tag_mask,
                     tag_recv_awaitable *&cancel)
      : tag_recv_awaitable(worker, buffer, length, tag, tag_mask) {
    cancel = this;
  }

  static void tag_recv_cb(void *request, ucs_status_t status,
                          ucp_tag_recv_info_t const *tag_info,
                          void *user_data) {
    auto self = reinterpret_cast<tag_recv_awaitable *>(user_data);
    self->status_ = status;
    self->recv_info_.length = tag_info->length;
    self->recv_info_.sender_tag = tag_info->sender_tag;
    ::ucp_request_free(request);
    self->h_.resume();
  }

  bool await_ready() noexcept {
    ucp_request_param_t tag_recv_param;
    tag_recv_param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                                  UCP_OP_ATTR_FIELD_USER_DATA |
                                  UCP_OP_ATTR_FIELD_RECV_INFO;
    tag_recv_param.cb.recv = &tag_recv_cb;
    tag_recv_param.user_data = this;
    tag_recv_param.recv_info.tag_info = &recv_info_;

    auto request = ::ucp_tag_recv_nbx(worker_, buffer_, length_, tag_,
                                      tag_mask_, &tag_recv_param);

    if (!check_request_ready(request)) {
      request_ = request;
      return false;
    }
    return true;
  }

  bool await_suspend(std::coroutine_handle<> h) {
    h_ = h;
    return status_ == UCS_INPROGRESS;
  }

  std::pair<size_t, ucp_tag_t> await_resume() {
    request_ = nullptr;
    check_ucs_status(status_, "error in ucp_tag_recv_nbx");
    return std::make_pair(recv_info_.length, recv_info_.sender_tag);
  }

  void cancel() {
    if (request_) {
      ::ucp_request_cancel(worker_, request_);
      request_ = nullptr;
    }
  }
};

}  // namespace ucxpp