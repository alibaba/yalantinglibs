#pragma once
#include <cstddef>
#include <exception>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include "async_simple/Try.h"
#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_io/io_context_pool.hpp"
#if __has_include("ylt/coro_io/coro_io.hpp")
#include "ylt/coro_io/coro_io.hpp"
#else
#include "cinatra/ylt/coro_io/coro_io.hpp"
#endif

namespace ylt::thread {
inline thread_local size_t thd_index_;

template <typename T>
class thread_local_t {
 public:
  thread_local_t() { create_key(); }

  ~thread_local_t() { destroy_key(); }

  thread_local_t(thread_local_t &&other) noexcept {
    tls_key_ = other.tls_key_;
    thrd_locals_ = std::move(other.thrd_locals_);
    other.tls_key_ = 0;
  }

  thread_local_t &operator=(thread_local_t &&other) noexcept {
    if (this != &other) {
      destroy_key();
      tls_key_ = other.tls_key_;
      thrd_locals_ = std::move(other.thrd_locals_);
      other.tls_key_ = 0;
    }
    return *this;
  }

  thread_local_t(const thread_local_t &other) = delete;
  thread_local_t &operator=(const thread_local_t &other) = delete;

  void create_tls() {
    auto t = std::make_unique<T>();
    auto ptr = t.get();
    set_tls(ptr);
    thrd_locals_.push_back(std::move(t));
  }

  T &value() {
    static thread_local size_t index = get_rr();
    return *thrd_locals_.at(index);
  }

  template <typename F>
  void for_each(F &&fn) {
    for (auto &ptr : thrd_locals_) {
      fn(*ptr);
    }
  }

  size_t tls_count() const { return thrd_locals_.size(); }

 private:
#if _WIN32 || _WIN64
  using tls_key_t = DWORD;
  void create_key() { tls_key_ = TlsAlloc(); }
  void destroy_key() { TlsFree(tls_key_); }
  void set_tls(void *value) { TlsSetValue(tls_key_, (LPVOID)value); }
  void *get_tls() { return (void *)TlsGetValue(tls_key_); }
#else
  using tls_key_t = pthread_key_t;
  void create_key() { pthread_key_create(&tls_key_, nullptr); }
  void destroy_key() { pthread_key_delete(tls_key_); }
  void set_tls(void *value) const { pthread_setspecific(tls_key_, value); }
  void *get_tls() const { return pthread_getspecific(tls_key_); }
#endif
  size_t get_rr() {
    static std::atomic<size_t> round = 0;
    return round++ % thrd_locals_.size();
  }

  tls_key_t tls_key_;
  std::vector<std::unique_ptr<T>> thrd_locals_;
};

template <typename T>
inline async_simple::coro::Lazy<void> init_thread_locals_impl(
    std::vector<thread_local_t<T> *> &tls_list, size_t thd_num) {
  std::vector<async_simple::coro::Lazy<async_simple::Try<void>>> vec;
  for (size_t j = 0; j < thd_num; j++) {
    auto executor = coro_io::get_global_block_executor();
    vec.push_back(coro_io::post(
        [&tls_list, j] {
          for (auto &t : tls_list) {
            t->create_tls();
          }
        },
        executor));
  }

  co_await async_simple::coro::collectAll(std::move(vec));
}

constexpr inline size_t tls_keys_max() {
#if _WIN32 || _WIN64
  return TLS_MINIMUM_AVAILABLE;
#else
  return PTHREAD_KEYS_MAX;
#endif
}

template <typename T>
inline void init_thread_locals(
    std::vector<thread_local_t<T> *> &tls_list,
    size_t thd_num = std::thread::hardware_concurrency()) {
  if (tls_list.size() > tls_keys_max()) {
    throw std::out_of_range("exceed the max number of tls keys");
  }
  coro_io::g_block_io_context_pool(thd_num);
  async_simple::coro::syncAwait(init_thread_locals_impl<T>(tls_list, thd_num));
}
}  // namespace ylt::thread