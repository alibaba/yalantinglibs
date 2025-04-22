#pragma once

#include <infiniband/verbs.h>
#include <sys/types.h>

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <system_error>

#include "async_simple/coro/Lazy.h"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/easylog.hpp"
#include "ylt/coro_io/detail/client_queue.hpp"

#include "ib_device.hpp"

namespace coro_io {

class ib_buffer_pool_t;
struct ib_buffer_t {
  
 private:
  std::unique_ptr<ibv_mr, ib_deleter> mr_;
  std::shared_ptr<ib_device_t> dev_;
  std::weak_ptr<ib_buffer_pool_t> owner_pool_;
  ib_buffer_t(ibv_mr* mr, std::shared_ptr<ib_device_t> dev, std::weak_ptr<ib_buffer_pool_t> owner_pool={});
 public:
 
  ib_buffer_t()=default;
  ibv_mr* operator->() noexcept {
    return mr_.get();
  }
  ibv_mr* operator->() const noexcept{
    return mr_.get();
  }
  ibv_mr& operator*() noexcept {
    return *mr_.get();
  }
  ibv_mr& operator*() const noexcept{
    return *mr_.get();
  }
  operator bool() const noexcept {
    return mr_.get()!=nullptr;
  }
  static ib_buffer_t regist(std::shared_ptr<ib_device_t> dev, void* ptr, std::size_t size,
                            int ib_flags = IBV_ACCESS_LOCAL_WRITE |
                                           IBV_ACCESS_REMOTE_READ |
                                           IBV_ACCESS_REMOTE_WRITE) {
    auto mr = ibv_reg_mr(dev->pd(), ptr, size, ib_flags);
    ELOG_INFO<<"ibv_reg_mr regist: "<<mr<<" with pd:"<<dev->pd();
    if (mr != nullptr) [[unlikely]] {
      return ib_buffer_t{mr,std::move(dev)};
    }
    else {
      throw std::make_error_code(std::errc{errno});
    }
  };

  static ib_buffer_t regist(ib_buffer_pool_t& pool, std::shared_ptr<ib_device_t> dev, void* ptr, std::size_t size,
                            int ib_flags = IBV_ACCESS_LOCAL_WRITE |
                                           IBV_ACCESS_REMOTE_READ |
                                           IBV_ACCESS_REMOTE_WRITE);
  void change_owner(std::weak_ptr<ib_buffer_pool_t> owner_pool);
  ib_buffer_t(ib_buffer_t&&) = default;
  ib_buffer_t& operator=(ib_buffer_t&&) = default;
  ~ib_buffer_t();
};


struct ib_buffer_view_t {
private:
  uint32_t lkey_;
  void* address_;
  std::size_t length_;
public:
  ib_buffer_view_t(uint32_t lkey,void* address,std::size_t length):lkey_(lkey),address_(address),length_(length){}
  ib_buffer_view_t(const ib_buffer_t& buffer):lkey_(buffer->lkey),address_(buffer->addr),length_(buffer->length){}
  ib_buffer_view_t(const ib_buffer_t& buffer, std::size_t length):lkey_(buffer->lkey),address_(buffer->addr),length_(length){}
  ib_buffer_view_t(ibv_mr* buffer):lkey_(buffer->lkey),address_(buffer->addr),length_(buffer->length){}
  ib_buffer_view_t(ibv_mr* buffer, void* address, std::size_t length):lkey_(buffer->lkey),address_(address),length_(length){}
  uint32_t lkey() const noexcept {
    return lkey_;
  }
  void* address() const noexcept {
    return address_;
  }
  std::size_t length() const noexcept {
    return length_;
  }
  operator std::span<char>() const noexcept {
    return std::span<char>{(char*)address_, length_};
  }
  ib_buffer_view_t subview(std::size_t start, std::size_t length) const noexcept {
    assert(start+length<length_);
    return ib_buffer_view_t{lkey_,(char*)address_+start,length};
  }
};

class ib_buffer_pool_t: public std::enable_shared_from_this<ib_buffer_pool_t> {
private:
  struct private_construct_token {};
  static async_simple::coro::Lazy<void> collect_idle_timeout_client(
      std::weak_ptr<ib_buffer_pool_t> self_weak,
      std::chrono::milliseconds sleep_time, std::size_t clear_cnt) {
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
        ELOG_TRACE << "start collect timeout client of pool{"
                   << self.get()
                   << "}, now client count: " << self->free_buffers_.size();
        std::size_t is_all_cleared = self->free_buffers_.clear_old(clear_cnt);
        ELOG_TRACE << "finish collect timeout client of pool{"
                   << self.get()
                   << "}, now client cnt: " << self->free_buffers_.size();
        if (is_all_cleared != 0) [[unlikely]] {
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
      if (!self->free_buffers_.collecter_cnt_.compare_exchange_strong(expected, 1))
        break;
    }
    co_return;
  }
  void enqueue(
      ib_buffer_t client) {
    if (free_buffers_.enqueue(std::move(client)) == 1) {
      std::size_t expected = 0;
      if (free_buffers_.collecter_cnt_.compare_exchange_strong(expected, 1)) {
        ELOG_TRACE << "start timeout client collecter of client_pool{"
                   << this << "}";
        collect_idle_timeout_client(
            this->weak_from_this(),
            (std::max)(pool_config_.idle_timeout, std::chrono::milliseconds{50}),
            pool_config_.idle_queue_per_max_clear_count)
            .directlyStart([](auto&&) {
            },coro_io::get_global_executor());
      }
    }
  }
public:
  void collect_free_client(ib_buffer_t buffer) {
    if (buffer) {
      if (free_buffers_.size() < pool_config_.max_buffer_count) {
        ELOG_TRACE << "collect free buffer{data:" << buffer->addr << ",len"<<buffer->length<<"} enqueue";
        buffer.change_owner(weak_from_this());
        enqueue(std::move(buffer));
      }
      else {
        ELOG_TRACE << "out of max connection limit <<"
                   << pool_config_.max_buffer_count << "buffer{data:" << buffer->addr << ",len"<<buffer->length<<"} wont be collect";
      }
    }
    return;
  };
  ib_buffer_t get_buffer() {
    ib_buffer_t buffer;
    free_buffers_.try_dequeue(buffer);
    if (!buffer) {
      ELOG_TRACE << "There is no free buffer. Allocate and regist new buffer now";
      if (pool_config_.max_memory_usage <
          pool_config_.buffer_size +
              total_memory_.load(std::memory_order_acquire)) [[unlikely]] {
        ELOG_TRACE << "Memory out of pool limit. return null buffer";
        return std::move(buffer);
      }
      auto ptr = malloc(pool_config_.buffer_size);
      if (ptr == nullptr) {
        ELOG_TRACE << "Allocate failed.";
        throw std::bad_alloc();
      }
      buffer = ib_buffer_t::regist(*this, device_, ptr, pool_config_.buffer_size);
    }
    ELOG_TRACE << "get buffer{data:" << buffer->addr << ",len"<<buffer->length<<"} from queue";
    return std::move(buffer);
  }
  ib_buffer_t try_get_buffer() {
    ib_buffer_t buffer;
    free_buffers_.try_dequeue(buffer);
    if (buffer) {
      ELOG_TRACE << "get buffer{data:" << buffer->addr << ",len"<<buffer->length<<"} from queue";
    }
    else {
      ELOG_TRACE << "get buffer failed. there is no free buffer";
    }
    return std::move(buffer);
  }
  struct config_t {
    size_t buffer_size = 8 * 1024 * 1024; // 16MB
    size_t max_buffer_count = 64;
    size_t max_memory_usage = 512 * 1024 * 1024; // 512MB
    size_t idle_queue_per_max_clear_count = 1000;
    std::chrono::milliseconds idle_timeout = std::chrono::milliseconds{5000};
  };
  ib_buffer_pool_t(private_construct_token t,std::shared_ptr<ib_device_t> device,const config_t& pool_config):device_(std::move(device)),pool_config_(std::move(pool_config)) {
    
  }
  static std::shared_ptr<ib_buffer_pool_t> create(std::shared_ptr<ib_device_t> device,
      const config_t& pool_config) {
    return std::make_shared<ib_buffer_pool_t>(private_construct_token{},std::move(device), pool_config);
  }
  static std::shared_ptr<ib_buffer_pool_t> create(std::shared_ptr<ib_device_t> device) {
    return std::make_shared<ib_buffer_pool_t>(private_construct_token{},std::move(device),config_t{});
  }
  std::atomic<std::size_t> total_memory_ = 0;
  std::size_t total_memory() const noexcept { return total_memory_.load(std::memory_order_acquire);}
  std::size_t free_client_size() const noexcept { return free_buffers_.size(); }
  coro_io::detail::client_queue<ib_buffer_t> free_buffers_;
  std::shared_ptr<ib_device_t> device_;
  config_t pool_config_;
};

inline ib_buffer_t ib_buffer_t::regist(ib_buffer_pool_t& pool, std::shared_ptr<ib_device_t> dev, void* ptr, std::size_t size,
                            int ib_flags) {
    auto mr = ibv_reg_mr(dev->pd(), ptr, size, ib_flags);
    if (mr != nullptr) [[unlikely]] {
      return ib_buffer_t{mr,std::move(dev),pool.weak_from_this()};
    }
    else {
      throw std::make_error_code(std::errc{errno});
    }
};
inline ib_buffer_t::ib_buffer_t(ibv_mr* mr, std::shared_ptr<ib_device_t> dev, std::weak_ptr<ib_buffer_pool_t> owner_pool) : mr_(mr),dev_(std::move(dev)),owner_pool_(std::move(owner_pool)) {
  if (auto ptr = owner_pool_.lock(); ptr) {
    ptr->total_memory_.fetch_add((*this)->length,std::memory_order_relaxed);
  }
}
inline ib_buffer_t::~ib_buffer_t() {
  if (auto ptr = owner_pool_.lock(); ptr) {
    ptr->total_memory_.fetch_sub((*this)->length,std::memory_order_relaxed);
  }
}
inline void ib_buffer_t::change_owner(std::weak_ptr<ib_buffer_pool_t> owner_pool){
  if (auto ptr = owner_pool_.lock(); ptr) {
    ptr->total_memory_.fetch_sub((*this)->length,std::memory_order_relaxed);
  }
  if (auto ptr = owner_pool.lock(); ptr) {
    ptr->total_memory_.fetch_add((*this)->length,std::memory_order_relaxed);
  }
  owner_pool_=std::move(owner_pool);
}


inline std::shared_ptr<ib_device_t> g_ib_device() {
  static auto dev=std::make_shared<ib_device_t>(ib_config_t{});
  return dev;
}

inline ib_buffer_pool_t& g_ib_buffer_pool(const ib_buffer_pool_t::config_t &pool_config={}) {
  static auto pool = ib_buffer_pool_t::create(g_ib_device(),pool_config);
  return *pool;
}
}  // namespace coro_rdma