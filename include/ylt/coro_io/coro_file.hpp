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

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "io_context_pool.hpp"
#if defined(YLT_ENABLE_FILE_IO_URING)
#include <asio/random_access_file.hpp>
#include <asio/stream_file.hpp>
#endif
#include <async_simple/coro/Lazy.h>

#include <asio/error.hpp>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "coro_io.hpp"

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

enum class read_type {
#if defined(YLT_ENABLE_FILE_IO_URING)
  uring,
  uring_random,
#else
  fread,
#endif
#if defined(__GNUC__)
  pread,
#endif
};

class coro_file {
 public:
#if defined(YLT_ENABLE_FILE_IO_URING)
  coro_file(
      coro_io::ExecutorWrapper<>* executor = coro_io::get_global_executor())
      : coro_file(executor->get_asio_executor()) {}

  coro_file(asio::io_context::executor_type executor)
      : executor_wrapper_(executor) {}
#else

  coro_file(coro_io::ExecutorWrapper<>* executor =
                coro_io::get_global_block_executor())
      : coro_file(executor->get_asio_executor()) {}

  coro_file(asio::io_context::executor_type executor)
      : executor_wrapper_(executor) {}
#endif

  bool is_open() { return stream_file_ != nullptr || fd_file_ != nullptr; }

  void flush() {
#if defined(YLT_ENABLE_FILE_IO_URING)

#else
    if (fd_file_) {
#if defined(__GNUC__)
      fsync(*fd_file_);
#endif
    }
    else if (stream_file_) {
      fflush(stream_file_.get());
    }
#endif
  }

  bool eof() { return eof_; }

  void close() {
    if (stream_file_) {
      stream_file_.reset();
    }
    else if (fd_file_) {
      fd_file_.reset();
    }
  }

  static size_t file_size(std::string_view filepath) {
    std::error_code ec;
    size_t size = std::filesystem::file_size(filepath, ec);
    return size;
  }

#if defined(__GNUC__)
  bool open_fd(std::string_view filepath, int open_mode = flags::read_write) {
    if (fd_file_) {
      return true;
    }

    int fd = open(filepath.data(), open_mode);
    if (fd < 0) {
      return false;
    }

    fd_file_ = std::shared_ptr<int>(new int(fd), [](int* ptr) {
      ::close(*ptr);
      delete ptr;
    });
    return true;
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_prw(
      auto io_func, bool is_read, size_t offset, char* buf, size_t size) {
    std::function<int()> func = [=, this] {
      int fd = *fd_file_;
      return io_func(fd, buf, size, offset);
    };

    std::error_code ec{};
    size_t op_size = 0;

    auto len_val = co_await coro_io::post(std::move(func), &executor_wrapper_);
    int len = len_val.value();
    if (len == 0) {
      if (is_read) {
        eof_ = true;
      }
    }
    else if (len > 0) {
      op_size = len;
    }
    else {
      ec = std::make_error_code(std::errc::io_error);
    }

    co_return std::make_pair(ec, op_size);
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_pread(
      size_t offset, char* data, size_t size) {
    co_return co_await async_prw(pread, true, offset, data, size);
  }

  async_simple::coro::Lazy<std::error_code> async_pwrite(size_t offset,
                                                         const char* data,
                                                         size_t size) {
    auto result = co_await async_prw(pwrite, false, offset, (char*)data, size);
    co_return result.first;
  }
#endif

#if defined(YLT_ENABLE_FILE_IO_URING)
  async_simple::coro::Lazy<bool> async_open(std::string_view filepath,
                                            int open_mode = flags::read_write,
                                            read_type type = read_type::uring) {
    type_ = type;
    if (type == read_type::pread) {
      co_return open_fd(filepath, open_mode);
    }

    try {
      if (type == read_type::uring) {
        stream_file_ = std::make_shared<asio::stream_file>(
            executor_wrapper_.get_asio_executor());
      }
      else {
        stream_file_ = std::make_shared<asio::random_access_file>(
            executor_wrapper_.get_asio_executor());
      }
    } catch (std::exception& ex) {
      std::cout << ex.what() << "\n";
      co_return false;
    }

    std::error_code ec;
    stream_file_->open(filepath.data(),
                       static_cast<asio::file_base::flags>(open_mode), ec);

    if (ec) {
      std::cout << ec.message() << "\n";
      co_return false;
    }

    co_return true;
  }

  bool seek(long offset, int whence) {
    if (type_ != read_type::uring) {
      return false;
    }

    assert(stream_file_);
    std::error_code seek_ec;
    reinterpret_cast<asio::stream_file*>(stream_file_.get())
        ->seek(offset, static_cast<asio::file_base::seek_basis>(whence),
               seek_ec);
    if (seek_ec) {
      return false;
    }
    return true;
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
  async_read_some_at(uint64_t offset, char* data, size_t size) {
    assert(stream_file_);

    auto [ec, read_size] = co_await coro_io::async_read_some_at(
        offset,
        *reinterpret_cast<asio::random_access_file*>(stream_file_.get()),
        asio::buffer(data, size));

    if (ec == asio::error::eof) {
      eof_ = true;
      co_return std::make_pair(std::error_code{}, read_size);
    }

    co_return std::make_pair(std::error_code{}, read_size);
  }

  async_simple::coro::Lazy<std::error_code> async_write_some_at(
      uint64_t offset, const char* data, size_t size) {
    assert(stream_file_);

    auto [ec, write_size] = co_await coro_io::async_write_some_at(
        offset,
        *reinterpret_cast<asio::random_access_file*>(stream_file_.get()),
        asio::buffer(data, size));
    co_return ec;
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
      char* data, size_t size) {
    assert(stream_file_);

    auto [ec, read_size] = co_await coro_io::async_read_some(
        *reinterpret_cast<asio::stream_file*>(stream_file_.get()),
        asio::buffer(data, size));
    if (ec == asio::error::eof) {
      eof_ = true;
      co_return std::make_pair(std::error_code{}, read_size);
    }

    co_return std::make_pair(std::error_code{}, read_size);
  }

  async_simple::coro::Lazy<std::error_code> async_write(const char* data,
                                                        size_t size) {
    assert(stream_file_);

    auto [ec, write_size] = co_await coro_io::async_write_some(
        *reinterpret_cast<asio::stream_file*>(stream_file_.get()),
        asio::buffer(data, size));

    co_return ec;
  }
#else
  std::string str_mode(int open_mode) {
    switch (open_mode) {
      case flags::read_only:
        return "r";
      case flags::create_write:
      case flags::write_only:
        return "w";
      case flags::read_write:
        return "r+";
      case flags::append:
        return "a";
      case flags::create_read_write_append:
        return "a+";
      case flags::truncate:
        return "w+";
      default:
        return "r+";
    }
  }

  bool seek(long offset, int whence) {
    assert(fd_file_ == nullptr);

    return fseek(stream_file_.get(), offset, whence) == 0;
  }

  async_simple::coro::Lazy<bool> async_open(std::string filepath,
                                            int open_mode = flags::read_write,
                                            read_type type = read_type::fread) {
#if defined(__GNUC__)
    if (type == read_type::pread) {
      co_return open_fd(filepath, open_mode);
    }
#endif

    if (stream_file_ != nullptr) {
      co_return true;
    }

    auto result = co_await coro_io::post(
        [this, &filepath, open_mode] {
          auto fptr = fopen(filepath.data(), str_mode(open_mode).data());
          if (fptr == nullptr) {
            std::cout << "open file " << filepath << " failed "
                      << "\n";
            return false;
          }
          stream_file_ = std::shared_ptr<FILE>(fptr, [](FILE* ptr) {
            fclose(ptr);
          });
          return true;
        },
        &executor_wrapper_);
    co_return result.value();
  }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
      char* data, size_t size) {
    auto result = co_await coro_io::post(
        [this, data, size] {
          auto fptr = stream_file_.get();
          size_t read_size = fread(data, sizeof(char), size, fptr);
          if (ferror(fptr)) {
            return std::pair<std::error_code, size_t>(
                std::make_error_code(std::errc::io_error), 0);
          }
          eof_ = feof(fptr);
          return std::pair<std::error_code, size_t>(std::error_code{},
                                                    read_size);
        },
        &executor_wrapper_);

    co_return result.value();
  }

  async_simple::coro::Lazy<std::error_code> async_write(const char* data,
                                                        size_t size) {
    auto result = co_await coro_io::post(
        [this, data, size] {
          auto fptr = stream_file_.get();
          fwrite(data, sizeof(char), size, fptr);
          if (ferror(fptr)) {
            return std::make_error_code(std::errc::io_error);
          }
          return std::error_code{};
        },
        &executor_wrapper_);

    co_return result.value();
  }
#endif

 private:
#if defined(YLT_ENABLE_FILE_IO_URING)
  std::shared_ptr<asio::basic_file<>> stream_file_;
  read_type type_ = read_type::uring;
#else
  std::shared_ptr<FILE> stream_file_;
#endif
  coro_io::ExecutorWrapper<> executor_wrapper_;
  std::shared_ptr<int> fd_file_;
  std::atomic<bool> eof_ = false;
};
}  // namespace coro_io
