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
#include <async_simple/Promise.h>
#include <async_simple/Traits.h>
#include <async_simple/coro/FutureAwaiter.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <span>

#include "async_simple/coro/SyncAwait.h"
#include "io_context_pool.hpp"
#if defined(ASIO_HAS_FILE)
#include <asio/random_access_file.hpp>
#include <asio/stream_file.hpp>
#endif
#include <async_simple/coro/Lazy.h>
#include <fcntl.h>

#include <asio/error.hpp>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "coro_io.hpp"
#include "shared_file_handle.hpp"
#if defined(ASIO_WINDOWS)
#include <io.h>
#endif

namespace coro_io {

/*
              ┌─────────────┬───────────────────────────────┐
              │fopen() mode │ open() flags                  │
              ├─────────────┼───────────────────────────────┤
              │     r       │ O_RDONLY                      │
              ├─────────────┼───────────────────────────────┤
              │     w       │ O_WRONLY | O_CREAT | O_TRUNC  │
              ├─────────────┼───────────────────────────────┤
              │     a       │ O_WRONLY | O_CREAT | O_APPEND │
              ├─────────────┼───────────────────────────────┤
              │     r+      │ O_RDWR                        │
              ├─────────────┼───────────────────────────────┤
              │     w+      │ O_RDWR | O_CREAT | O_TRUNC    │
              ├─────────────┼───────────────────────────────┤
              │     a+      │ O_RDWR | O_CREAT | O_APPEND   │
              └─────────────┴───────────────────────────────┘
*/
enum flags {
#if defined(ASIO_WINDOWS)
  read_only = 1,
  write_only = 2,
  read_write = 4,
  append = 8,
  create = 16,
  exclusive = 32,
  truncate = 64,
  create_write = create | write_only,
  create_write_trunc = create | write_only | truncate,
  create_read_write_trunc = read_write | create | truncate,
  create_read_write_append = read_write | create | append,
  sync_all_on_write = 128
#else   // defined(ASIO_WINDOWS)
  read_only = O_RDONLY,
  write_only = O_WRONLY,
  read_write = O_RDWR,
  append = O_APPEND,
  create = O_CREAT,
  exclusive = O_EXCL,
  truncate = O_TRUNC,
  create_write = O_CREAT | O_WRONLY,
  create_write_trunc = O_WRONLY | O_CREAT | O_TRUNC,
  create_read_write_trunc = O_RDWR | O_CREAT | O_TRUNC,
  create_read_write_append = O_RDWR | O_CREAT | O_APPEND,
  sync_all_on_write = O_SYNC
#endif  // defined(ASIO_WINDOWS)
};

constexpr inline flags to_flags(std::ios::ios_base::openmode mode) {
  flags access = flags::read_write;

  if (mode == std::ios::in)
    access = flags::read_only;
  else if (mode == std::ios::out)
    access = flags::write_only;
  else if (mode == std::ios::app)
    access = flags::append;
  else if (mode == std::ios::trunc)
    access = flags::truncate;
  else if (mode == (std::ios::in | std::ios::out))
    access = flags::read_write;
  else if (mode == (std::ios::trunc | std::ios::out))
    access = flags::create_write_trunc;
  if (mode == (std::ios::in | std::ios::out | std::ios::trunc))
    access = create_read_write_trunc;
  else if (mode == (std::ios::in | std::ios::out | std::ios::app))
    access = create_read_write_append;

  return access;
}

#if defined(ASIO_HAS_FILE)
template <bool seq, typename File, typename Executor>
inline bool open_native_async_file(File &file, Executor &executor,
                                   std::string_view filepath, flags open_flags,
                                   bool use_direct_io = false) {
  if (file && file->is_open()) {
    return true;
  }

  try {
    asio::file_base::flags asio_flags =
        static_cast<asio::file_base::flags>(open_flags);

    if (use_direct_io) {
#if defined(ASIO_WINDOWS)
#elif defined(__linux__)
      asio_flags = static_cast<asio::file_base::flags>(
          static_cast<int>(asio_flags) | O_DIRECT);
#endif
    }

    if constexpr (seq) {
      file = std::make_shared<asio::stream_file>(
          executor.get_asio_executor(), std::string(filepath), asio_flags);
    }
    else {
      file = std::make_shared<asio::random_access_file>(
          executor.get_asio_executor(), std::string(filepath), asio_flags);
    }

    // On macOS, use F_NOCACHE as an alternative to O_DIRECT
    if (use_direct_io && file && file->is_open()) {
#if defined(__APPLE__) || defined(__MACH__)
      int fd = file->native_handle();
      if (fd >= 0 && fcntl(fd, F_NOCACHE, 1) != 0) {
        std::error_code ec;
        file->close(ec);
        return false;
      }
#endif
    }
  } catch (std::exception &ex) {
    ELOG_INFO << "line " << __LINE__ << " coro_file open failed" << ex.what()
              << "\n";
    return false;
  }

  return true;
}
#endif

enum class execution_type { none, native_async, thread_pool };

template <execution_type execute_type = execution_type::native_async>
class basic_seq_coro_file {
 public:
  basic_seq_coro_file(coro_io::ExecutorWrapper<> *executor =
                          coro_io::get_global_block_executor())
      : basic_seq_coro_file(executor->get_asio_executor()) {}

  basic_seq_coro_file(asio::io_context::executor_type executor)
      : executor_wrapper_(executor) {}

  basic_seq_coro_file(std::string_view filepath,
                      std::ios::ios_base::openmode open_flags,
                      coro_io::ExecutorWrapper<> *executor =
                          coro_io::get_global_block_executor())
      : basic_seq_coro_file(filepath, open_flags,
                            executor->get_asio_executor()) {}

  basic_seq_coro_file(std::string_view filepath,
                      std::ios::ios_base::openmode open_flags,
                      asio::io_context::executor_type executor)
      : executor_wrapper_(executor) {
    open(filepath, open_flags);
  }

  bool open(std::string_view filepath,
            std::ios::ios_base::openmode open_flags) {
    file_path_ = std::string{filepath};
    if constexpr (execute_type == execution_type::thread_pool) {
      return open_stream_file_in_pool(open_flags);
    }
    else {
#if defined(ASIO_HAS_FILE)
      return open_native_async_file<true>(async_seq_file_, executor_wrapper_,
                                          filepath, to_flags(open_flags));
#else
      return open_stream_file_in_pool(open_flags);
#endif
    }
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
      char *buf, size_t size) {
    if constexpr (execute_type == execution_type::thread_pool) {
      co_return co_await async_read_write({buf, size});
    }
    else {
#if defined(ASIO_HAS_FILE)
      if (async_seq_file_ == nullptr) {
        co_return std::make_pair(
            std::make_error_code(std::errc::invalid_argument), 0);
      }
      auto [ec, read_size] = co_await coro_io::async_read(
          *async_seq_file_, asio::buffer(buf, size));
      if (ec == asio::error::eof) {
        eof_ = true;
        co_return std::make_pair(std::error_code{}, read_size);
      }

      co_return std::make_pair(ec, read_size);
#else
      co_return co_await async_read_write({buf, size});
#endif
    }
  }

  template <bool is_read = true>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read_write(
      std::span<char> buf) {
    auto result = co_await coro_io::post(
        [this, buf]() -> std::pair<std::error_code, size_t> {
          if constexpr (is_read) {
            if (frw_seq_file_.read(buf.data(), buf.size())) {
              return std::make_pair(std::error_code{}, frw_seq_file_.gcount());
            }
          }
          else {
            if (frw_seq_file_.write(buf.data(), buf.size())) {
              return std::make_pair(std::error_code{}, buf.size());
            }
          }

          if (frw_seq_file_.eof()) {
            eof_ = true;
            return std::make_pair(std::error_code{}, frw_seq_file_.gcount());
          }

          return std::make_pair(std::make_error_code(std::errc::io_error), 0);
        },
        &executor_wrapper_);

    co_return result.value();
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_write(
      std::string_view buf) {
    if constexpr (execute_type == execution_type::thread_pool) {
      co_return co_await async_read_write<false>(
          std::span(const_cast<char *>(buf.data()), buf.size()));
    }
    else {
#if defined(ASIO_HAS_FILE)
      if (async_seq_file_ == nullptr) {
        co_return std::make_pair(
            std::make_error_code(std::errc::invalid_argument), 0);
      }
      auto [ec, size] =
          co_await coro_io::async_write(*async_seq_file_, asio::buffer(buf));
      co_return std::make_pair(ec, size);
#else
      co_return co_await async_read_write<false>(
          std::span(const_cast<char *>(buf.data()), buf.size()));
#endif
    }
  }

#if defined(ASIO_HAS_FILE)
  std::shared_ptr<asio::stream_file> get_async_stream_file() {
    return async_seq_file_;
  }
#endif

  std::fstream &get_stream_file() { return frw_seq_file_; }

  bool is_open() {
#if defined(ASIO_HAS_FILE)
    if (async_seq_file_ && async_seq_file_->is_open()) {
      return true;
    }
#endif
    return frw_seq_file_.is_open();
  }

  bool eof() { return eof_; }

  void close() {
#if defined(ASIO_HAS_FILE)
    if (async_seq_file_ && async_seq_file_->is_open()) {
      std::error_code ec;
      async_seq_file_->close(ec);
    }
#endif
    if (frw_seq_file_.is_open()) {
      frw_seq_file_.close();
    }
  }

  bool seek(size_t offset, std::ios_base::seekdir dir) {
#if defined(ASIO_HAS_FILE)
    if (async_seq_file_ && async_seq_file_->is_open()) {
      int whence = SEEK_SET;
      if (dir == std::ios_base::cur)
        whence = SEEK_CUR;
      else if (dir == std::ios_base::end)
        whence = SEEK_END;

      std::error_code seek_ec;
      async_seq_file_->seek(
          offset, static_cast<asio::file_base::seek_basis>(whence), seek_ec);
      if (seek_ec) {
        return false;
      }
      return true;
    }
#endif
    if (frw_seq_file_.is_open()) {
      if (frw_seq_file_.seekg(offset, dir)) {
        return true;
      }
    }

    return false;
  }

  execution_type get_execution_type() {
#if defined(ASIO_HAS_FILE)
    if (async_seq_file_ && async_seq_file_->is_open()) {
      return execution_type::native_async;
    }
#endif
    if (frw_seq_file_.is_open()) {
      return execution_type::thread_pool;
    }

    return execution_type::none;
  }

  size_t file_size(std::error_code &ec) const noexcept {
    return std::filesystem::file_size(file_path_, ec);
  }

  size_t file_size() const { return std::filesystem::file_size(file_path_); }

  std::string_view file_path() const { return file_path_; }

 private:
  bool open_stream_file_in_pool(std::ios::ios_base::openmode flags) {
    if (frw_seq_file_.is_open()) {
      return true;
    }
    auto coro_func = coro_io::post(
        [this, flags]() mutable {
          frw_seq_file_.open(file_path_, flags);
          if (!frw_seq_file_.is_open()) {
            ELOG_INFO << "line " << __LINE__ << " coro_file open failed "
                      << file_path_ << "\n";
            std::cerr << "Error: " << strerror(errno);
            return false;
          }
          return true;
        },
        &executor_wrapper_);
    auto result = async_simple::coro::syncAwait(coro_func);
    return result.value();
  }

  coro_io::ExecutorWrapper<> executor_wrapper_;
#if defined(ASIO_HAS_FILE)
  std::shared_ptr<asio::stream_file> async_seq_file_;  // seq
#endif
  std::fstream frw_seq_file_;  // fread/fwrite seq file
  std::string file_path_;
  bool eof_ = false;
};

using coro_file = basic_seq_coro_file<>;

template <execution_type execute_type = execution_type::native_async>
class basic_random_coro_file {
 private:
  struct file_state {
    explicit file_state(asio::io_context::executor_type file_executor)
        : executor(file_executor) {}

    file_state(shared_file_handle file_handle,
               asio::io_context::executor_type file_executor)
        : handle(std::move(file_handle)), executor(file_executor) {}

    ~file_state() noexcept {
#if defined(ASIO_HAS_FILE)
      // Only release (detach without closing) when a shared handle owns the
      // fd. On Windows native_async there is no shared handle, so let the asio
      // file's destructor CloseHandle the overlapped HANDLE it created itself.
      if (handle.valid() && async_random_file && async_random_file->is_open()) {
        std::error_code ec;
        (void)async_random_file->release(ec);
      }
#endif
    }

    shared_file_handle handle;
    asio::io_context::executor_type executor;
#if defined(ASIO_HAS_FILE)
    std::shared_ptr<asio::random_access_file> async_random_file;
#endif
    std::atomic<bool> eof{false};
  };

#if defined(ASIO_WINDOWS)
  static constexpr bool supports_shared_native_async = false;
#else
  static constexpr bool supports_shared_native_async = true;
#endif

 public:
  basic_random_coro_file(coro_io::ExecutorWrapper<> *executor =
                             coro_io::get_global_block_executor())
      : basic_random_coro_file(executor->get_asio_executor()) {}

  basic_random_coro_file(asio::io_context::executor_type executor)
      : executor_wrapper_(executor) {}

  basic_random_coro_file(std::string_view filepath,
                         std::ios::ios_base::openmode open_flags,
                         coro_io::ExecutorWrapper<> *executor =
                             coro_io::get_global_block_executor())
      : basic_random_coro_file(filepath, open_flags,
                               executor->get_asio_executor()) {}

  basic_random_coro_file(std::string_view filepath,
                         std::ios::ios_base::openmode open_flags,
                         asio::io_context::executor_type executor)
      : executor_wrapper_(executor) {
    open(filepath, open_flags);
  }

  /// Constructs a random-access file that shares ownership of a native file
  /// descriptor. On Windows this overload is unavailable for native_async;
  /// use the path-based overload so Asio can create an overlapped HANDLE.
  template <execution_type type = execute_type,
            std::enable_if_t<type != execution_type::native_async ||
                                 supports_shared_native_async,
                             int> = 0>
  basic_random_coro_file(shared_file_handle handle,
                         coro_io::ExecutorWrapper<> *executor =
                             coro_io::get_global_block_executor(),
                         std::string_view file_path = "")
      : basic_random_coro_file(std::move(handle), executor->get_asio_executor(),
                               file_path) {}

  /// Constructs a shared-handle file using the specified executor. On Windows
  /// shared_file_handle is supported only by the thread_pool backend.
  template <execution_type type = execute_type,
            std::enable_if_t<type != execution_type::native_async ||
                                 supports_shared_native_async,
                             int> = 0>
  basic_random_coro_file(shared_file_handle handle,
                         asio::io_context::executor_type executor,
                         std::string_view file_path = "")
      : executor_wrapper_(executor), file_path_(file_path) {
    initialize_state(std::move(handle));
  }

  bool open(std::string_view filepath, std::ios::ios_base::openmode open_flags,
            bool use_direct_io = false) {
    file_path_ = std::string{filepath};
    if (load_state()) {
      return true;
    }

#if defined(ASIO_WINDOWS) && defined(ASIO_HAS_FILE)
    if constexpr (execute_type == execution_type::native_async) {
      return initialize_native_async_state(filepath, to_flags(open_flags),
                                           use_direct_io);
    }
#endif

    int native_flags = to_flags(open_flags);
#if defined(ASIO_WINDOWS)
    native_flags = adjust_flags(native_flags);
#elif defined(__linux__)
    if (use_direct_io) {
      native_flags |= O_DIRECT;
    }
#endif

    auto [ec, handle] = shared_file_handle::open(filepath, native_flags);
    if (ec) {
      return false;
    }

#if defined(__APPLE__) || defined(__MACH__)
    if (use_direct_io && fcntl(handle.native_handle(), F_NOCACHE, 1) != 0) {
      return false;
    }
#endif

    return initialize_state(std::move(handle));
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read_at(
      uint64_t offset, char *buf, size_t size) {
    auto state = load_state();
    if (!state) {
      co_return bad_file_descriptor_result();
    }

    if constexpr (execute_type == execution_type::thread_pool) {
      co_return co_await async_pread(std::move(state), offset, buf, size);
    }
    else {
#if defined(ASIO_HAS_FILE)
      auto [ec, read_size] = co_await coro_io::async_read_at(
          offset, *state->async_random_file, asio::buffer(buf, size));

      if (ec == asio::error::eof) {
        state->eof.store(true, std::memory_order_relaxed);
        co_return std::make_pair(std::error_code{}, read_size);
      }

      co_return std::make_pair(ec, read_size);
#else
      co_return co_await async_pread(std::move(state), offset, buf, size);
#endif
    }
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_write_at(
      uint64_t offset, std::string_view buf) {
    auto state = load_state();
    if (!state) {
      co_return bad_file_descriptor_result();
    }

    if constexpr (execute_type == execution_type::thread_pool) {
      co_return co_await async_pwrite(std::move(state), offset, buf.data(),
                                      buf.size());
    }
    else {
#if defined(ASIO_HAS_FILE)
      auto [ec, write_size] = co_await coro_io::async_write_at(
          offset, *state->async_random_file, asio::buffer(buf));

      co_return std::make_pair(ec, write_size);
#else
      co_return co_await async_pwrite(std::move(state), offset, buf.data(),
                                      buf.size());
#endif
    }
  }

  bool is_open() {
    auto state = load_state();
    if (!state) {
      return false;
    }

#if defined(ASIO_HAS_FILE)
    if constexpr (execute_type == execution_type::native_async) {
      return state->async_random_file && state->async_random_file->is_open();
    }
#endif
    return state->handle.valid();
  }

  bool eof() {
    auto state = load_state();
    return state && state->eof.load(std::memory_order_relaxed);
  }

  execution_type get_execution_type() {
    auto state = load_state();
    if (!state) {
      return execution_type::none;
    }

#if defined(ASIO_HAS_FILE)
    if constexpr (execute_type == execution_type::native_async) {
      return state->async_random_file && state->async_random_file->is_open()
                 ? execution_type::native_async
                 : execution_type::none;
    }
#endif
    return execution_type::thread_pool;
  }

  void close() noexcept {
#if defined(__cpp_lib_atomic_shared_ptr) && \
    __cpp_lib_atomic_shared_ptr >= 201711L
    (void)state_.exchange({}, std::memory_order_acq_rel);
#else
    (void)std::atomic_exchange_explicit(&state_, std::shared_ptr<file_state>{},
                                        std::memory_order_acq_rel);
#endif
  }

  size_t file_size(std::error_code &ec) const noexcept {
    auto state = load_state();
    if (state && state->handle.valid()) {
      return file_size_from_handle(state->handle.native_handle(), ec);
    }
    return std::filesystem::file_size(file_path_, ec);
  }

  size_t file_size() const {
    auto state = load_state();
    if (!state || !state->handle.valid()) {
      return std::filesystem::file_size(file_path_);
    }

    std::error_code ec;
    auto size = file_size_from_handle(state->handle.native_handle(), ec);
    if (ec) {
      throw std::system_error(ec);
    }
    return size;
  }

  std::string_view file_path() const { return file_path_; }

 private:
  bool initialize_state(shared_file_handle handle) {
    if (!handle.valid()) {
      return false;
    }

    try {
      auto state = std::make_shared<file_state>(
          std::move(handle), executor_wrapper_.get_asio_executor());
#if defined(ASIO_HAS_FILE)
      if constexpr (execute_type == execution_type::native_async) {
#if defined(ASIO_WINDOWS)
        return false;
#else
        state->async_random_file =
            std::make_shared<asio::random_access_file>(state->executor);
        std::error_code ec;
        state->async_random_file->assign(state->handle.native_handle(), ec);
        if (ec) {
          return false;
        }
#endif
      }
#endif
      store_state(std::move(state));
      return true;
    } catch (...) {
      return false;
    }
  }

#if defined(ASIO_WINDOWS) && defined(ASIO_HAS_FILE)
  bool initialize_native_async_state(std::string_view filepath,
                                     flags open_flags, bool use_direct_io) {
    try {
      auto state =
          std::make_shared<file_state>(executor_wrapper_.get_asio_executor());
      if (!open_native_async_file<false>(state->async_random_file,
                                         executor_wrapper_, filepath,
                                         open_flags, use_direct_io)) {
        return false;
      }
      store_state(std::move(state));
      return true;
    } catch (...) {
      return false;
    }
  }
#endif

  void store_state(std::shared_ptr<file_state> state) noexcept {
#if defined(__cpp_lib_atomic_shared_ptr) && \
    __cpp_lib_atomic_shared_ptr >= 201711L
    state_.store(std::move(state), std::memory_order_release);
#else
    std::atomic_store_explicit(&state_, std::move(state),
                               std::memory_order_release);
#endif
  }

  std::shared_ptr<file_state> load_state() const noexcept {
#if defined(__cpp_lib_atomic_shared_ptr) && \
    __cpp_lib_atomic_shared_ptr >= 201711L
    return state_.load(std::memory_order_acquire);
#else
    return std::atomic_load_explicit(&state_, std::memory_order_acquire);
#endif
  }

  static std::pair<std::error_code, size_t> bad_file_descriptor_result() {
    return {std::make_error_code(std::errc::bad_file_descriptor), 0};
  }

  static size_t file_size_from_handle(int fd, std::error_code &ec) noexcept {
#if defined(ASIO_WINDOWS)
    struct _stat64 status;
    int result = ::_fstat64(fd, &status);
#else
    struct stat status;
    int result = ::fstat(fd, &status);
#endif
    if (result != 0) {
      ec = std::error_code(errno, std::generic_category());
      return static_cast<size_t>(-1);
    }
    ec.clear();
    return static_cast<size_t>(status.st_size);
  }

  static async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
  async_pread(std::shared_ptr<file_state> state, size_t offset, char *data,
              size_t size) {
#if defined(ASIO_WINDOWS)
    auto pread = [](int fd, void *buf, uint64_t count,
                    uint64_t offset) -> int64_t {
      DWORD bytes_read = 0;
      OVERLAPPED overlapped;
      memset(&overlapped, 0, sizeof(OVERLAPPED));
      overlapped.Offset = offset & 0xFFFFFFFF;
      overlapped.OffsetHigh = (offset >> 32) & 0xFFFFFFFF;

      BOOL ok = ReadFile(reinterpret_cast<HANDLE>(_get_osfhandle(fd)), buf,
                         count, &bytes_read, &overlapped);
      if (!ok && (errno = GetLastError()) != ERROR_HANDLE_EOF) {
        return -1;
      }

      return bytes_read;
    };
#else
    auto pread = [](int fd, void *buf, uint64_t count,
                    uint64_t offset) -> int64_t {
      return ::pread(fd, buf, count, offset);
    };
#endif
    co_return co_await async_prw(std::move(state), pread, true, offset, data,
                                 size);
  }

  static async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
  async_pwrite(std::shared_ptr<file_state> state, size_t offset,
               const char *data, size_t size) {
#if defined(ASIO_WINDOWS)
    auto pwrite = [](int fd, const void *buf, uint64_t count,
                     uint64_t offset) -> int64_t {
      DWORD bytes_write = 0;
      OVERLAPPED overlapped;
      memset(&overlapped, 0, sizeof(OVERLAPPED));
      overlapped.Offset = offset & 0xFFFFFFFF;
      overlapped.OffsetHigh = (offset >> 32) & 0xFFFFFFFF;

      BOOL ok = WriteFile(reinterpret_cast<HANDLE>(_get_osfhandle(fd)), buf,
                          count, &bytes_write, &overlapped);
      if (!ok) {
        return -1;
      }

      return bytes_write;
    };
#else
    auto pwrite = [](int fd, const void *buf, uint64_t count,
                     uint64_t offset) -> int64_t {
      return ::pwrite(fd, buf, count, offset);
    };
#endif
    co_return co_await async_prw(std::move(state), pwrite, false, offset,
                                 const_cast<char *>(data), size);
  }

  static async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_prw(
      std::shared_ptr<file_state> state, auto io_func, bool is_read,
      size_t offset, char *buf, size_t size) {
    auto executor = state->executor;
    std::function<std::pair<std::error_code, size_t>()> operation =
        [state, io_func, offset, buf, size]() {
          auto length =
              io_func(state->handle.native_handle(), buf, size, offset);
          if (length < 0) {
            return std::make_pair(std::make_error_code(std::errc::io_error),
                                  size_t{0});
          }
          return std::make_pair(std::error_code{}, static_cast<size_t>(length));
        };
    auto result =
        co_await coro_io::post(std::move(operation), std::move(executor));

    auto operation_result = result.value();
    if (is_read && !operation_result.first && operation_result.second == 0) {
      state->eof.store(true, std::memory_order_relaxed);
    }
    co_return operation_result;
  }

#if defined(ASIO_WINDOWS)
  static int adjust_flags(int open_mode) {
    switch (open_mode) {
      case flags::read_only:
        return _O_RDONLY;
      case flags::write_only:
        return _O_WRONLY;
      case flags::read_write:
        return _O_RDWR;
      case flags::append:
        return _O_APPEND;
      case flags::create:
        return _O_CREAT;
      case flags::exclusive:
        return _O_EXCL;
      case flags::truncate:
        return _O_TRUNC;
      case flags::create_write:
        return _O_CREAT | _O_WRONLY;
      case flags::create_write_trunc:
        return _O_CREAT | _O_WRONLY | _O_TRUNC;
      case flags::create_read_write_trunc:
        return _O_RDWR | _O_CREAT | _O_TRUNC;
      case flags::create_read_write_append:
        return _O_RDWR | _O_CREAT | _O_APPEND;
      case flags::sync_all_on_write:
      default:
        return open_mode;
        break;
    }
    return open_mode;
  }
#endif

  coro_io::ExecutorWrapper<> executor_wrapper_;
#if defined(__cpp_lib_atomic_shared_ptr) && \
    __cpp_lib_atomic_shared_ptr >= 201711L
  std::atomic<std::shared_ptr<file_state>> state_;
#else
  std::shared_ptr<file_state> state_;
#endif
  std::string file_path_;
};

using random_coro_file = basic_random_coro_file<>;
}  // namespace coro_io
