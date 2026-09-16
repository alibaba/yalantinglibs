#pragma once

#include <sys/eventfd.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <cerrno>
#include <mutex>
#include <utility>

#include "own_ring_types.hpp"

namespace coro_io::detail {

class OwnRingRequestQueue {
 public:
  explicit OwnRingRequestQueue(size_t capacity) : capacity_(capacity) {}
  OwnRingRequestQueue(const OwnRingRequestQueue &) = delete;
  OwnRingRequestQueue &operator=(const OwnRingRequestQueue &) = delete;
  ~OwnRingRequestQueue() {
    assert(outstanding() == 0);
    if (wake_fd_ >= 0) {
      ::close(wake_fd_);
    }
  }

  int initialize() noexcept {
    wake_fd_ = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    return wake_fd_ < 0 ? -errno : 0;
  }

  template <typename TerminalError>
  int push(OwnRingReadRequest &request, bool owner,
           TerminalError terminal_error) noexcept {
    if (owner) {
      if (int error = terminal_error()) {
        return error;
      }
      if (!reserve()) {
        return -EAGAIN;
      }
      append(pending_head_, pending_tail_, &request);
      return 0;
    }
    bool notify;
    {
      std::lock_guard lock(mutex_);
      if (int error = terminal_error()) {
        return error;
      }
      if (!reserve()) {
        return -EAGAIN;
      }
      append(incoming_head_, incoming_tail_, &request);
      notify = std::exchange(sleeping_, false);
    }
    if (notify) {
      wake();
    }
    return 0;
  }

  void stop() noexcept {
    bool notify;
    {
      std::lock_guard lock(mutex_);
      stopped_.store(true, std::memory_order_release);
      notify = std::exchange(sleeping_, false);
    }
    if (notify) {
      wake();
    }
  }

  bool stopped() const noexcept {
    return stopped_.load(std::memory_order_acquire);
  }
  uint64_t outstanding() const noexcept {
    return outstanding_.load(std::memory_order_acquire);
  }
  uint64_t accepted() const noexcept { return accepted_.load(); }
  uint64_t wake_writes() const noexcept { return wake_writes_.load(); }
  int wake_fd() const noexcept { return wake_fd_; }

  void finish() noexcept {
    outstanding_.fetch_sub(1, std::memory_order_release);
  }

  OwnRingReadRequest *front() const noexcept { return pending_head_; }

  OwnRingReadRequest *pop() noexcept {
    auto *request = pending_head_;
    if (request) {
      pending_head_ = request->next_;
      if (!pending_head_) {
        pending_tail_ = nullptr;
      }
    }
    return request;
  }

  void retry(OwnRingReadRequest &request) noexcept {
    append(pending_head_, pending_tail_, &request);
  }

  void collect() noexcept {
    std::lock_guard lock(mutex_);
    sleeping_ = false;
    if (incoming_head_) {
      if (pending_tail_) {
        pending_tail_->next_ = incoming_head_;
      }
      else {
        pending_head_ = incoming_head_;
      }
      pending_tail_ = incoming_tail_;
      incoming_head_ = incoming_tail_ = nullptr;
    }
  }

  bool prepare_wait(size_t inflight) noexcept {
    std::lock_guard lock(mutex_);
    sleeping_ = incoming_head_ == nullptr && (!stopped() || inflight > 0);
    return sleeping_;
  }

  void consume_wake() noexcept {
    uint64_t value;
    while (::read(wake_fd_, &value, sizeof(value)) > 0) {
    }
  }

 private:
  bool reserve() noexcept {
    auto count = outstanding_.load(std::memory_order_relaxed);
    while (count < capacity_) {
      if (outstanding_.compare_exchange_weak(count, count + 1,
                                             std::memory_order_relaxed)) {
        accepted_.fetch_add(1, std::memory_order_relaxed);
        return true;
      }
    }
    return false;
  }

  static void append(OwnRingReadRequest *&head, OwnRingReadRequest *&tail,
                     OwnRingReadRequest *request) noexcept {
    request->next_ = nullptr;
    if (tail) {
      tail->next_ = request;
    }
    else {
      head = request;
    }
    tail = request;
  }

  void wake() noexcept {
    uint64_t value = 1;
    ssize_t result;
    do {
      result = ::write(wake_fd_, &value, sizeof(value));
    } while (result < 0 && errno == EINTR);
    wake_writes_.fetch_add(1, std::memory_order_relaxed);
  }

  size_t capacity_;
  int wake_fd_ = -1;
  OwnRingReadRequest *incoming_head_ = nullptr;
  OwnRingReadRequest *incoming_tail_ = nullptr;
  OwnRingReadRequest *pending_head_ = nullptr;
  OwnRingReadRequest *pending_tail_ = nullptr;
  std::mutex mutex_;
  bool sleeping_ = false;
  std::atomic<bool> stopped_{false};
  std::atomic<uint64_t> outstanding_{0};
  std::atomic<uint64_t> accepted_{0};
  std::atomic<uint64_t> wake_writes_{0};
};

}  // namespace coro_io::detail
