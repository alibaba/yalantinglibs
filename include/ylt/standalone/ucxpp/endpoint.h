#pragma once

#include <ucp/api/ucp.h>
#include <ucs/type/status.h>

#include "ucxpp/address.h"
#include "ucxpp/awaitable.h"
#include "ucxpp/error.h"
#include "ucxpp/memory.h"
#include "ucxpp/worker.h"

namespace ucxpp {

/**
 * @brief Abstraction for a UCX endpoint.
 *
 */
class endpoint {
  friend class worker;
  friend class local_memory_handle;
  friend class remote_memory_handle;
  friend class ep_close_awaitable;
  worker_ptr worker_;
  ucp_ep_h ep_;
  void *close_request_;
  remote_address peer_;

 public:
  endpoint(endpoint const &) = delete;
  endpoint &operator=(endpoint const &) = delete;

  /**
   * @brief Construct a new endpoint object
   *
   * @param worker UCX worker
   * @param peer Remote UCX address
   */
  endpoint(worker_ptr worker, remote_address const &peer)
      : worker_(worker), peer_(peer) {
    ucp_ep_params_t ep_params;
    ep_params.field_mask =
        UCP_EP_PARAM_FIELD_REMOTE_ADDRESS | UCP_EP_PARAM_FIELD_ERR_HANDLER;
    ep_params.address = peer.get_address();
    ep_params.err_handler.cb = &error_cb;
    ep_params.err_handler.arg = this;
    check_ucs_status(::ucp_ep_create(worker_->worker_, &ep_params, &ep_),
                     "failed to create ep");
  }

  /**
   * @brief Error handler for all endpoints
   *
   * @param ep endpoint object
   * @param ep_h UCX endpoint handle
   * @param status error status
   */
  static void error_cb(void *ep, ucp_ep_h ep_h, ucs_status_t status) {
    auto ep_ptr = reinterpret_cast<endpoint *>(ep);
    if (!ep_ptr->close_request_) {
      auto request = ::ucp_ep_close_nb(ep_h, UCP_EP_CLOSE_MODE_FLUSH);
      if (UCS_PTR_IS_ERR(request)) {
        ep_ptr->close_request_ = nullptr;
        ep_ptr->ep_ = nullptr;
      }
      else if (UCS_PTR_IS_PTR(request)) {
        ep_ptr->close_request_ = request;
      }
      else {
        ep_ptr->close_request_ = nullptr;
        ep_ptr->ep_ = nullptr;
      }
    }
  }

  /**
   * @brief Get the worker object
   *
   * @return std::shared_ptr<worker> The endpoint's worker
   */
  worker_ptr worker() const { return worker_; }

  /**
   * @brief Print the endpoint's information
   *
   */
  void print() const { ::ucp_ep_print_info(ep_, stdout); }

  /**
   * @brief Get the endpoint's native UCX handle
   *
   * @return ucp_ep_h The endpoint's native UCX handle
   */
  ucp_ep_h handle() const { return ep_; }

  /**
   * @brief Get the endpoint's remote address
   *
   * @return remote_address The endpoint's remote address
   */
  const remote_address &get_address() const { return peer_; }

  /**
   * @brief Stream send the buffer
   *
   * @param buffer The buffer to send
   * @param length The length of the buffer
   * @return stream_send_awaitable A coroutine that returns upon completion
   */
  stream_send_awaitable stream_send(void const *buffer, size_t length) const {
    return stream_send_awaitable(ep_, buffer, length);
  }

  /**
   * @brief Stream receive to the buffer
   *
   * @param buffer The buffer to receive to
   * @param length The length of the buffer
   * @return stream_recv_awaitable A coroutine that returns number of bytes
   * received upon completion
   */
  stream_recv_awaitable stream_recv(void *buffer, size_t length) const {
    return stream_recv_awaitable(ep_, buffer, length);
  }

  /**
   * @brief Tag send the buffer
   *
   * @param buffer The buffer to send
   * @param length The length of the buffer
   * @param tag The tag to send with
   * @return tag_send_awaitable A coroutine that returns upon completion
   */
  tag_send_awaitable tag_send(void const *buffer, size_t length,
                              ucp_tag_t tag) const {
    return tag_send_awaitable(ep_, buffer, length, tag);
  }

  class ep_flush_awaitable : public ucxpp::send_awaitable<ep_flush_awaitable> {
    endpoint *endpoint_;
    friend class send_awaitable;

   public:
    ep_flush_awaitable(endpoint *endpoint) : endpoint_(endpoint) {}
    bool await_ready() noexcept {
      auto send_param = build_param();
      auto request = ::ucp_ep_flush_nbx(endpoint_->handle(), &send_param);
      return check_request_ready(request);
    }
  };

  class ep_close_awaitable : public send_awaitable<ep_close_awaitable> {
    endpoint *endpoint_;
    friend class send_awaitable;

   public:
    ep_close_awaitable(endpoint *endpoint) : endpoint_(endpoint) {}

    bool await_ready() noexcept {
      auto send_param = build_param();
      auto request = ::ucp_ep_close_nbx(endpoint_->handle(), &send_param);
      if (check_request_ready(request)) {
        return true;
      }
      endpoint_->close_request_ = request;
      return false;
    }

    void await_resume() const {
      check_ucs_status(status_, "operation failed");
      endpoint_->ep_ = nullptr;
    }
  };

  /**
   * @brief Flush the endpoint
   *
   * @return ep_flush_awaitable A coroutine that returns upon completion
   */
  ep_flush_awaitable flush() const {
    return ep_flush_awaitable(const_cast<endpoint *>(this));
  }

  /**
   * @brief Close the endpoint. You should not use the endpoint after calling
   * this function.
   *
   * @return task<void> A coroutine that returns upon completion
   */
  ep_close_awaitable close() { return ep_close_awaitable(this); }

  /**
   * @brief Endpoint close callback
   *
   * @param request UCX request handle
   * @param status UCX status
   * @param user_data User data
   */
  static void close_cb(void *request, ucs_status_t status, void *user_data) {
    (void)status;
    (void)user_data;
    ::ucp_request_free(request);
  }

  /**
   * @brief Destroy the endpoint object. If the endpoint is not closed yet, it
   * will be closed.
   *
   */
  ~endpoint() {
    if (ep_ != nullptr && close_request_ == nullptr) {
      ucp_request_param_t param;
      param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK;
      param.cb.send = &close_cb;
      ::ucp_ep_close_nbx(ep_, &param);
    }
  }
};

using endpoint_ptr = endpoint *;

/**
 * @brief Represents a remote memory region. Note that this does not contain the
 * remote address. It should be kept by the user.
 *
 */
class remote_memory_handle {
  endpoint_ptr endpoint_;
  ucp_ep_h ep_;
  ucp_rkey_h rkey_;

 public:
  remote_memory_handle(remote_memory_handle const &) = delete;
  remote_memory_handle &operator=(remote_memory_handle const &) = delete;

  /**
   * @brief Construct a new remote memory handle object. All subsequent remote
   * memory access will happen on the given endpoint.
   *
   * @param endpoint UCX endpoint
   * @param packed_rkey_buffer Packed remote key buffer received from remote
   * peer
   */
  remote_memory_handle(endpoint_ptr endpoint, void const *packed_rkey_buffer)
      : endpoint_(endpoint), ep_(endpoint->handle()) {
    check_ucs_status(
        ::ucp_ep_rkey_unpack(endpoint_->handle(), packed_rkey_buffer, &rkey_),
        "failed to unpack memory");
  }
  /**
   * @brief Construct a new remote memory handle object
   *
   * @param other Another remote memory handle to move from
   */
  remote_memory_handle(remote_memory_handle &&other)
      : endpoint_(std::move(other.endpoint_)),
        ep_(std::exchange(other.ep_, nullptr)),
        rkey_(std::exchange(other.rkey_, nullptr)) {}

  /**
   * @brief Write to the remote memory region
   *
   * @param buffer Local buffer to write from
   * @param length Length of the buffer
   * @param remote_addr Remote address to write to
   * @return rma_put_awaitable A coroutine that returns upon completion
   */
  rma_put_awaitable put(void const *buffer, size_t length,
                        uint64_t remote_addr) const {
    return rma_put_awaitable(ep_, buffer, length, remote_addr, rkey_);
  }

  /**
   * @brief Read from the remote memory region
   *
   * @param buffer Local buffer to read into
   * @param length Length of the buffer
   * @param remote_addr Remote address to read from
   * @return rma_get_awaitable A coroutine that returns upon completion
   */
  rma_get_awaitable get(void *buffer, size_t length,
                        uint64_t remote_addr) const {
    return rma_get_awaitable(ep_, buffer, length, remote_addr, rkey_);
  }

  /**
   * \copydoc endpoint::put
   *
   */
  rma_put_awaitable write(void const *buffer, size_t length,
                          uint64_t remote_addr) const {
    return rma_put_awaitable(ep_, buffer, length, remote_addr, rkey_);
  }

  /**
   * \copydoc endpoint::get
   *
   */
  rma_get_awaitable read(void *buffer, size_t length,
                         uint64_t remote_addr) const {
    return rma_get_awaitable(ep_, buffer, length, remote_addr, rkey_);
  }

  /**
   * @brief Get the memory region's endpoint object
   *
   * @return endpoint_ptr The memory region's endpoint object
   */
  endpoint_ptr endpoint() const { return endpoint_; }

  /**
   * @brief Get the native UCX rkey handle
   *
   * @return ucp_rkey_h The native UCX rkey handle
   */
  ucp_rkey_h handle() const { return rkey_; }

  /**
   * @brief Atomically fetch and add a value to the remote memory region
   *
   * @tparam T The type of the value to add, should be of 4 bytes or 8 bytes
   * long
   * @param remote_addr The remote address to add to
   * @param delta The value to add
   * @param old_value A reference to a variable to store the old value
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   * The old value is placed in old_value
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_fetch_add(uint64_t remote_addr, T const &delta,
                                           T &old_value) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_ADD, &delta, remote_addr,
                                   rkey_, &old_value);
  }

  /**
   * @brief Atomically fetch and AND a value to the remote memory region
   *
   * @tparam T The type of the value to AND, should be of 4 bytes or 8 bytes
   * long
   * @param remote_addr The remote address to AND to
   * @param delta The other operand of the AND operation
   * @param old_value A reference to a variable to store the old value
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   * The old value is placed in old_value
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_fetch_and(uint64_t remote_addr, T const &bits,
                                           T &old_value) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_AND, &bits, remote_addr,
                                   rkey_, &old_value);
  }

  /**
   * @brief Atomically fetch and OR a value to the remote memory region
   *
   * @tparam T The type of the value to OR, should be of 4 bytes or 8 bytes
   * long
   * @param remote_addr The remote address to OR to
   * @param delta The other operand of the OR operation
   * @param old_value A reference to a variable to store the old value
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   * The old value is placed in old_value
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_fetch_or(uint64_t remote_addr, T const &bits,
                                          T &old_value) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_OR, &bits, remote_addr,
                                   rkey_, &old_value);
  }

  /**
   * @brief Atomically fetch and XOR a value to the remote memory region
   *
   * @tparam T The type of the value to XOR, should be of 4 bytes or 8 bytes
   * long
   * @param remote_addr The remote address to XOR to
   * @param delta The other operand of the XOR operation
   * @param old_value A reference to a variable to store the old value
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   * The old value is placed in old_value
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_fetch_xor(uint64_t remote_addr, T const &bits,
                                           T &old_value) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_XOR, &bits, remote_addr,
                                   rkey_, &old_value);
  }

  /**
   * @brief Atomically add to a value in the remote memory region
   *
   * @tparam T The type of the value to add, should be of 4 bytes or 8 bytes
   * long
   * @param remote_addr The remote address to add to
   * @param delta The value to add
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_add(uint64_t remote_addr,
                                     T const &delta) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_ADD, &delta, remote_addr,
                                   rkey_);
  }

  /**
   * @brief Atomically AND a value in the remote memory region
   *
   * @tparam T The type of the value to AND, should be of 4 bytes or 8 bytes
   * long
   * @param remote_addr The remote address to AND to
   * @param delta The other operand of the AND operation
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_and(uint64_t remote_addr,
                                     T const &bits) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_AND, &bits, remote_addr,
                                   rkey_);
  }

  /**
   * @brief Atomically OR to a value in the remote memory region
   *
   * @tparam T The type of the value to OR, should be of 4 bytes or 8 bytes long
   * @param remote_addr The remote address to OR to
   * @param delta The other operand of the OR operation
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_or(uint64_t remote_addr, T const &bits) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_OR, &bits, remote_addr,
                                   rkey_);
  }

  /**
   * @brief Atomically XOR a value to the remote memory region
   *
   * @tparam T The type of the value to XOR, should be of 4 bytes or 8 bytes
   * long
   * @param remote_addr The remote address to XOR to
   * @param delta The other operand of the XOR operation
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   * The old value is placed in old_value
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_xor(uint64_t remote_addr,
                                     T const &bits) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_XOR, &bits, remote_addr,
                                   rkey_);
  }

  /**
   * @brief Atomically swap a value in the remote memory region
   *
   * @tparam T The type of the value to swap, should be of 4 bytes or 8 bytes
   * long
   * @param remote_addr The remote address to swap
   * @param new_value The new value to swap in
   * @param old_value A reference to a variable to store the old value
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   * The old value is placed in old_value
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_swap(uint64_t remote_addr, T const &new_value,
                                      T &old_value) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_SWAP, &new_value,
                                   remote_addr, rkey_, &old_value);
  }

  /**
   * @brief Atomically compare and swap a value in the remote memory region
   *
   * @tparam T The type of the value to swap, should be of 4 bytes or 8 bytes
   * long
   * @param raddr The remote address to swap
   * @param expected The expected value to compare against
   * @param desired_and_old A reference to a variable to store the desired new
   * value and the old value swapped out
   * @return rma_atomic_awaitable<T> A coroutine that returns upon completion.
   * The old value is placed in desired_and_old
   */
  template <class T>
  rma_atomic_awaitable<T> atomic_compare_swap(uint64_t raddr, T const &expected,
                                              T &desired_and_old) const {
    return rma_atomic_awaitable<T>(ep_, UCP_ATOMIC_OP_CSWAP, &expected, raddr,
                                   rkey_, &desired_and_old);
  }

  /**
   * @brief Destroy the remote memory handle object and the associated rkey
   * handle
   *
   */
  ~remote_memory_handle() {
    if (rkey_ != nullptr) {
      ::ucp_rkey_destroy(rkey_);
    }
  }
};

}  // namespace ucxpp
