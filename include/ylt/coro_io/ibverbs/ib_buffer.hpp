#pragma once

#include <infiniband/verbs.h>
#include <sys/types.h>

#include <cerrno>
#include <cstddef>
#include <memory>
#include <new>
#include <system_error>

#include "async_simple/coro/Lazy.h"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/easylog.hpp"
#include "ylt/coro_io/detail/client_queue.hpp"

#include "ib_device.hpp"

namespace coro_io {

using mr_buffer_base_t = std::unique_ptr<ibv_mr, ib_deleter>;
struct mr_buffer_t : public mr_buffer_base_t {
  using mr_buffer_base_t::mr_buffer_base_t;

 private:
  mr_buffer_t(mr_buffer_base_t&& o) noexcept : mr_buffer_base_t(std::move(o)) {}

 public:
  static mr_buffer_t regist(ibv_pd* pd, void* ptr, uint64_t size,
                            int mr_flags = IBV_ACCESS_LOCAL_WRITE |
                                           IBV_ACCESS_REMOTE_READ |
                                           IBV_ACCESS_REMOTE_WRITE) {
    auto mr = ibv_reg_mr(pd, ptr, size, mr_flags);
    if (mr != nullptr) [[unlikely]] {
      auto i = mr_buffer_base_t{mr};
      return mr_buffer_t{std::move(i)};
    }
    else {
      throw std::make_error_code(std::errc{errno});
    }
  };
};
class mr_buffer_pool_t: public std::enable_shared_from_this<mr_buffer_pool_t> {
private:
  struct private_construct_token {};
  static async_simple::coro::Lazy<void> collect_idle_timeout_client(
      std::weak_ptr<mr_buffer_pool_t> self_weak,
      coro_io::detail::client_queue<mr_buffer_t>& clients,
      std::chrono::milliseconds sleep_time, std::size_t clear_cnt) {
    std::shared_ptr<mr_buffer_pool_t> self = self_weak.lock();
    if (self == nullptr) {
      co_return;
    }
    while (true) {
      clients.reselect();
      self = nullptr;
      co_await coro_io::sleep_for(sleep_time);
      if ((self = self_weak.lock()) == nullptr) {
        break;
      }
      while (true) {
        ELOG_TRACE << "start collect timeout client of pool{"
                   << self.get()
                   << "}, now client count: " << clients.size();
        std::size_t is_all_cleared = clients.clear_old(clear_cnt);
        ELOG_TRACE << "finish collect timeout client of pool{"
                   << self.get()
                   << "}, now client cnt: " << clients.size();
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
      --clients.collecter_cnt_;
      if (clients.size() == 0) {
        break;
      }
      std::size_t expected = 0;
      if (!clients.collecter_cnt_.compare_exchange_strong(expected, 1))
        break;
    }
    co_return;
  }
  void enqueue(
      mr_buffer_t client) {
    if (free_buffers_.enqueue(std::move(client)) == 1) {
      std::size_t expected = 0;
      if (free_buffers_.collecter_cnt_.compare_exchange_strong(expected, 1)) {
        ELOG_TRACE << "start timeout client collecter of client_pool{"
                   << *this << "}";
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
  void collect_free_client(mr_buffer_t buffer) {
    if (buffer) {
      if (free_buffers_.size() < pool_config_.max_buffer_count) {
        ELOG_TRACE << "collect free buffer{data:" << buffer->addr << ",len"<<buffer->length<<"} enqueue";
        enqueue(std::move(buffer));
      }
      else {
        ELOG_TRACE << "out of max connection limit <<"
                   << pool_config_.max_buffer_count << "buffer{data:" << buffer->addr << ",len"<<buffer->length<<"} wont be collect";
      }
    }
    return;
  };
  mr_buffer_t get_buffer() {
    mr_buffer_t buffer;
    free_buffers_.try_dequeue(buffer);
    if (!buffer) {
      ELOG_TRACE << "There is no free buffer. Allocate and regist new buffer now";
      auto ptr = malloc(pool_config_.buffer_size);
      if (ptr == nullptr) {
        ELOG_TRACE << "Allocate failed.";
        throw std::bad_alloc();
      }
      buffer = mr_buffer_t::regist(device_->pd(), ptr, pool_config_.buffer_size);
    }
    ELOG_TRACE << "get buffer{data:" << buffer->addr << ",len"<<buffer->length<<"} from queue";
    return std::move(buffer);
  }
  mr_buffer_t try_get_buffer() {
    mr_buffer_t buffer;
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
    size_t buffer_size = 16 * 1024 * 1024; // 16MB
    size_t max_buffer_count = 64;
    size_t idle_queue_per_max_clear_count = 1000;
    std::chrono::milliseconds idle_timeout = std::chrono::milliseconds{5000};
  };
  mr_buffer_pool_t(private_construct_token t,std::shared_ptr<ib_device_t> device,const config_t& pool_config):device_(std::move(device)),pool_config_(std::move(pool_config)) {
    
  }
  static std::shared_ptr<mr_buffer_pool_t> create(std::shared_ptr<ib_device_t> device,
      const config_t& pool_config={}) {
    return std::make_shared<mr_buffer_pool_t>(private_construct_token{},std::move(device), pool_config);
  }
  std::size_t size() const noexcept { return free_buffers_.size(); }
  coro_io::detail::client_queue<mr_buffer_t> free_buffers_;
  std::shared_ptr<ib_device_t> device_;
  config_t pool_config_;
};
}  // namespace coro_rdma