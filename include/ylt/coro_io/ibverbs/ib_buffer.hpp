#pragma once

#include <infiniband/verbs.h>
#include <sys/types.h>

#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <stdexcept>
#include <system_error>

#include "async_simple/coro/Lazy.h"
#include "ib_device.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/detail/client_queue.hpp"
#include "ylt/easylog.hpp"

namespace coro_io {

class ib_buffer_pool_t;

struct ib_buffer_t {
 private:
  std::unique_ptr<ibv_mr, ib_deleter> mr_;
  std::weak_ptr<ib_buffer_pool_t> owner_pool_;
  std::unique_ptr<char[]> memory_owner_;
  ib_buffer_t(std::unique_ptr<ibv_mr, ib_deleter> mr,
              std::unique_ptr<char[]> memory_owner,
              ib_buffer_pool_t& owner_pool) noexcept;
  void release_resource();

 public:
  ib_buffer_t() = default;
  friend class ib_buffer_pool_t;
  ibv_sge subview(std::size_t start = 0) const {
    return ibv_sge{.addr = (uintptr_t)mr_->addr + start,
                   .length = (uint32_t)(mr_->length - start),
                   .lkey = mr_->lkey};
  }
  ibv_sge subview(std::size_t start, uint32_t length) const {
    return ibv_sge{.addr = (uintptr_t)mr_->addr + start,
                   .length = length,
                   .lkey = mr_->lkey};
  }
  ibv_mr* operator->() noexcept { return mr_.get(); }
  ibv_mr* operator->() const noexcept { return mr_.get(); }
  ibv_mr& operator*() noexcept { return *mr_.get(); }
  ibv_mr& operator*() const noexcept { return *mr_.get(); }
  operator bool() const noexcept { return mr_.get() != nullptr; }
  static std::unique_ptr<ibv_mr> regist(
      ib_device_t& dev, void* ptr, uint32_t size,
      int ib_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                     IBV_ACCESS_REMOTE_WRITE) {
    auto mr = ibv_reg_mr(dev.pd(), ptr, size, ib_flags);
    ELOG_TRACE << "ibv_reg_mr regist: " << mr << " with pd:" << dev.pd();
    if (mr != nullptr) [[unlikely]] {
      ELOG_TRACE << "regist sge.lkey: " << mr->lkey
                 << ", sge.addr: " << mr->addr
                 << ", sge.length: " << mr->length;
      return std::unique_ptr<ibv_mr>{mr};
    }
    else {
      throw std::make_error_code(std::errc{errno});
    }
  };
  static ib_buffer_t regist(ib_buffer_pool_t& pool,
                            std::unique_ptr<char[]> data, std::size_t size,
                            int ib_flags = IBV_ACCESS_LOCAL_WRITE |
                                           IBV_ACCESS_REMOTE_READ |
                                           IBV_ACCESS_REMOTE_WRITE);
  ib_buffer_t(ib_buffer_t&&) = default;
  ib_buffer_t& operator=(ib_buffer_t&&);
  ~ib_buffer_t();
};

std::shared_ptr<ib_device_t> g_ib_device(ib_config_t conf = {});
class ib_buffer_pool_t : public std::enable_shared_from_this<ib_buffer_pool_t> {
 private:
  friend struct ib_buffer_t;

  struct ib_buffer_impl_t {
    std::unique_ptr<ibv_mr, ib_deleter> mr_;
    std::unique_ptr<char[]> memory_owner_;
    ib_buffer_t convert_to_ib_buffer(ib_buffer_pool_t& pool) && {
      return ib_buffer_t{std::move(mr_), std::move(memory_owner_), pool};
    }
    ib_buffer_impl_t& operator=(ib_buffer_impl_t&& o) noexcept = default;
    ib_buffer_impl_t(ib_buffer_impl_t&& o) noexcept = default;
    ib_buffer_impl_t() noexcept = default;
    ib_buffer_impl_t(std::unique_ptr<ibv_mr, ib_deleter>&& mr,
                     std::unique_ptr<char[]>&& memory_owner) noexcept
        : mr_(std::move(mr)), memory_owner_(std::move(memory_owner)) {}
  };
  struct private_construct_token {};
  static async_simple::coro::Lazy<void> collect_idle_timeout_client(
      std::weak_ptr<ib_buffer_pool_t> self_weak,
      std::chrono::milliseconds sleep_time) {
    std::shared_ptr<ib_buffer_pool_t> self = self_weak.lock();
    if (self == nullptr) {
      co_return;
    }
    while (true) {
      self->free_buffers_.reselect();
      self = nullptr;
      co_await coro_io::sleep_for(sleep_time);
      if ((self = self_weak.lock()) == nullptr) {
        break;
      }
      while (true) {
        ELOG_TRACE << "start ib_buffer timeout free of pool{" << self.get()
                   << "}, now ib_buffer count: " << self->free_buffers_.size();
        std::size_t clear_cnt = self->free_buffers_.clear_old(1000);
        self->total_memory_ -= clear_cnt * self->buffer_size();
        ELOG_INFO << "finish ib_buffer timeout free of pool{" << self.get()
                  << "}, now ib_buffer cnt: " << self->free_buffers_.size()
                  << " mem usage:"
                  << (int64_t)(std::round(self->total_memory_ /
                                          (1.0 * 1024 * 1024)))
                  << " MB";
        if (clear_cnt != 0) {
          try {
            co_await async_simple::coro::Yield{};
          } catch (std::exception& e) {
            ELOG_ERROR << "unexcepted yield exception: " << e.what();
          }
        }
        else {
          break;
        }
      }
      --self->free_buffers_.collecter_cnt_;
      if (self->free_buffers_.size() == 0) {
        break;
      }
      std::size_t expected = 0;
      if (!self->free_buffers_.collecter_cnt_.compare_exchange_strong(expected,
                                                                      1))
        break;
    }
    co_return;
  }
  void enqueue(ib_buffer_t& buffer) {
    auto impl = std::make_unique<ib_buffer_impl_t>(
        std::move(buffer.mr_), std::move(buffer.memory_owner_));
    if (free_buffers_.enqueue(std::move(impl)) == 1) {
      std::size_t expected = 0;
      if (free_buffers_.collecter_cnt_.compare_exchange_strong(expected, 1))
          [[unlikely]] {
        ELOG_TRACE << "start timeout client collecter of client_pool{" << this
                   << "}";
        collect_idle_timeout_client(this->weak_from_this(),
                                    (std::max)(pool_config_.idle_timeout,
                                               std::chrono::milliseconds{50}))
            .directlyStart(
                [](auto&&) {
                },
                coro_io::get_global_executor());
      }
    }
  }
  void collect_free(ib_buffer_t& buffer) {
    if (buffer) {
      if (free_buffers_.size() * buffer_size() <
          pool_config_.max_memory_usage) {
        ELOG_TRACE << "collect free buffer{data:" << buffer->addr << ",len"
                   << buffer->length << "} enqueue";
        enqueue(buffer);
      }
      else {
        ELOG_TRACE << "out of max connection limit <<"
                   << pool_config_.max_memory_usage
                   << "buffer{data:" << buffer->addr << ",len" << buffer->length
                   << "} wont be collect";
      }
    }
    return;
  };

 public:
  friend struct ib_buffer_t;
  std::size_t buffer_size() const noexcept {
    return this->pool_config_.buffer_size;
  }
  ib_buffer_t get_buffer() {
    std::unique_ptr<ib_buffer_impl_t> buffer;
    ib_buffer_t ib_buffer;
    free_buffers_.try_dequeue(buffer);
    if (!buffer) {
      ELOG_TRACE
          << "There is no free buffer. Allocate and regist new buffer now";
      if (pool_config_.max_memory_usage <
          buffer_size() + total_memory_.load(std::memory_order_acquire))
          [[unlikely]] {
        ELOG_WARN << "Memory out of pool limit";
        return ib_buffer_t{};
      }
      std::unique_ptr<char[]> data;
      data.reset(new char[buffer_size()]);
      ib_buffer = ib_buffer_t::regist(*this, std::move(data), buffer_size());
    }
    else {
      ib_buffer = std::move(*buffer).convert_to_ib_buffer(*this);
    }
    ELOG_TRACE << "get buffer{data:" << ib_buffer->addr << ",len"
               << ib_buffer->length << "} from queue";
    return ib_buffer;
  }
  struct config_t {
    size_t buffer_size = 2 * 1024 * 1024;                   // 2MB
    uint64_t max_memory_usage = 4ull * 1024 * 1024 * 1024;  // 4GB
    std::chrono::milliseconds idle_timeout = std::chrono::milliseconds{5000};
  };
  ib_buffer_pool_t(private_construct_token t,
                   std::shared_ptr<ib_device_t> device,
                   const config_t& pool_config)
      : device_(device ? std::move(device) : coro_io::g_ib_device()),
        pool_config_(std::move(pool_config)) {}
  static std::shared_ptr<ib_buffer_pool_t> create(
      std::shared_ptr<ib_device_t> device, const config_t& pool_config) {
    return std::make_shared<ib_buffer_pool_t>(private_construct_token{},
                                              std::move(device), pool_config);
  }
  static std::shared_ptr<ib_buffer_pool_t> create(
      std::shared_ptr<ib_device_t> device) {
    return std::make_shared<ib_buffer_pool_t>(private_construct_token{},
                                              std::move(device), config_t{});
  }
  std::atomic<uint64_t> total_memory_ = 0;
  std::size_t total_memory() const noexcept {
    return total_memory_.load(std::memory_order_acquire);
  }
  std::size_t free_client_size() const noexcept { return free_buffers_.size(); }
  coro_io::detail::client_queue<std::unique_ptr<ib_buffer_impl_t>>
      free_buffers_;
  std::shared_ptr<ib_device_t> device_;
  config_t pool_config_;
};

inline ib_buffer_t ib_buffer_t::regist(ib_buffer_pool_t& pool,
                                       std::unique_ptr<char[]> data,
                                       std::size_t size, int ib_flags) {
  auto mr = ibv_reg_mr(pool.device_->pd(), data.get(), size, ib_flags);
  if (mr != nullptr) [[unlikely]] {
    ELOG_DEBUG << "ibv_reg_mr regist: " << mr
               << " with pd:" << pool.device_->pd();
    std::unique_ptr<int> a;
    a.release();
    pool.total_memory_.fetch_add(size, std::memory_order_relaxed);
    return ib_buffer_t{std::unique_ptr<ibv_mr, ib_deleter>{mr}, std::move(data),
                       pool};
  }
  else {
    throw std::make_error_code(std::errc{errno});
  }
};

inline std::shared_ptr<ib_device_t> g_ib_device(ib_config_t conf) {
  static auto dev = std::make_shared<ib_device_t>(conf);
  return dev;
}

inline std::shared_ptr<ib_buffer_pool_t> g_ib_buffer_pool(
    const ib_buffer_pool_t::config_t& pool_config = {}) {
  static auto pool = ib_buffer_pool_t::create(g_ib_device(), pool_config);
  return pool;
}

inline ib_buffer_t::ib_buffer_t(std::unique_ptr<ibv_mr, ib_deleter> mr,
                                std::unique_ptr<char[]> memory_owner,
                                ib_buffer_pool_t& owner_pool) noexcept
    : mr_(std::move(mr)),
      memory_owner_(std::move(memory_owner)),
      owner_pool_(owner_pool.weak_from_this()) {}

inline ib_buffer_t& ib_buffer_t::operator=(ib_buffer_t&& o) {
  release_resource();
  mr_ = std::move(o.mr_);
  owner_pool_ = std::move(o.owner_pool_);
  memory_owner_ = std::move(o.memory_owner_);
  return *this;
}

inline ib_buffer_t::~ib_buffer_t() { release_resource(); }

inline void ib_buffer_t::release_resource() {
  if (mr_) {
    if (auto ptr = owner_pool_.lock(); ptr) {
      ptr->collect_free(*this);
      // if buffer not collect by pool, we need dec total memory here.
      if (mr_) [[unlikely]] {
        ptr->total_memory_ -= mr_->length;
      }
    }
  }
}

}  // namespace coro_io