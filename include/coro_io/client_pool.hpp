/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "asio/io_context.hpp"
#include "asio/steady_timer.hpp"
#include "async_simple/Executor.h"
#include "async_simple/Promise.h"
#include "async_simple/Try.h"
#include "async_simple/Unit.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include "async_simple/coro/SpinLock.h"
#include "coro_io/coro_io.hpp"
#include "coro_io/io_context_pool.hpp"
#include "util/concurrentqueue.h"
#include "util/expected.hpp"
namespace coro_io {

template <typename client_t, typename io_context_pool_t>
class client_pools;

template <typename, typename>
class channel;

template <typename client_t,
          typename io_context_pool_t = coro_io::io_context_pool>
class client_pool : public std::enable_shared_from_this<
                        client_pool<client_t, io_context_pool_t>> {
  using client_pools_t = client_pools<client_t, io_context_pool_t>;
  static async_simple::coro::Lazy<void> collect_idle_timeout_client(
      std::weak_ptr<client_pool> self_weak) {
    std::shared_ptr<client_pool> self = self_weak.lock();
    while (true) {
      auto sleep_time = self->pool_config_.idle_timeout_;
      self = nullptr;
      co_await async_simple::coro::sleep(sleep_time);
      if ((self = self_weak.lock()) == nullptr) {
        break;
      }
      auto timeout_point =
          std::chrono::steady_clock::now() - self->pool_config_.idle_timeout_;
      client_info_t t;
      int cnt = 0;
      while (true) {
        bool is_not_empty = self->free_clients_.try_dequeue(t);
        if (!is_not_empty) {
          break;
        }
        ++cnt;
        if (self->free_clients_.size_approx() <
                self->pool_config_.max_connection_ &&
            t.second > timeout_point) {
          --cnt;
          self->free_clients_.enqueue(std::move(t));
          break;
        }
        --self->queue_size_;
        if (cnt % 10000 == 0) {
          co_await async_simple::coro::Yield{};
        }
      }

      --self->collecter_cnt_;
      if (self->queue_size_ == 0) {
        break;
      }
      std::size_t expected = 0;
      if (!self->collecter_cnt_.compare_exchange_strong(expected, 1))
        break;
    }
    co_return;
  }

  async_simple::coro::Lazy<std::unique_ptr<client_t>> reconnect(
      std::unique_ptr<client_t> client) {
    bool ok = false;

    for (int i = 0; !ok && i < pool_config_.connect_retry_count; ++i) {
      ok = (client_t::is_ok(co_await client->connect(host_name_)));
      if (!ok) {
        auto executor = co_await async_simple::CurrentExecutor();
        if (executor == nullptr) {
          coro_io::period_timer reconnect_timer(
              io_context_pool_.get_executor());
          reconnect_timer.expires_after(rand_wait_time());
          co_await reconnect_timer.async_await();
        }
        else {
          co_await async_simple::coro::sleep(rand_wait_time());
        }
      }
    }
    co_return ok ? std::move(client) : nullptr;
  }

  async_simple::coro::Lazy<std::unique_ptr<client_t>> get_client(
      const typename client_t::config& client_config) {
    std::unique_ptr<client_t> client;

    while (true) {
      {
        client_info_t info;
        if (free_clients_.try_dequeue(info)) {
          client = std::move(info.first);
          --queue_size_;
        }
        else {
          break;
        }
      }

      if (client->has_closed()) {
        [this](std::unique_ptr<client_t> client)
            -> async_simple::coro::Lazy<void> {
          co_await collect_free_client(co_await reconnect(std::move(client)));
        }(std::move(client))
                   .start([](auto&&) {
                   });
      }
      else {
        break;
      }
    }

    if (client == nullptr) {
      client = std::make_unique<client_t>(*io_context_pool_.get_executor());
      if (!client->init_config(client_config)) {
        co_return nullptr;
      }
      bool ok;
      if constexpr (requires { client->async_connect(host_name_); }) {
        ok = (client_t::is_ok(co_await client->async_connect(host_name_)));
      }
      else {
        ok = (client_t::is_ok(co_await client->connect(host_name_)));
      }
      if (ok) {
        co_return std::move(client);
      }
      else {
        co_return co_await reconnect(std::move(client));
      }
    }
    else {
      if (client->has_closed()) {
        co_return co_await reconnect(std::move(client));
      }
      else {
        co_return std::move(client);
      }
    }
  }

  std::chrono::microseconds rand_wait_time() {
    using namespace std::chrono_literals;
    return rand() %
               (pool_config_.reconnect_wait_random_duration + 1ms).count() *
               1ms +
           pool_config_.reconnect_wait_min_time;
  }

  async_simple::coro::Lazy<void> collect_free_client(
      std::unique_ptr<client_t> client) {
    if (client && queue_size_ < pool_config_.max_connection_) {
      auto time_point = std::chrono::steady_clock::now();
      if (!client->has_closed()) {
        free_clients_.enqueue(client_info_t{std::move(client), time_point});
        if (queue_size_++ == 0) {
          std::size_t expected = 0;
          if (collecter_cnt_.compare_exchange_strong(expected, 1)) {
            collect_idle_timeout_client(this->shared_from_this())
                .via(idle_timeout_executor)
                .start([](auto&&) {
                });
          }
        }
      }
    }
    co_return;
  };

  template <typename T>
  struct lazy_hacker {
    using type = void;
  };
  template <typename T>
  struct lazy_hacker<async_simple::coro::Lazy<T>> {
    using type = T;
  };
  template <typename T>
  using return_type =
      tl::expected<typename lazy_hacker<decltype(std::declval<T>()(
                       std::declval<client_t&>()))>::type,
                   std::errc>;

  template <typename T>
  using return_type_with_host =
      tl::expected<typename lazy_hacker<decltype(std::declval<T>()(
                       std::declval<client_t&>(), std::string_view{}))>::type,
                   std::errc>;

  constexpr static int BLOCK_SIZE = 32;

 public:
  struct pool_config {
    uint32_t max_connection_ = 100;
    uint32_t connect_retry_count = 5;
    std::chrono::milliseconds reconnect_wait_min_time{1000};
    std::chrono::milliseconds reconnect_wait_random_duration{5000};
    std::chrono::milliseconds idle_timeout_{5000};
    typename client_t::config client_config;
  };

 private:
  struct private_construct_token {};

 public:
  static std::shared_ptr<client_pool> create(
      const std::string& host_name, const pool_config& pool_config = {},
      io_context_pool_t& io_context_pool = coro_io::g_io_context_pool()) {
    return std::make_shared<client_pool>(private_construct_token{}, host_name,
                                         pool_config, io_context_pool);
  }

  client_pool(private_construct_token t, const std::string& host_name,
              const pool_config& pool_config,
              io_context_pool_t& io_context_pool)
      : host_name_(host_name),
        pool_config_(pool_config),
        io_context_pool_(io_context_pool),
        free_clients_(pool_config.max_connection_),
        idle_timeout_executor(io_context_pool.get_executor()) {
    if (pool_config_.connect_retry_count == 0) {
      pool_config_.connect_retry_count = 1;
    }
  };

  client_pool(private_construct_token t, client_pools_t* pools_manager_,
              const std::string& host_name, const pool_config& pool_config,
              io_context_pool_t& io_context_pool)
      : pools_manager_(pools_manager_),
        host_name_(host_name),
        pool_config_(pool_config),
        io_context_pool_(io_context_pool),
        free_clients_(pool_config.max_connection_),
        idle_timeout_executor(io_context_pool.get_executor()) {
    if (pool_config_.connect_retry_count == 0) {
      pool_config_.connect_retry_count = 1;
    }
  };

  template <typename T>
  async_simple::coro::Lazy<return_type<T>> send_request(
      T&& op, const typename client_t::config& client_config) {
    // return type: Lazy<expected<T::returnType,std::errc>>
    auto client = co_await get_client(client_config);
    if (!client) {
      co_return return_type<T>{tl::unexpect, std::errc::connection_refused};
    }
    if constexpr (std::is_same_v<typename return_type<T>::value_type, void>) {
      co_await op(*client);
      co_await collect_free_client(std::move(client));
      co_return return_type<T>{};
    }
    else {
      auto ret = co_await op(*client);
      co_await collect_free_client(std::move(client));
      co_return std::move(ret);
    }
  }

  template <typename T>
  decltype(auto) send_request(T&& op) {
    return send_request(op, pool_config_.client_config);
  }

  std::size_t free_client_count() const noexcept { return queue_size_; }

  std::string_view get_host_name() const noexcept { return host_name_; }

 private:
  template <typename, typename>
  friend class client_pools;

  template <typename, typename>
  friend class channel;

  template <typename T>
  async_simple::coro::Lazy<return_type_with_host<T>> send_request(
      T&& op, std::string_view endpoint,
      const typename client_t::config& client_config) {
    // return type: Lazy<expected<T::returnType,std::errc>>
    auto client = co_await get_client(client_config);
    if (!client) {
      co_return return_type_with_host<T>{tl::unexpect,
                                         std::errc::connection_refused};
    }
    if constexpr (std::is_same_v<typename return_type_with_host<T>::value_type,
                                 void>) {
      co_await op(*client, endpoint);
      co_await collect_free_client(std::move(client));
      co_return return_type_with_host<T>{};
    }
    else {
      auto ret = co_await op(*client, endpoint);
      co_await collect_free_client(std::move(client));
      co_return std::move(ret);
    }
  }

  template <typename T>
  decltype(auto) send_request(T&& op, std::string_view sv) {
    return send_request(op, sv, pool_config_.client_config);
  }

  using client_info_t = std::pair<std::unique_ptr<client_t>,
                                  std::chrono::steady_clock::time_point>;

  client_pools_t* pools_manager_ = nullptr;
  coro_io::ExecutorWrapper<>* idle_timeout_executor;
  async_simple::Promise<async_simple::Unit> idle_timeout_waiter;
  std::string host_name_;
  moodycamel::ConcurrentQueue<client_info_t> free_clients_;
  pool_config pool_config_;
  io_context_pool_t& io_context_pool_;
  std::atomic<std::size_t> queue_size_;
  std::atomic<std::size_t> collecter_cnt_;
};

template <typename client_t,
          typename io_context_pool_t = coro_io::io_context_pool>
class client_pools {
  using client_pool_t = client_pool<client_t, io_context_pool_t>;

 public:
  client_pools(
      const typename client_pool_t::pool_config& pool_config = {},
      const std::vector<std::string>& fixed_host_list = {},
      io_context_pool_t& io_context_pool = coro_io::g_io_context_pool())
      : io_context_pool_(io_context_pool),
        default_pool_config_(pool_config),
        is_pool_fixed(fixed_host_list.size()) {
    for (auto& e : fixed_host_list) {
      client_pool_manager_.emplace(
          e, std::make_shared<client_pool_t>(
                 typename client_pool_t::private_construct_token{}, this,
                 std::string{e}, default_pool_config_, io_context_pool_));
    }
  }
  auto send_request(const std::string& host_name, auto&& op)
      -> decltype(std::declval<client_pool_t>().send_request(op)) {
    auto pool = co_await get_client_pool(host_name, default_pool_config_);
    auto ret = co_await pool->send_request(op);
    co_return ret;
  }
  auto send_request(const std::string& host_name,
                    const typename client_pool_t::pool_config& pool_config,
                    auto&& op)
      -> decltype(std::declval<client_pool_t>().send_request(op)) {
    auto pool = co_await get_client_pool(host_name, pool_config);
    auto ret = co_await pool.send_request(op);
    co_return ret;
  }
  auto at(const std::string& host_name) {
    return get_client_pool(host_name, default_pool_config_);
  }
  auto at(const std::string& host_name,
          const typename client_pool_t::pool_config& pool_config) {
    return get_client_pool(host_name, pool_config);
  }
  auto operator[](const std::string& host_name) { return at(host_name); }
  auto get_io_context_pool() { return io_context_pool_; }

 private:
  async_simple::coro::Lazy<std::shared_ptr<client_pool_t>> get_client_pool(
      const std::string& host_name,
      const typename client_pool_t::pool_config& pool_config) {
    if (is_pool_fixed) {
      auto iter = client_pool_manager_.find(host_name);
      assert(iter != client_pool_manager_.end());
      if (iter != client_pool_manager_.end()) {
        co_return iter->second;
      }
      else {
        co_return nullptr;
      }
    }
    else {
      auto scoper = co_await mutex.coScopedLock();
      auto&& [iter, has_inserted] =
          client_pool_manager_.emplace(host_name, nullptr);
      if (has_inserted) {
        iter->second = std::make_shared<client_pool_t>(
            typename client_pool_t::private_construct_token{}, this, host_name,
            pool_config, io_context_pool_);
      }
      co_return iter->second;
    }
  }
  typename client_pool_t::pool_config default_pool_config_{};
  std::unordered_map<std::string, std::shared_ptr<client_pool_t>>
      client_pool_manager_;
  io_context_pool_t& io_context_pool_;
  async_simple::coro::SpinLock mutex;
  bool is_pool_fixed;
};

template <typename client_t,
          typename io_context_pool_t = coro_io::io_context_pool>
inline client_pools<client_t, io_context_pool_t>& g_clients_pool(
    const typename client_pool<client_t, io_context_pool_t>::pool_config&
        pool_config = {},
    const std::vector<std::string>& fixed_host_list = {},
    io_context_pool_t& io_context_pool = coro_io::g_io_context_pool()) {
  static client_pools<client_t, io_context_pool_t> _g_clients_pool(
      pool_config, fixed_host_list, io_context_pool);
  return _g_clients_pool;
}

}  // namespace coro_io
