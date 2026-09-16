#pragma once

#include <sys/uio.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace coro_io::detail {

class OwnRingDriver;
class OwnRingRequestQueue;

enum class OwnRingMode { plain, single_issuer, defer_taskrun };

struct OwnRingOptions {
  unsigned entries = 4096;
  unsigned submit_batch = 128;
  size_t max_requests = 65536;
  OwnRingMode mode = OwnRingMode::defer_taskrun;
  bool allow_fallback = true;
  bool poll = false;
  unsigned spin_us = 0;
  int cpu_id = -1;
  bool post_resume = true;
  bool measure_queue_time = false;
};

struct OwnRingReadRequest {
  OwnRingReadRequest() = default;
  OwnRingReadRequest(const OwnRingReadRequest &) = delete;
  OwnRingReadRequest &operator=(const OwnRingReadRequest &) = delete;
  OwnRingReadRequest(OwnRingReadRequest &&other) noexcept {
    *this = std::move(other);
  }
  OwnRingReadRequest &operator=(OwnRingReadRequest &&other) noexcept {
    assert(!active_.load(std::memory_order_relaxed));
    assert(!other.active_.load(std::memory_order_relaxed));
    fd = other.fd;
    buf = other.buf;
    len = other.len;
    iov = other.iov;
    iovcnt = other.iovcnt;
    offset = other.offset;
    on_done = std::move(other.on_done);
    cancellation = std::move(other.cancellation);
    buffers_ = std::move(other.buffers_);
    return *this;
  }
  ~OwnRingReadRequest() { assert(!active_.load(std::memory_order_relaxed)); }

  int fd = -1;
  void *buf = nullptr;
  unsigned len = 0;
  const iovec *iov = nullptr;
  int iovcnt = 0;
  uint64_t offset = 0;
  std::function<void(int)> on_done;
  std::shared_ptr<std::atomic<bool>> cancellation;

 private:
  friend class OwnRingDriver;
  friend class OwnRingRequestQueue;
  std::atomic<bool> active_{false};
  OwnRingReadRequest *next_ = nullptr;
  bool owned_ = false;
  unsigned retries_ = 0;
  size_t total_ = 0;
  size_t done_ = 0;
  size_t iov_index_ = 0;
  std::vector<iovec> buffers_;
  std::chrono::steady_clock::time_point queued_at_;
};

struct OwnRingStatistics {
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

}
