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

#include <async_simple/Executor.h>
#include <async_simple/Promise.h>
#include <async_simple/Try.h>
#include <async_simple/Unit.h>
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/Sleep.h>
#include <async_simple/coro/SpinLock.h>

#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <ylt/util/expected.hpp>

#include "coro_io.hpp"
#include "detail/client_queue.hpp"
#include "io_context_pool.hpp"
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
    if (self == nullptr) {
      co_return;
    }
    while (true) {
      auto sleep_time = self->pool_config_.idle_timeout;
      auto clear_cnt = self->pool_config_.idle_queue_per_max_clear_count;
      self->free_clients_.reselect();
      self = nullptr;
      co_await coro_io::sleep_for(sleep_time);
      if ((self = self_weak.lock()) == nullptr) {
        break;
      }
      std::unique_ptr<client_t> client;
      while (true) {
        std::size_t is_all_cleared = self->free_clients_.clear_old(clear_cnt);
        if (is_all_cleared != 0) [[unlikely]] {
          try {
            co_await async_simple::coro::Yield{};
          } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
          }
        }
        else {
          break;
        }
      }
      --self->collecter_cnt_;
      if (self->free_clients_.size() == 0) {
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
      co_await coro_io::sleep_for(pool_config_.reconnect_wait_time);
      ok = (client_t::is_ok(co_await client->reconnect(host_name_)));
    }
    co_return ok ? std::move(client) : nullptr;
  }

  async_simple::coro::Lazy<std::unique_ptr<client_t>> get_client(
      const typename client_t::config& client_config) {
    std::unique_ptr<client_t> client;

    while (true) {
      if (!free_clients_.try_dequeue(client)) {
        break;
      }
      if (!client->has_closed()) {
        break;
      }
    }

    if (client == nullptr) {
      client = std::make_unique<client_t>(*io_context_pool_.get_executor());
      if (!client->init_config(client_config)) {
        co_return nullptr;
      }
      if (client_t::is_ok(co_await client->connect(host_name_))) {
        co_return std::move(client);
      }
      else {
        co_return co_await reconnect(std::move(client));
      }
    }
    else {
      co_return std::move(client);
    }
  }

  void collect_free_client(std::unique_ptr<client_t> client) {
    if (client && free_clients_.size() < pool_config_.max_connection) {
      if (!client->has_closed()) {
        if (free_clients_.enqueue(std::move(client)) == 1) {
          std::size_t expected = 0;
          if (collecter_cnt_.compare_exchange_strong(expected, 1)) {
            collect_idle_timeout_client(this->shared_from_this())
                .via(coro_io::get_global_executor())
                .start([](auto&&) {
                });
          }
        }
      }
    }
    return;
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

 public:
  struct pool_config {
    uint32_t max_connection = 100;
    uint32_t connect_retry_count = 3;
    uint32_t idle_queue_per_max_clear_count = 1000;
    std::chrono::milliseconds reconnect_wait_time{1000};
    std::chrono::milliseconds idle_timeout{30000};
    typename client_t::config client_config;
  };

 private:
  struct private_construct_token {};

 public:
  static std::shared_ptr<client_pool> create(
      std::string_view host_name, const pool_config& pool_config = {},
      io_context_pool_t& io_context_pool = coro_io::g_io_context_pool()) {
    return std::make_shared<client_pool>(private_construct_token{}, host_name,
                                         pool_config, io_context_pool);
  }

  client_pool(private_construct_token t, std::string_view host_name,
              const pool_config& pool_config,
              io_context_pool_t& io_context_pool)
      : host_name_(host_name),
        pool_config_(pool_config),
        io_context_pool_(io_context_pool),
        free_clients_(pool_config.max_connection) {
    if (pool_config_.connect_retry_count == 0) {
      pool_config_.connect_retry_count = 1;
    }
  };

  client_pool(private_construct_token t, client_pools_t* pools_manager_,
              std::string_view host_name, const pool_config& pool_config,
              io_context_pool_t& io_context_pool)
      : pools_manager_(pools_manager_),
        host_name_(host_name),
        pool_config_(pool_config),
        io_context_pool_(io_context_pool),
        free_clients_(pool_config.max_connection) {
    if (pool_config_.connect_retry_count == 0) {
      pool_config_.connect_retry_count = 1;
    }
  };

  template <typename T>
  async_simple::coro::Lazy<return_type<T>> send_request(
      T op, typename client_t::config& client_config) {
    // return type: Lazy<expected<T::returnType,std::errc>>
    auto client = co_await get_client(client_config);
    if (!client) {
      co_return return_type<T>{tl::unexpect, std::errc::connection_refused};
    }
    if constexpr (std::is_same_v<typename return_type<T>::value_type, void>) {
      co_await op(*client);
      collect_free_client(std::move(client));
      co_return return_type<T>{};
    }
    else {
      auto ret = co_await op(*client);
      collect_free_client(std::move(client));
      co_return std::move(ret);
    }
  }

  template <typename T>
  decltype(auto) send_request(T op) {
    return send_request(std::move(op), pool_config_.client_config);
  }

  std::size_t free_client_count() const noexcept {
    return free_clients_.size();
  }

  std::string_view get_host_name() const noexcept { return host_name_; }

 private:
  template <typename, typename>
  friend class client_pools;

  template <typename, typename>
  friend class channel;

  template <typename T>
  async_simple::coro::Lazy<return_type_with_host<T>> send_request(
      T op, std::string_view endpoint,
      typename client_t::config& client_config) {
    // return type: Lazy<expected<T::returnType,std::errc>>
    auto client = co_await get_client(client_config);
    if (!client) {
      co_return return_type_with_host<T>{tl::unexpect,
                                         std::errc::connection_refused};
    }
    if constexpr (std::is_same_v<typename return_type_with_host<T>::value_type,
                                 void>) {
      co_await op(*client, endpoint);
      collect_free_client(std::move(client));
      co_return return_type_with_host<T>{};
    }
    else {
      auto ret = co_await op(*client, endpoint);
      collect_free_client(std::move(client));
      co_return std::move(ret);
    }
  }

  template <typename T>
  decltype(auto) send_request(T op, std::string_view sv) {
    return send_request(std::move(op), sv, pool_config_.client_config);
  }

  coro_io::detail::client_queue<std::unique_ptr<client_t>> free_clients_;
  client_pools_t* pools_manager_ = nullptr;
  async_simple::Promise<async_simple::Unit> idle_timeout_waiter;
  std::string host_name_;
  pool_config pool_config_;
  io_context_pool_t& io_context_pool_;
  std::atomic<std::size_t> collecter_cnt_;
};

template <typename client_t,
          typename io_context_pool_t = coro_io::io_context_pool>
class client_pools {
  using client_pool_t = client_pool<client_t, io_context_pool_t>;

 public:
  client_pools(
      const typename client_pool_t::pool_config& pool_config = {},
      io_context_pool_t& io_context_pool = coro_io::g_io_context_pool())
      : io_context_pool_(io_context_pool), default_pool_config_(pool_config) {}
  auto send_request(std::string_view host_name, auto op)
      -> decltype(std::declval<client_pool_t>().send_request(std::move(op))) {
    auto pool = get_client_pool(host_name, default_pool_config_);
    auto ret = co_await pool->send_request(std::move(op));
    co_return ret;
  }
  auto send_request(std::string_view host_name,
                    typename client_pool_t::pool_config& pool_config, auto op)
      -> decltype(std::declval<client_pool_t>().send_request(std::move(op))) {
    auto pool = get_client_pool(host_name, pool_config);
    auto ret = co_await pool->send_request(std::move(op));
    co_return ret;
  }
  auto at(std::string_view host_name) {
    return get_client_pool(host_name, default_pool_config_);
  }
  auto at(std::string_view host_name,
          const typename client_pool_t::pool_config& pool_config) {
    return get_client_pool(host_name, pool_config);
  }
  auto operator[](std::string_view host_name) { return at(host_name); }
  auto get_io_context_pool() { return io_context_pool_; }

 private:
  std::shared_ptr<client_pool_t> get_client_pool(
      std::string_view host_name,
      const typename client_pool_t::pool_config& pool_config) {
    decltype(client_pool_manager_.end()) iter;
    bool has_inserted;
    {
#ifdef __cpp_lib_generic_unordered_lookup
      std::shared_lock shared_lock{mutex_};
      iter = client_pool_manager_.find(host_name);
#else
      std::string host_name_copy = std::string{host_name};
      std::shared_lock shared_lock{mutex_};
      iter = client_pool_manager_.find(host_name_copy);
#endif
      if (iter == client_pool_manager_.end()) {
        shared_lock.unlock();
        std::lock_guard lock{mutex_};
        std::tie(iter, has_inserted) =
            client_pool_manager_.emplace(host_name, nullptr);
        if (has_inserted) {
          iter->second = std::make_shared<client_pool_t>(
              typename client_pool_t::private_construct_token{}, this,
              host_name, pool_config, io_context_pool_);
        }
      }
      return iter->second;
    }
  }
  struct string_hash {
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(std::string_view str) const {
      return hash_type{}(str);
    }
    std::size_t operator()(std::string const& str) const {
      return hash_type{}(str);
    }
  };
  typename client_pool_t::pool_config default_pool_config_{};
  std::unordered_map<std::string, std::shared_ptr<client_pool_t>, string_hash,
                     std::equal_to<>>
      client_pool_manager_;
  io_context_pool_t& io_context_pool_;
  std::shared_mutex mutex_;
};

template <typename client_t,
          typename io_context_pool_t = coro_io::io_context_pool>
inline client_pools<client_t, io_context_pool_t>& g_clients_pool(
    const typename client_pool<client_t, io_context_pool_t>::pool_config&
        pool_config = {},
    io_context_pool_t& io_context_pool = coro_io::g_io_context_pool()) {
  static client_pools<client_t, io_context_pool_t> _g_clients_pool(
      pool_config, io_context_pool);
  return _g_clients_pool;
}

}  // namespace coro_io
