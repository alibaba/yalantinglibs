#pragma once

#include <ucp/api/ucp.h>
#include <ucs/type/status.h>

#include <utility>
#include <vector>

#include "ucxpp/awaitable.h"
#include "ucxpp/context.h"
#include "ucxpp/detail/serdes.h"

namespace ucxpp {

/**
 * @brief Abstraction for a UCX worker.
 *
 */
class worker {
  friend class local_address;
  friend class endpoint;
  ucp_worker_h worker_;
  context_ptr ctx_;
  int event_fd_;

 public:
  using worker_ptr = worker *;

  /**
   * @brief Construct a new worker object
   *
   * @param ctx UCX context
   */
  worker(context_ptr ctx) : ctx_(ctx), event_fd_(-1) {
    ucp_worker_params_t worker_params;
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    check_ucs_status(
        ::ucp_worker_create(ctx->context_, &worker_params, &worker_),
        "failed to create ucp worker");
    if (ctx_->features() & UCP_FEATURE_WAKEUP) {
      check_ucs_status(::ucp_worker_get_efd(worker_, &event_fd_),
                       "failed to get ucp worker event fd");
    }
  }

  /**
   * @brief Get the event fd for the worker. The wakeup feature must be enabled
   * for this to work.
   *
   * @return int
   */
  int event_fd() const {
    assert(event_fd_ != -1);
    return event_fd_;
  }

  /**
   * @brief Get the worker's context object
   *
   * @return context_ptr The worker's context object
   */
  context_ptr context() const { return ctx_; }

  /**
   * @brief Represents a local UCX address.
   *
   */
  class local_address {
    worker_ptr worker_;
    ucp_address_t *address_;
    size_t address_length_;
    friend class endpoint;

   public:
    /**
     * @brief Construct a new local address object
     *
     * @param worker UCX worker
     * @param address UCP address
     * @param address_length UCP address length
     */
    local_address(worker_ptr worker, ucp_address_t *address,
                  size_t address_length)
        : worker_(worker), address_(address), address_length_(address_length) {}

    /**
     * @brief Construct a new local address object
     *
     * @param other Another local address object to move from
     */
    local_address(local_address &&other)
        : worker_(std::move(other.worker_)),
          address_(std::exchange(other.address_, nullptr)),
          address_length_(other.address_length_) {}

    /**
     * @brief Move assignment operator
     *
     * @param other Another local address object to move from
     * @return local_address& This object
     */
    local_address &operator=(local_address &&other) {
      worker_ = std::move(other.worker_);
      address_ = std::exchange(other.address_, nullptr);
      address_length_ = other.address_length_;
      return *this;
    }

    /**
     * @brief Serialize the address to a buffer ready to be sent to a remote
     * peer
     *
     * @return std::vector<char> The serialized address
     */
    std::vector<char> serialize() const {
      std::vector<char> buffer;
      auto it = std::back_inserter(buffer);
      detail::serialize(address_length_, it);
      std::copy_n(reinterpret_cast<char const *>(address_), address_length_,
                  it);
      return buffer;
    }

    /**
     * @brief Get the UCP address
     *
     * @return const ucp_address_t* The UCP address
     */
    const ucp_address_t *get_address() const { return address_; }

    /**
     * @brief Get the length of the address
     *
     * @return size_t The address length
     */
    size_t get_length() const { return address_length_; }

    /**
     * @brief Destroy the local address object and release the buffer
     *
     */
    ~local_address() {
      if (address_ == nullptr) [[unlikely]] {
        return;
      }
      ::ucp_worker_release_address(worker_->worker_, address_);
    }
  };

  /**
   * @brief Get the worker's UCX address
   *
   * @return local_address The worker's UCX address
   */
  local_address get_address() const {
    ucp_address_t *address;
    size_t address_length;
    check_ucs_status(
        ::ucp_worker_get_address(worker_, &address, &address_length),
        "failed to get address");
    return local_address(const_cast<worker_ptr>(this), address, address_length);
  }

  std::vector<char> get_remote_address() const {
    ucp_address_t *address;
    size_t address_length;
    check_ucs_status(
        ::ucp_worker_get_address(worker_, &address, &address_length),
        "failed to get address");
    std::vector<char> buffer(address_length);
    std::copy_n(reinterpret_cast<char const *>(address), address_length,
                buffer.begin());
    return buffer;
  }

  /**
   * @brief Get the worker's native UCX handle
   *
   * @return ucp_worker_h The worker's native UCX handle
   */
  ucp_worker_h handle() const { return worker_; }

  /**
   * @brief Progress the worker
   *
   * @return true If progress was made
   * @return false If no progress was made
   */
  bool progress() const { return ::ucp_worker_progress(worker_); }

  /**
   * @brief Wait for an event on the worker. It should be called only after a
   * call to progress() returns false.
   *
   */
  void wait() const {
    check_ucs_status(::ucp_worker_wait(worker_), "failed to wait worker");
  }

  /**
   * @brief Arm the worker for next event notification.
   *
   * @return true If the worker was armed
   * @return false If the worker has pending events. In this case, the user must
   * call progress() until it returns false.
   */
  bool arm() const {
    auto status = ::ucp_worker_arm(worker_);
    if (status == UCS_ERR_BUSY) {
      return false;
    }
    check_ucs_status(status, "failed to arm worker");
    return true;
  }

  /**
   * @brief Tag receive to the buffer
   *
   * @param buffer The buffer to receive to
   * @param length The length of the buffer
   * @param tag The tag to receive with
   * @param tag_mask The bit mask for tag matching, 0 means accepting any tag
   * @return tag_recv_awaitable A coroutine that returns a pair of number of
   * bytes received and the sender tag upon completion
   */
  tag_recv_awaitable tag_recv(void *buffer, size_t length, ucp_tag_t tag,
                              ucp_tag_t tag_mask = 0xFFFFFFFFFFFFFFFF) const {
    return tag_recv_awaitable(worker_, buffer, length, tag, tag_mask);
  }

  /**
   * @brief Fence the worker. Operations issued on the worker before the fence
   * are ensured to complete before operations issued after the fence.
   *
   */
  void fence() {
    check_ucs_status(::ucp_worker_fence(worker_), "failed to fence worker");
  }

  /**
   * @brief Flush the worker
   *
   * @return worker_flush_awaitable A coroutine that returns when the worker is
   * flushed
   */
  worker_flush_awaitable flush() { return worker_flush_awaitable(this); }

  ~worker() { ::ucp_worker_destroy(worker_); }
};

}  // namespace ucxpp