#pragma once

#include <liburing.h>
#include <poll.h>
#include <sched.h>
#include <sys/uio.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "own_ring_request_queue.hpp"
#include "own_ring_types.hpp"

namespace coro_io::detail {

class OwnRingDriver {
 public:
  using Mode = OwnRingMode;
  using Options = OwnRingOptions;
  using ReadRequest = OwnRingReadRequest;
  using Statistics = OwnRingStatistics;

#ifdef OWN_RING_FAULT_INJECT
  struct FaultHooks {
    std::function<int(int, const ReadRequest &)> map_res;
    std::function<int(io_uring *, bool)> submit;
    std::function<void()> before_submit;
    bool force_init_fail = false;
    bool reject_optimized_setup = false;
  };

  static FaultHooks &fault() {
    static FaultHooks hooks;
    return hooks;
  }
#endif

  explicit OwnRingDriver(unsigned entries = 4096)
      : OwnRingDriver(environment_options(entries)) {}

  explicit OwnRingDriver(Options options)
      : options_(options), requests_(options.max_requests) {
    options_.submit_batch = std::max(1u, options_.submit_batch);
    if (options_.entries < 2 || options_.max_requests == 0) {
      init_error_ = -EINVAL;
      return;
    }
    if (int result = requests_.initialize()) {
      init_error_ = result;
      return;
    }
    thread_ = std::thread([this] {
      run();
    });
    std::unique_lock lock(mutex_);
    ready_.wait(lock, [this] {
      return initialized_;
    });
  }

  OwnRingDriver(const OwnRingDriver &) = delete;
  OwnRingDriver &operator=(const OwnRingDriver &) = delete;

  ~OwnRingDriver() {
    if (thread_.joinable() && thread_.get_id() == std::this_thread::get_id()) {
      std::terminate();
    }
    stop();
  }

  bool ok() const noexcept { return init_error_ == 0; }
  int init_error() const noexcept { return init_error_; }
  unsigned setup_flags() const noexcept { return setup_flags_; }
  bool defer_enabled() const noexcept {
    return (setup_flags_ & IORING_SETUP_DEFER_TASKRUN) != 0;
  }
  bool post_resume() const noexcept { return options_.post_resume; }
  bool on_owner_thread() const noexcept { return current_context_ == this; }
  int error() const noexcept {
    if (init_error_ != 0) {
      return init_error_;
    }
    if (int failure = failure_error_.load(std::memory_order_acquire)) {
      return failure;
    }
    return requests_.stopped() ? -ECANCELED : 0;
  }

  Statistics statistics() const noexcept {
    return {
        requests_.accepted(),    completed_.load(),   rejected_.load(),
        requests_.wake_writes(), enter_calls_.load(), read_sqes_.load(),
        max_inflight_.load(),    queue_ns_.load(),    requests_.outstanding()};
  }

  uint64_t enter_count() const noexcept { return enter_calls_.load(); }
  uint64_t completed_count() const noexcept { return completed_.load(); }
  size_t inflight_size() const noexcept { return requests_.outstanding(); }

  void request_stop() noexcept { requests_.stop(); }

  void stop() noexcept {
    request_stop();
    if (thread_.joinable() && thread_.get_id() != std::this_thread::get_id()) {
      thread_.join();
    }
  }

  void submit_read(ReadRequest request) noexcept {
    auto *owned = new (std::nothrow) ReadRequest(std::move(request));
    if (!owned) {
      invoke(std::move(request.on_done), -ENOMEM);
      return;
    }
    owned->owned_ = true;
    enqueue(*owned);
  }

  bool submit_borrowed_read(ReadRequest &request) noexcept {
    return enqueue(request);
  }

 private:
  static Options environment_options(unsigned entries) {
    Options options;
    options.entries = entries;
    if (const char *value = std::getenv("YLT_OWN_RING_MODE")) {
      if (std::strcmp(value, "plain") == 0) {
        options.mode = Mode::plain;
      }
      else if (std::strcmp(value, "single") == 0) {
        options.mode = Mode::single_issuer;
      }
    }
    if (const char *value = std::getenv("YLT_OWN_RING_SPIN_US")) {
      options.spin_us = std::max(0, std::atoi(value));
    }
    if (const char *value = std::getenv("YLT_OWN_RING_POLL")) {
      options.poll = std::atoi(value) != 0;
    }
    if (const char *value = std::getenv("YLT_OWN_RING_POST_RESUME")) {
      options.post_resume =
          value[0] == '1' || value[0] == 't' || value[0] == 'T';
    }
    if (const char *value = std::getenv("YLT_OWN_RING_CPU_AFFINITY")) {
      cpu_set_t available;
      CPU_ZERO(&available);
      if (std::atoi(value) != 0 &&
          ::sched_getaffinity(0, sizeof(available), &available) == 0 &&
          CPU_COUNT(&available) > 0) {
        static std::atomic<unsigned> next{0};
        unsigned selected = next.fetch_add(1) % CPU_COUNT(&available);
        for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
          if (CPU_ISSET(cpu, &available) && selected-- == 0) {
            options.cpu_id = cpu;
            break;
          }
        }
      }
    }
    return options;
  }

  static void invoke(std::function<void(int)> callback, int result) noexcept {
    try {
      if (callback) {
        callback(result);
      }
    } catch (...) {
    }
  }

  static bool canceled(const ReadRequest &request) noexcept {
    return request.cancellation &&
           request.cancellation->load(std::memory_order_acquire);
  }

  static int validate(ReadRequest &request) {
    request.next_ = nullptr;
    request.retries_ = 0;
    request.done_ = 0;
    request.iov_index_ = 0;
    request.total_ = request.len;
    request.buffers_.clear();
    if (request.iovcnt < 0 || request.iovcnt > IOV_MAX ||
        (request.iovcnt > 0 && !request.iov) ||
        (request.iovcnt == 0 && request.len > 0 && !request.buf)) {
      return -EINVAL;
    }
    if (request.iovcnt > 0) {
      request.total_ = 0;
      request.buffers_.assign(request.iov, request.iov + request.iovcnt);
      for (const auto &buffer : request.buffers_) {
        if (buffer.iov_len > INT_MAX - request.total_) {
          return -EOVERFLOW;
        }
        if (buffer.iov_len && !buffer.iov_base) {
          return -EINVAL;
        }
        request.total_ += buffer.iov_len;
      }
    }
    if (request.total_ > INT_MAX || request.offset > INT64_MAX ||
        request.total_ > INT64_MAX - request.offset) {
      return -EOVERFLOW;
    }
    return 0;
  }

  bool enqueue(ReadRequest &request) noexcept {
    if (request.active_.exchange(true, std::memory_order_acq_rel)) {
      rejected_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }
    int error = 0;
    try {
      error = validate(request);
    } catch (...) {
      error = -ENOMEM;
    }
    if (error) {
      rejected_.fetch_add(1, std::memory_order_relaxed);
      release(request, error);
      return true;
    }
    if (options_.measure_queue_time) {
      request.queued_at_ = std::chrono::steady_clock::now();
    }
    error = requests_.push(request, current_context_ == this, [this] {
      return this->error();
    });
    if (error) {
      rejected_.fetch_add(1, std::memory_order_relaxed);
      release(request, error);
    }
    return true;
  }

  static void release(ReadRequest &request, int result) noexcept {
    auto callback = std::move(request.on_done);
    bool owned = request.owned_;
    request.active_.store(false, std::memory_order_release);
    if (owned) {
      delete &request;
    }
    invoke(std::move(callback), result);
  }

  void finish(ReadRequest &request, int result) noexcept {
    completed_.fetch_add(1, std::memory_order_relaxed);
    requests_.finish();
    release(request, result);
  }

  void prepare_reads() noexcept {
    unsigned prepared = 0;
    unsigned processed = 0;
    while (requests_.front() && processed++ < options_.submit_batch) {
      auto *request = requests_.front();
      if (error() || canceled(*request)) {
        requests_.pop();
        finish(*request, error() ? error() : -ECANCELED);
        continue;
      }
      if (request->total_ == 0) {
        requests_.pop();
        finish(*request, 0);
        continue;
      }
      auto *sqe = ::io_uring_get_sqe(&ring_);
      if (!sqe) {
        break;
      }
      requests_.pop();
      if (options_.measure_queue_time && request->done_ == 0 &&
          request->retries_ == 0) {
        auto elapsed = std::chrono::steady_clock::now() - request->queued_at_;
        queue_ns_.fetch_add(
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed)
                .count(),
            std::memory_order_relaxed);
      }
      if (request->iovcnt > 0) {
        ::io_uring_prep_readv(sqe, request->fd,
                              request->buffers_.data() + request->iov_index_,
                              request->buffers_.size() - request->iov_index_,
                              request->offset + request->done_);
      }
      else {
        ::io_uring_prep_read(sqe, request->fd,
                             static_cast<char *>(request->buf) + request->done_,
                             request->total_ - request->done_,
                             request->offset + request->done_);
      }
      ::io_uring_sqe_set_data(sqe, request);
      prepared_slots_[(ring_.sq.sqe_tail - 1) & ring_.sq.ring_mask] = request;
      ++prepared_;
      ++prepared;
    }
    read_sqes_.fetch_add(prepared, std::memory_order_relaxed);
  }

  void complete_read(ReadRequest &request, int result) noexcept {
    if (canceled(request) || error()) {
      finish(request, error() ? error() : -ECANCELED);
      return;
    }
    if (result == -EAGAIN || result == -EINTR) {
      if (++request.retries_ <= 5) {
        requests_.retry(request);
      }
      else {
        finish(request, result);
      }
      return;
    }
    if (result < 0) {
      finish(request, result);
      return;
    }
    if (static_cast<size_t>(result) > request.total_ - request.done_) {
      finish(request, -EIO);
      return;
    }
    request.done_ += result;
    if (result == 0 || request.done_ == request.total_) {
      finish(request, static_cast<int>(request.done_));
      return;
    }
    size_t consumed = result;
    while (request.iov_index_ < request.buffers_.size()) {
      auto &buffer = request.buffers_[request.iov_index_];
      if (consumed < buffer.iov_len) {
        buffer.iov_base = static_cast<char *>(buffer.iov_base) + consumed;
        buffer.iov_len -= consumed;
        break;
      }
      consumed -= buffer.iov_len;
      ++request.iov_index_;
    }
    requests_.retry(request);
  }

  unsigned reap() noexcept {
    struct Completion {
      void *data;
      int result;
    };
    std::array<Completion, 256> completions;
    unsigned head;
    io_uring_cqe *cqe;
    unsigned count = 0;
    io_uring_for_each_cqe(&ring_, head, cqe) {
      completions[count++] = {::io_uring_cqe_get_data(cqe), cqe->res};
      if (count == completions.size()) {
        break;
      }
    }
    ::io_uring_cq_advance(&ring_, count);
    for (unsigned index = 0; index < count; ++index) {
      const auto &completion = completions[index];
      if (completion.data == &wake_tag_) {
        wake_armed_ = false;
        requests_.consume_wake();
        continue;
      }
      auto *request = static_cast<ReadRequest *>(completion.data);
      --inflight_;
      int result = completion.result;
#ifdef OWN_RING_FAULT_INJECT
      if (fault().map_res) {
        result = fault().map_res(result, *request);
      }
#endif
      complete_read(*request, result);
    }
    return count;
  }

  void arm_wake() noexcept {
    if (wake_armed_ || options_.poll) {
      return;
    }
    if (auto *sqe = ::io_uring_get_sqe(&ring_)) {
      ::io_uring_prep_poll_add(sqe, requests_.wake_fd(), POLLIN);
      ::io_uring_sqe_set_data(sqe, &wake_tag_);
      prepared_slots_[(ring_.sq.sqe_tail - 1) & ring_.sq.ring_mask] =
          &wake_tag_;
      wake_armed_ = true;
    }
  }

  void initialize() noexcept {
    io_uring_params params{};
    if (options_.mode != Mode::plain) {
      params.flags |= IORING_SETUP_SINGLE_ISSUER;
    }
    if (options_.mode == Mode::defer_taskrun) {
      params.flags |= IORING_SETUP_DEFER_TASKRUN;
    }
    params.flags |= IORING_SETUP_CLAMP;
#ifdef OWN_RING_FAULT_INJECT
    if (fault().reject_optimized_setup && options_.mode != Mode::plain) {
      init_error_ = -EINVAL;
    }
    else
#endif
    {
      init_error_ =
          ::io_uring_queue_init_params(options_.entries, &ring_, &params);
    }
    if (init_error_ == -EINVAL && options_.allow_fallback &&
        options_.mode != Mode::plain) {
      params = {};
      init_error_ =
          ::io_uring_queue_init_params(options_.entries, &ring_, &params);
    }
#ifdef OWN_RING_FAULT_INJECT
    if (fault().force_init_fail) {
      if (init_error_ == 0) {
        ::io_uring_queue_exit(&ring_);
      }
      init_error_ = -EIO;
    }
#endif
    if (init_error_ == 0) {
      setup_flags_ = params.flags;
      try {
        prepared_slots_.resize(ring_.sq.ring_entries);
        submitted_head_ = *ring_.sq.khead;
      } catch (...) {
        ::io_uring_queue_exit(&ring_);
        init_error_ = -ENOMEM;
      }
    }
    {
      std::lock_guard lock(mutex_);
      initialized_ = true;
    }
    ready_.notify_one();
  }

  void track_submitted() noexcept {
    unsigned head = io_uring_smp_load_acquire(ring_.sq.khead);
    while (submitted_head_ != head) {
      auto &slot = prepared_slots_[submitted_head_++ & ring_.sq.ring_mask];
      if (slot && slot != &wake_tag_) {
        --prepared_;
        ++inflight_;
      }
      slot = nullptr;
    }
    if (inflight_ > max_inflight_.load(std::memory_order_relaxed)) {
      max_inflight_.store(inflight_, std::memory_order_relaxed);
    }
  }

  void drain_unsubmitted(int result) noexcept {
    assert(requests_.stopped());
    requests_.collect();
    unsigned tail = ring_.sq.sqe_tail;
    ring_.sq.sqe_head = ring_.sq.sqe_tail = submitted_head_;
    io_uring_smp_store_release(ring_.sq.ktail, submitted_head_);
    for (unsigned index = submitted_head_; index != tail; ++index) {
      auto data =
          std::exchange(prepared_slots_[index & ring_.sq.ring_mask], nullptr);
      if (data == &wake_tag_) {
        wake_armed_ = false;
      }
      else if (data) {
        --prepared_;
        finish(*static_cast<ReadRequest *>(data), result);
      }
    }
    while (auto *request = requests_.pop()) {
      finish(*request, result);
    }
    assert(prepared_ == 0);
  }

  void wait_inflight() noexcept {
    assert(requests_.stopped() && !requests_.front() && prepared_ == 0);
    if (inflight_ != 0) {
      io_uring_cqe *completion = nullptr;
      if (::io_uring_wait_cqe(&ring_, &completion) < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }

  void run() noexcept {
    current_context_ = this;
    if (options_.cpu_id >= 0 && options_.cpu_id < CPU_SETSIZE) {
      cpu_set_t selected;
      CPU_ZERO(&selected);
      CPU_SET(options_.cpu_id, &selected);
      ::sched_setaffinity(0, sizeof(selected), &selected);
    }
    initialize();
    if (!ok()) {
      current_context_ = nullptr;
      return;
    }
    auto spin_deadline = std::chrono::steady_clock::now();
    while (true) {
      requests_.collect();
      if (reap() > 0) {
        spin_deadline = std::chrono::steady_clock::now() +
                        std::chrono::microseconds(options_.spin_us);
      }
      if (!requests_.stopped()) {
        prepare_reads();
      }
      if (requests_.stopped()) {
        drain_unsubmitted(error());
        if (requests_.outstanding() == 0) {
          break;
        }
        wait_inflight();
        continue;
      }
      arm_wake();
      bool park = !options_.poll && !requests_.front() && wake_armed_ &&
                  (options_.spin_us == 0 || inflight_ == 0 ||
                   std::chrono::steady_clock::now() >= spin_deadline);
      if (park) {
        park = requests_.prepare_wait(inflight_);
      }
#ifdef OWN_RING_FAULT_INJECT
      if (fault().before_submit) {
        fault().before_submit();
      }
#endif
      enter_calls_.fetch_add(1, std::memory_order_relaxed);
      int result;
#ifdef OWN_RING_FAULT_INJECT
      if (fault().submit) {
        result = fault().submit(&ring_, park);
      }
      else
#endif
      {
        result = park ? ::io_uring_submit_and_wait(&ring_, 1)
                      : ::io_uring_submit_and_get_events(&ring_);
      }
      track_submitted();
      if (result < 0 && result != -EINTR && result != -EAGAIN &&
          result != -EBUSY) {
        failure_error_.store(result, std::memory_order_release);
        request_stop();
      }
    }
    ::io_uring_queue_exit(&ring_);
    current_context_ = nullptr;
  }

  inline static thread_local OwnRingDriver *current_context_ = nullptr;
  Options options_;
  OwnRingRequestQueue requests_;
  io_uring ring_{};
  int init_error_ = -EIO;
  unsigned setup_flags_ = 0;
  int wake_tag_ = 0;
  bool wake_armed_ = false;
  bool initialized_ = false;
  size_t inflight_ = 0;
  size_t prepared_ = 0;
  unsigned submitted_head_ = 0;
  std::vector<void *> prepared_slots_;
  std::mutex mutex_;
  std::condition_variable ready_;
  std::atomic<int> failure_error_{0};
  std::atomic<uint64_t> completed_{0};
  std::atomic<uint64_t> rejected_{0};
  std::atomic<uint64_t> enter_calls_{0};
  std::atomic<uint64_t> read_sqes_{0};
  std::atomic<uint64_t> max_inflight_{0};
  std::atomic<uint64_t> queue_ns_{0};
  std::thread thread_;
};

}
