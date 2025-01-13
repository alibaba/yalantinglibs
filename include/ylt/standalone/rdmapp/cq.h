#pragma once

#include <infiniband/verbs.h>

#include <array>
#include <memory>
#include <vector>

#include "device.h"
#include "error.h"
#include "fcntl.h"

namespace rdmapp {

class qp;
class cq;
typedef cq *cq_ptr;

class comp_channel {
  struct ibv_comp_channel *comp_channel_;

 public:
  comp_channel(device_ptr device) {
    comp_channel_ = ::ibv_create_comp_channel(device->ctx());
    check_ptr(comp_channel_, "failed to create comp channel");
  }

  void set_non_blocking() {
    int flags = ::fcntl(comp_channel_->fd, F_GETFL);
    if (flags < 0) {
      check_errno(errno, "failed to get flags");
    }
    int ret = ::fcntl(comp_channel_->fd, F_SETFL, flags | O_NONBLOCK);
    if (ret < 0) {
      check_errno(errno, "failed to set flags");
    }
  }

  cq_ptr get_event() {
    struct ibv_cq *cq;
    void *ev_ctx;
    check_rc(::ibv_get_cq_event(comp_channel_, &cq, &ev_ctx),
             "failed to get event");
    auto cq_obj_ptr = reinterpret_cast<cq_ptr>(ev_ctx);
    return cq_obj_ptr;
  }

  int fd() const { return comp_channel_->fd; }

  ~comp_channel() {
    if (comp_channel_ == nullptr) [[unlikely]] {
      return;
    }
    if (auto rc = ::ibv_destroy_comp_channel(comp_channel_); rc != 0) {
    }
    else {
    }
  }

  struct ibv_comp_channel *channel() const { return comp_channel_; }
};

typedef comp_channel *comp_channel_ptr;

/**
 * @brief This class is an abstraction of a Completion Queue.
 *
 */
class cq {
  device_ptr device_;
  struct ibv_cq *cq_;
  friend class qp;

 public:
  cq(cq const &) = delete;
  cq &operator=(cq const &) = delete;

  /**
   * @brief Construct a new cq object.
   *
   * @param device The device to use.
   * @param num_cqe The number of completion entries to allocate.
   * @param channel If not null, assign this cq to the completion channel
   */
  cq(device_ptr device, size_t nr_cqe = 128, comp_channel_ptr channel = nullptr)
      : device_(device) {
    cq_ = ::ibv_create_cq(device->ctx_, nr_cqe, this,
                          channel ? channel->channel() : nullptr, 0);
    check_ptr(cq_, "failed to create cq");
  }

  void request_notify() {
    check_rc(::ibv_req_notify_cq(cq_, 0), "failed to request notify");
  }

  void ack_event(unsigned int nr_events = 1) {
    ::ibv_ack_cq_events(cq_, nr_events);
  }

  /**
   * @brief Poll the completion queue.
   *
   * @param wc If any, this will be filled with a completion entry.
   * @return true If there is a completion entry.
   * @return false If there is no completion entry.
   * @exception std::runtime_exception Error occured while polling the
   * completion queue.
   */
  bool poll(struct ibv_wc &wc) {
    if (auto rc = ::ibv_poll_cq(cq_, 1, &wc); rc < 0) [[unlikely]] {
      check_rc(-rc, "failed to poll cq");
    }
    else if (rc == 0) {
      return false;
    }
    else {
      return true;
    }
    return false;
  }

  /**
   * @brief Poll the completion queue.
   *
   * @param wc_vec If any, this will be filled with completion entries up to the
   * size of the vector.
   * @return size_t The number of completion entries. 0 means no completion
   * entry.
   * @exception std::runtime_exception Error occured while polling the
   * completion queue.
   */
  size_t poll(std::vector<struct ibv_wc> &wc_vec) {
    return poll(&wc_vec[0], wc_vec.size());
  }

  template <class It>
  size_t poll(It wc, int count) {
    int rc = ::ibv_poll_cq(cq_, count, wc);
    if (rc < 0) {
      throw_with("failed to poll cq: %s (rc=%d)", strerror(rc), rc);
    }
    return rc;
  }

  template <int N>
  size_t poll(std::array<struct ibv_wc, N> &wc_array) {
    return poll(&wc_array[0], N);
  }

  ~cq() {
    if (cq_ == nullptr) [[unlikely]] {
      return;
    }

    if (auto rc = ::ibv_destroy_cq(cq_); rc != 0) [[unlikely]] {
    }
    else {
    }
  }
};

}  // namespace rdmapp