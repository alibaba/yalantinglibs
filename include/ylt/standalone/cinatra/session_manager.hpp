#pragma once

#include <array>
#include <asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "secure_string_hash.hpp"
#include "session.hpp"
#include "ylt/coro_io/coro_io.hpp"

namespace cinatra {

#ifndef CINATRA_MAX_SESSION_ID_SIZE
#define CINATRA_MAX_SESSION_ID_SIZE 128
#endif

class session_manager {
 public:
  static session_manager &get() {
    static session_manager instance;
    return instance;
  }

  std::string generate_session_id() {
    const auto sequence =
        static_cast<uint64_t>(id_.fetch_add(1, std::memory_order_relaxed) + 1);
    const auto now = static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::array<uint64_t, 3> input = {
        sequence, now,
        static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(this))};
    static const auto key = detail::make_sip_hash_key();
    auto input_view = std::string_view(
        reinterpret_cast<const char *>(input.data()), sizeof(input));
    const auto first = detail::sip_hash_2_4(input_view, key[0], key[1]);

    input[0] ^= UINT64_C(0x73657373696f6e32);
    input[1] ^= UINT64_C(0x69642d646f6d6169);
    const auto second = detail::sip_hash_2_4(input_view, key[0], key[1]);

    std::string session_id(32, '0');
    write_hex(first, session_id.data());
    write_hex(second, session_id.data() + 16);
    return session_id;
  }

  static bool is_valid_session_id(std::string_view session_id) noexcept {
    return !session_id.empty() &&
           session_id.size() <= CINATRA_MAX_SESSION_ID_SIZE;
  }

  std::shared_ptr<session> find_session(std::string_view session_id) {
    if (!is_valid_session_id(session_id)) {
      return nullptr;
    }

    std::unique_lock<std::mutex> lock(mtx_);
    auto iter = map_.find(session_id);
    return iter == map_.end() ? nullptr : iter->second;
  }

  std::shared_ptr<session> create_session() {
    while (true) {
      auto session_id = generate_session_id();
      auto new_session =
          std::make_shared<session>(session_id, session_timeout_, true);

      std::unique_lock<std::mutex> lock(mtx_);
      auto [iter, inserted] = map_.emplace(session_id, new_session);
      if (inserted) {
        return new_session;
      }
    }
  }

  std::shared_ptr<session> get_session(const std::string &session_id) {
    return find_session(session_id);
  }

  void remove_expire_session() {
    std::unique_lock<std::mutex> lock(mtx_);

    auto now = std::time(nullptr);
    for (auto it = map_.begin(); it != map_.end();) {
      if (it->second->get_time_stamp() <= now)
        it = map_.erase(it);
      else
        ++it;
    }
  }

  bool check_session_existence(const std::string &session_id) {
    if (!is_valid_session_id(session_id)) {
      return false;
    }

    std::unique_lock<std::mutex> lock(mtx_);

    return map_.find(session_id) != map_.end();
  }

  void start_check_session_timer() {
    std::lock_guard lock(timer_mtx_);
    if (stop_timer_ || !check_session_timer_) {
      return;
    }
    std::weak_ptr<asio::steady_timer> timer = check_session_timer_;
    check_session_timer_->expires_after(check_session_duration_);
    check_session_timer_->async_wait([this, timer](auto ec) {
      auto ptr = timer.lock();
      if (ptr == nullptr) {
        return;
      }

      if (ec) {
        return;
      }

      {
        std::lock_guard lock(shutdown_mtx_);
        if (stop_timer_) {
          return;
        }
        ++active_callbacks_;
      }

      remove_expire_session();
      start_check_session_timer();

      {
        std::lock_guard lock(shutdown_mtx_);
        --active_callbacks_;
      }
      shutdown_cv_.notify_all();
    });
  }

  void set_check_session_duration(auto duration) {
    check_session_duration_ = duration;
    {
      std::lock_guard lock(timer_mtx_);
      std::error_code ec;
      check_session_timer_->cancel(ec);
    }

    start_check_session_timer();
  }

  void stop_timer() {
    std::lock_guard lock(shutdown_mtx_);
    stop_timer_ = true;
  }

 private:
  ~session_manager() {
    {
      std::lock_guard lock(timer_mtx_);
      check_session_timer_->cancel();
      check_session_timer_.reset();
    }
    {
      std::lock_guard lock(shutdown_mtx_);
      stop_timer_ = true;
    }
    {
      std::unique_lock lock(shutdown_mtx_);
      shutdown_cv_.wait(lock, [this] {
        return active_callbacks_ == 0;
      });
    }
    std::lock_guard lock(mtx_);
    map_.clear();
  }

  session_manager()
      : check_session_timer_(std::make_shared<asio::steady_timer>(
            coro_io::get_global_executor()->get_asio_executor())) {
    start_check_session_timer();
  };
  session_manager(const session_manager &) = delete;
  session_manager(session_manager &&) = delete;

  static void write_hex(uint64_t value, char *output) noexcept {
    static constexpr char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < 16; ++i) {
      output[15 - i] = hex[value & 0x0f];
      value >>= 4;
    }
  }

  std::atomic_uint64_t id_ = 0;
  std::unordered_map<std::string, std::shared_ptr<session>, secure_string_hash,
                     std::equal_to<>>
      map_;
  std::mutex mtx_;

  // session_timeout_ should be no less than 0
  std::size_t session_timeout_ = 86400;
  std::atomic<bool> stop_timer_ = false;
  std::mutex timer_mtx_;
  std::shared_ptr<asio::steady_timer> check_session_timer_;
  std::atomic<std::chrono::steady_clock::duration> check_session_duration_ = {
      std::chrono::seconds(15)};

  std::mutex shutdown_mtx_;
  std::condition_variable shutdown_cv_;
  int active_callbacks_ = 0;
};

}  // namespace cinatra