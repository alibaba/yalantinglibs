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
#include <asio/io_context.hpp>
#include <fstream>

#include "async_simple/Promise.h"
#include "async_simple/Traits.h"
#include "async_simple/coro/FutureAwaiter.h"
#if defined(ENABLE_FILE_IO_URING)
#include <asio/random_access_file.hpp>
#include <asio/stream_file.hpp>
#endif
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "asio/error.hpp"
#include "async_simple/coro/Lazy.h"
#include "coro_io.hpp"

namespace ylt {
#if defined(ENABLE_FILE_IO_URING)
inline asio::file_base::flags default_flags() {
  return asio::stream_file::read_write | asio::stream_file::append |
         asio::stream_file::create;
}
#endif

class coro_file {
 public:
#if defined(ENABLE_FILE_IO_URING)
  coro_file(asio::io_context::executor_type executor,
            const std::string& filepath,
            asio::file_base::flags flags = default_flags()) {
    try {
      stream_file_ = std::make_unique<asio::stream_file>(executor);
    } catch (std::exception& ex) {
      std::cout << ex.what() << "\n";
      return;
    }

    std::error_code ec;
    stream_file_->open(filepath, flags, ec);
    if (ec) {
      std::cout << ec.message() << "\n";
    }
  }
#else
  coro_file(asio::io_context::executor_type executor,
            const std::string& filepath, bool read = 0)
      : executor_wrapper_(executor) {
    stream_file_ = std::make_unique<std::fstream>(
        filepath,
#ifdef __APPLE__
        std::ios::binary | std::ios::in | std::ios::out);
#else
        std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
#endif
    if (!stream_file_->is_open()) {
      std::cout << "open file " << filepath << " failed "
                << "\n";
      stream_file_.reset();
    }
  }
#endif

  bool is_open() {
#if defined(ENABLE_FILE_IO_URING)
    return stream_file_ && stream_file_->is_open();
#else
    return stream_file_ && stream_file_->is_open();
#endif
  }

  bool eof() { return eof_; }

#if defined(ENABLE_FILE_IO_URING)
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
      char* data, size_t size) {
    size_t left_size = size;
    size_t offset = 0;
    size_t read_total = 0;
    while (left_size) {
      auto [ec, read_size] = co_await asio_util::async_read_some(
          *stream_file_, asio::buffer(data + offset, size - offset));
      if (ec) {
        if (ec == asio::error::eof) {
          eof_ = true;
          co_return std::make_pair(std::error_code{}, read_total);
        }

        co_return std::make_pair(ec, 0);
      }

      if (read_size > size) {
        // if read_size is very large, it means the size if negative, and there
        // is an error occurred.
        co_return std::make_pair(
            std::make_error_code(std::errc::invalid_argument), 0);
      }

      read_total += read_size;

      left_size -= read_size;
      offset += read_size;
      seek_offset_ += read_size;
      std::error_code seek_ec;
      stream_file_->seek(seek_offset_, asio::file_base::seek_basis::seek_set,
                         seek_ec);
      if (seek_ec) {
        co_return std::make_pair(std::make_error_code(std::errc::invalid_seek),
                                 0);
      }
    }

    co_return std::make_pair(std::error_code{}, read_total);
  }

  async_simple::coro::Lazy<std::error_code> async_write(const char* data,
                                                        size_t size) {
    size_t left_size = size;
    size_t offset = 0;
    while (left_size) {
      auto [ec, write_size] = co_await asio_util::async_write_some(
          *stream_file_, asio::buffer(data, size));

      if (ec) {
        co_return ec;
      }

      left_size -= write_size;
      if (left_size == 0) {
        co_return ec;
      }
      offset += write_size;
      std::error_code seek_ec;
      stream_file_->seek(offset, asio::file_base::seek_basis::seek_set,
                         seek_ec);
      if (seek_ec) {
        co_return seek_ec;
      }
    }

    co_return std::error_code{};
  }
#else
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
      char* data, size_t size) {
    async_simple::Promise<std::pair<std::error_code, size_t>> promise;
    async_read_impl(data, size)
        .via(&executor_wrapper_)
        .start([&promise](auto&& t) {
          if (t.available()) {
            promise.setValue(t.value());
          }
          else {
            promise.setValue(std::make_pair(
                std::make_error_code(std::errc::io_error), size_t(0)));
          }
        });

    co_return co_await promise.getFuture();
  }

  async_simple::coro::Lazy<std::error_code> async_write(const char* data,
                                                        size_t size) {
    async_simple::Promise<std::error_code> promise;
    async_write_impl(data, size)
        .via(&executor_wrapper_)
        .start([&promise](auto&& t) {
          if (t.available()) {
            promise.setValue(t.value());
          }
          else {
            promise.setValue(std::make_error_code(std::errc::io_error));
          }
        });

    co_return co_await promise.getFuture();
  }

 private:
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read_impl(
      char* data, size_t size) {
    stream_file_->read(data, size);
    size_t read_size = stream_file_->gcount();
    if (!stream_file_ && read_size == 0) {
      co_return std::make_pair(std::make_error_code(std::errc::io_error), 0);
    }
    eof_ = stream_file_->eof();
    co_return std::make_pair(std::error_code{}, read_size);
  }

  async_simple::coro::Lazy<std::error_code> async_write_impl(const char* data,
                                                             size_t size) {
    stream_file_->write(data, size);
    stream_file_->flush();
    co_return std::error_code{};
  }
#endif

 private:
#if defined(ENABLE_FILE_IO_URING)
  std::unique_ptr<asio::stream_file> stream_file_;
#else
  std::unique_ptr<std::fstream> stream_file_;
  coro_io::ExecutorWrapper<asio::io_context::executor_type> executor_wrapper_;
#endif
  size_t seek_offset_ = 0;
  bool eof_ = false;
};
}  // namespace ylt