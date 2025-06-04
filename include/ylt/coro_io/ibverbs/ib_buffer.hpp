#pragma once

#include <infiniband/verbs.h>
#include <sys/types.h>

#include <atomic>
#include <cerrno>
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
  std::shared_ptr<ib_device_t> dev_;
  std::weak_ptr<ib_buffer_pool_t> owner_pool_;
  bool has_memory_ownership_;
  ib_buffer_t(ibv_mr* mr, std::shared_ptr<ib_device_t> dev,
              std::weak_ptr<ib_buffer_pool_t> owner_pool = {});
  void set_has_ownership() { has_memory_ownership_ = true; }

 public:
  friend class ib_buffer_pool_t;
  ib_buffer_t() = default;
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
  static ib_buffer_t regist(std::shared_ptr<ib_device_t> dev, void* ptr,
                            uint32_t size,
                            int ib_flags = IBV_ACCESS_LOCAL_WRITE |
                                           IBV_ACCESS_REMOTE_READ |
                                           IBV_ACCESS_REMOTE_WRITE) {
    auto mr = ibv_reg_mr(dev->pd(), ptr, size, ib_flags);
    ELOG_TRACE << "ibv_reg_mr regist: " << mr << " with pd:" << dev->pd();
    if (mr != nullptr) [[unlikely]] {
      ELOG_TRACE << "regist sge.lkey: " << mr->lkey
                 << ", sge.addr: " << mr->addr
                 << ", sge.length: " << mr->length;
      return ib_buffer_t{mr, std::move(dev)};
    }
    else {
      throw std::make_error_code(std::errc{errno});
    }
  };
  static ib_buffer_t regist(ib_buffer_pool_t& pool,
                            std::shared_ptr<ib_device_t> dev, void* ptr,
                            std::size_t size,
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
        ELOG_TRACE << "finish ib_buffer timeout free of pool{" << self.get()
                  << "}, now ib_buffer cnt: " << self->free_buffers_.size()
                  << " mem usage:" << self->total_memory_;
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
  void enqueue(ib_buffer_t client) {
    client.owner_pool_ = std::weak_ptr<ib_buffer_pool_t>{};
    if (free_buffers_.enqueue(std::move(client)) == 1) {
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
  void collect_free(ib_buffer_t buffer) {
    if (buffer) {
      if (free_buffers_.size() * buffer_size() <
          pool_config_.max_memory_usage) {
        ELOG_TRACE << "collect free buffer{data:" << buffer->addr << ",len"
                   << buffer->length << "} enqueue";
        enqueue(std::move(buffer));
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
    ib_buffer_t buffer;
    free_buffers_.try_dequeue(buffer);
    if (!buffer) {
      ELOG_TRACE
          << "There is no free buffer. Allocate and regist new buffer now";
      if (pool_config_.max_memory_usage <
          buffer_size() + total_memory_.load(std::memory_order_acquire))
          [[unlikely]] {
        ELOG_WARN << "Memory out of pool limit";
        return std::move(buffer);
      }
      auto ptr = malloc(buffer_size());
      if (ptr == nullptr) {
        ELOG_ERROR << "ib_buffer_pool allocate failed.";
        throw std::bad_alloc();
      }
      buffer = ib_buffer_t::regist(*this, device_, ptr, buffer_size());
      buffer.set_has_ownership();
    }
    else {
      buffer.owner_pool_ = weak_from_this();
    }
    ELOG_TRACE << "get buffer{data:" << buffer->addr << ",len" << buffer->length
               << "} from queue";
    return buffer;
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
  coro_io::detail::client_queue<ib_buffer_t> free_buffers_;
  std::shared_ptr<ib_device_t> device_;
  config_t pool_config_;
};

inline ib_buffer_t ib_buffer_t::regist(ib_buffer_pool_t& pool,
                                       std::shared_ptr<ib_device_t> dev,
                                       void* ptr, std::size_t size,
                                       int ib_flags) {
  auto mr = ibv_reg_mr(dev->pd(), ptr, size, ib_flags);
  if (mr != nullptr) [[unlikely]] {
    ELOG_DEBUG << "ibv_reg_mr regist: " << mr << " with pd:" << dev->pd();
    return ib_buffer_t{mr, std::move(dev), pool.weak_from_this()};
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

inline ib_buffer_t::ib_buffer_t(ibv_mr* mr, std::shared_ptr<ib_device_t> dev,
                                std::weak_ptr<ib_buffer_pool_t> owner_pool)
    : mr_(mr),
      dev_(dev ? std::move(dev) : coro_io::g_ib_device()),
      owner_pool_(std::move(owner_pool)),
      has_memory_ownership_(false) {
  if (auto ptr = owner_pool_.lock(); ptr) {
    ptr->total_memory_.fetch_add((*this)->length, std::memory_order_relaxed);
  }
}

inline ib_buffer_t& ib_buffer_t::operator=(ib_buffer_t&& o) {
  this->~ib_buffer_t();
  mr_ = std::move(o.mr_);
  dev_ = std::move(o.dev_);
  owner_pool_ = std::move(o.owner_pool_);
  has_memory_ownership_ = o.has_memory_ownership_;
  return *this;
}

inline ib_buffer_t::~ib_buffer_t() {
  if (mr_) {
    if (auto ptr = owner_pool_.lock(); ptr) {
      ptr->collect_free(std::move(*this));
    }
    else if (has_memory_ownership_) {
      auto data = mr_->addr;
      mr_ = nullptr;
      free(data);
    }
  }
}

}  // namespace coro_io