#pragma once

#include <liburing.h>
#include <poll.h>
#include <sched.h>
#include <sys/eventfd.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <charconv>
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
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace coro_io {

class OwnRingIoContext {
 public:
  enum class Mode { plain, single_issuer, defer_taskrun };

  struct Options {
    unsigned entries = 4096;
    unsigned submit_batch = 128;
    size_t max_requests = 65536;
    Mode mode = Mode::defer_taskrun;
    bool allow_fallback = true;
    bool poll = false;
    unsigned spin_us = 0;
    int cpu_id = -1;
    bool post_resume = false;
    bool measure_queue_time = false;
  };

  struct ReadRequest {
    int fd = -1;
    void *buf = nullptr;
    unsigned len = 0;
    const iovec *iov = nullptr;
    int iovcnt = 0;
    uint64_t offset = 0;
    std::function<void(int)> on_done;
    std::shared_ptr<std::atomic<bool>> cancellation;

   private:
    friend class OwnRingIoContext;
    ReadRequest *next_ = nullptr;
    bool owned_ = false;
    unsigned retries_ = 0;
    size_t total_ = 0;
    size_t done_ = 0;
    size_t iov_index_ = 0;
    std::vector<iovec> buffers_;
    std::chrono::steady_clock::time_point queued_at_;
  };

  struct Statistics {
    uint64_t accepted;
    uint64_t completed;
    uint64_t rejected;
    uint64_t wake_writes;
    uint64_t enter_calls;
    uint64_t read_sqes;
    uint64_t max_inflight;
    uint64_t queue_ns;
    uint64_t outstanding;
  };

#ifdef OWN_RING_FAULT_INJECT
  struct FaultHooks {
    std::function<int(int, const ReadRequest &)> map_res;
    std::function<int(int)> map_submit_rc;
    std::function<void()> before_submit;
    bool force_init_fail = false;
    bool reject_optimized_setup = false;
  };

  static FaultHooks &fault() {
    static FaultHooks hooks;
    return hooks;
  }
#endif

  explicit OwnRingIoContext(unsigned entries = 4096)
      : OwnRingIoContext(environment_options(entries)) {}

  explicit OwnRingIoContext(Options options) : options_(options) {
    options_.submit_batch = std::max(1u, options_.submit_batch);
    if (options_.entries < 2 || options_.max_requests == 0) {
      init_error_ = -EINVAL;
      return;
    }
    wake_fd_ = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (wake_fd_ < 0) {
      init_error_ = -errno;
      return;
    }
    try {
      thread_ = std::thread([this] {
        run();
      });
    } catch (...) {
      ::close(wake_fd_);
      wake_fd_ = -1;
      throw;
    }
    std::unique_lock lock(mutex_);
    ready_.wait(lock, [this] {
      return initialized_;
    });
  }

  OwnRingIoContext(const OwnRingIoContext &) = delete;
  OwnRingIoContext &operator=(const OwnRingIoContext &) = delete;

  ~OwnRingIoContext() {
    if (thread_.joinable() && thread_.get_id() == std::this_thread::get_id()) {
      std::terminate();
    }
    stop();
    if (wake_fd_ >= 0) {
      ::close(wake_fd_);
    }
  }

  bool ok() const noexcept { return init_error_ == 0; }
  int init_error() const noexcept { return init_error_; }
  unsigned setup_flags() const noexcept { return setup_flags_; }
  bool defer_enabled() const noexcept {
    return (setup_flags_ & IORING_SETUP_DEFER_TASKRUN) != 0;
  }
  bool post_resume() const noexcept { return options_.post_resume; }

  Statistics statistics() const noexcept {
    return {accepted_.load(),     completed_.load(),   rejected_.load(),
            wake_writes_.load(),  enter_calls_.load(), read_sqes_.load(),
            max_inflight_.load(), queue_ns_.load(),    outstanding_.load()};
  }

  uint64_t enter_count() const noexcept { return enter_calls_.load(); }
  uint64_t completed_count() const noexcept { return completed_.load(); }
  size_t inflight_size() const noexcept { return outstanding_.load(); }

  void request_stop() noexcept {
    bool notify = false;
    {
      std::lock_guard lock(mutex_);
      stopping_.store(true, std::memory_order_release);
      notify = std::exchange(sleeping_, false);
    }
    if (notify) {
      wake();
    }
  }

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

  void submit_borrowed_read(ReadRequest &request) noexcept {
    request.owned_ = false;
    enqueue(request);
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

  void enqueue(ReadRequest &request) noexcept {
    int error = 0;
    try {
      error = validate(request);
    } catch (...) {
      error = -ENOMEM;
    }
    if (error) {
      rejected_.fetch_add(1, std::memory_order_relaxed);
      release(request, error);
      return;
    }
    if (options_.measure_queue_time) {
      request.queued_at_ = std::chrono::steady_clock::now();
    }
    if (current_context_ == this) {
      if (stopping_.load(std::memory_order_acquire)) {
        error = -ECANCELED;
      }
      else if (!reserve_request()) {
        error = -EAGAIN;
      }
      else {
        accepted_.fetch_add(1, std::memory_order_relaxed);
        append(pending_head_, pending_tail_, &request);
        return;
      }
      rejected_.fetch_add(1, std::memory_order_relaxed);
      release(request, error);
      return;
    }
    bool notify = false;
    {
      std::lock_guard lock(mutex_);
      if (!ok() || failed_.load(std::memory_order_relaxed)) {
        error = -EIO;
      }
      else if (stopping_.load(std::memory_order_relaxed)) {
        error = -ECANCELED;
      }
      else if (!reserve_request()) {
        error = -EAGAIN;
      }
      else {
        accepted_.fetch_add(1, std::memory_order_relaxed);
        append(incoming_head_, incoming_tail_, &request);
        notify = std::exchange(sleeping_, false);
      }
    }
    if (error) {
      rejected_.fetch_add(1, std::memory_order_relaxed);
      release(request, error);
    }
    else if (notify) {
      wake();
    }
  }

  bool reserve_request() noexcept {
    auto count = outstanding_.load(std::memory_order_relaxed);
    while (count < options_.max_requests) {
      if (outstanding_.compare_exchange_weak(count, count + 1,
                                             std::memory_order_relaxed)) {
        return true;
      }
    }
    return false;
  }

  static void append(ReadRequest *&head, ReadRequest *&tail,
                     ReadRequest *request) noexcept {
    request->next_ = nullptr;
    if (tail) {
      tail->next_ = request;
    }
    else {
      head = request;
    }
    tail = request;
  }

  static ReadRequest *pop(ReadRequest *&head, ReadRequest *&tail) noexcept {
    auto *request = head;
    head = request->next_;
    if (!head) {
      tail = nullptr;
    }
    return request;
  }

  static void release(ReadRequest &request, int result) noexcept {
    auto callback = std::move(request.on_done);
    if (request.owned_) {
      delete &request;
    }
    invoke(std::move(callback), result);
  }

  void finish(ReadRequest &request, int result) noexcept {
    completed_.fetch_add(1, std::memory_order_relaxed);
    outstanding_.fetch_sub(1, std::memory_order_release);
    release(request, result);
  }

  void wake() noexcept {
    uint64_t value = 1;
    ssize_t result;
    do {
      result = ::write(wake_fd_, &value, sizeof(value));
    } while (result < 0 && errno == EINTR);
    wake_writes_.fetch_add(1, std::memory_order_relaxed);
  }

  void collect_incoming() noexcept {
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

  void prepare_reads() noexcept {
    unsigned prepared = 0;
    unsigned processed = 0;
    while (pending_head_ && processed++ < options_.submit_batch) {
      auto *request = pending_head_;
      if (stopping_.load(std::memory_order_acquire) || canceled(*request)) {
        pop(pending_head_, pending_tail_);
        finish(*request, -ECANCELED);
        continue;
      }
      if (request->total_ == 0) {
        pop(pending_head_, pending_tail_);
        finish(*request, 0);
        continue;
      }
      auto *sqe = ::io_uring_get_sqe(&ring_);
      if (!sqe) {
        break;
      }
      pop(pending_head_, pending_tail_);
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
      ++inflight_;
      ++prepared;
    }
    read_sqes_.fetch_add(prepared, std::memory_order_relaxed);
    if (inflight_ > max_inflight_.load(std::memory_order_relaxed)) {
      max_inflight_.store(inflight_, std::memory_order_relaxed);
    }
  }

  void complete_read(ReadRequest &request, int result) noexcept {
    if (canceled(request) || stopping_.load(std::memory_order_acquire)) {
      finish(request, -ECANCELED);
      return;
    }
    if (result == -EAGAIN || result == -EINTR) {
      if (++request.retries_ <= 5) {
        append(pending_head_, pending_tail_, &request);
      }
      else {
        finish(request, -EIO);
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
    append(pending_head_, pending_tail_, &request);
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
        uint64_t value;
        while (::read(wake_fd_, &value, sizeof(value)) > 0) {
        }
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
      ::io_uring_prep_poll_add(sqe, wake_fd_, POLLIN);
      ::io_uring_sqe_set_data(sqe, &wake_tag_);
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
    }
    {
      std::lock_guard lock(mutex_);
      initialized_ = true;
    }
    ready_.notify_one();
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
      collect_incoming();
      if (reap() > 0) {
        spin_deadline = std::chrono::steady_clock::now() +
                        std::chrono::microseconds(options_.spin_us);
      }
      prepare_reads();
      if (stopping_.load(std::memory_order_acquire) &&
          outstanding_.load(std::memory_order_acquire) == 0) {
        break;
      }
      arm_wake();
      bool park = !options_.poll && !pending_head_ && wake_armed_ &&
                  (options_.spin_us == 0 || inflight_ == 0 ||
                   std::chrono::steady_clock::now() >= spin_deadline);
      if (park) {
        std::lock_guard lock(mutex_);
        park = incoming_head_ == nullptr &&
               (!stopping_.load(std::memory_order_relaxed) || inflight_ > 0);
        sleeping_ = park;
      }
#ifdef OWN_RING_FAULT_INJECT
      if (fault().before_submit) {
        fault().before_submit();
      }
#endif
      enter_calls_.fetch_add(1, std::memory_order_relaxed);
      int result = park ? ::io_uring_submit_and_wait(&ring_, 1)
                        : ::io_uring_submit_and_get_events(&ring_);
#ifdef OWN_RING_FAULT_INJECT
      if (fault().map_submit_rc) {
        result = fault().map_submit_rc(result);
      }
#endif
      if (result < 0 && result != -EINTR && result != -EAGAIN &&
          result != -EBUSY) {
        failed_.store(true, std::memory_order_release);
        request_stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
    ::io_uring_queue_exit(&ring_);
    current_context_ = nullptr;
  }

  inline static thread_local OwnRingIoContext *current_context_ = nullptr;
  Options options_;
  io_uring ring_{};
  int init_error_ = -EIO;
  unsigned setup_flags_ = 0;
  int wake_fd_ = -1;
  int wake_tag_ = 0;
  bool wake_armed_ = false;
  bool initialized_ = false;
  bool sleeping_ = false;
  size_t inflight_ = 0;
  ReadRequest *incoming_head_ = nullptr;
  ReadRequest *incoming_tail_ = nullptr;
  ReadRequest *pending_head_ = nullptr;
  ReadRequest *pending_tail_ = nullptr;
  std::mutex mutex_;
  std::condition_variable ready_;
  std::atomic<bool> stopping_{false};
  std::atomic<bool> failed_{false};
  std::atomic<uint64_t> accepted_{0};
  std::atomic<uint64_t> completed_{0};
  std::atomic<uint64_t> rejected_{0};
  std::atomic<uint64_t> wake_writes_{0};
  std::atomic<uint64_t> enter_calls_{0};
  std::atomic<uint64_t> read_sqes_{0};
  std::atomic<uint64_t> max_inflight_{0};
  std::atomic<uint64_t> queue_ns_{0};
  std::atomic<uint64_t> outstanding_{0};
  std::thread thread_;
};

inline OwnRingIoContext &global_own_ring_context() {
  static auto *contexts = [] {
    unsigned count = 1;
    if (const char *value = std::getenv("YLT_OWN_RING_THREADS")) {
      std::string_view text(value);
      auto parsed =
          std::from_chars(text.data(), text.data() + text.size(), count);
      if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
          count == 0 || count > 64) {
        throw std::invalid_argument(
            "YLT_OWN_RING_THREADS must be between 1 and 64");
      }
    }
    auto pool =
        std::make_unique<std::vector<std::unique_ptr<OwnRingIoContext>>>();
    pool->reserve(count);
    for (unsigned index = 0; index < count; ++index) {
      auto context = std::make_unique<OwnRingIoContext>();
      if (!context->ok()) {
        throw std::system_error(-context->init_error(), std::system_category());
      }
      pool->push_back(std::move(context));
    }
    return pool.release();
  }();
  static std::atomic<size_t> next{0};
  return *(*contexts)[next.fetch_add(1, std::memory_order_relaxed) %
                      contexts->size()];
}

}  // namespace coro_io
