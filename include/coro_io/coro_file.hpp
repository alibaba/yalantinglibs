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
#include <asio/random_access_file.hpp>
#include <asio/stream_file.hpp>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "asio/file_base.hpp"
#include "asio_coro_util.hpp"
#include "async_simple/coro/Lazy.h"

namespace ylt {
#ifdef ASIO_HAS_LIB_AIO
template <typename T, size_t alignment>
class aligned_allocator {
 public:
  using value_type = T;
  template <typename U>
  struct rebind {
    using other = aligned_allocator<U, alignment>;
  };
  inline value_type* allocate(size_t size) const {
    value_type* ptr;
    auto ret = posix_memalign((void**)&ptr, alignment, sizeof(T) * size);
    if (ret != 0)
      throw std::bad_alloc();
    return ptr;
  };

  inline void deallocate(value_type* const ptr, size_t n) const noexcept {
    free(ptr);
  }
};
#endif

inline asio::file_base::flags write_flags() {
#if defined(ASIO_HAS_LIB_AIO)
  return asio::stream_file::direct | asio::stream_file::read_write;
#else
  return asio::stream_file::write_only | asio::stream_file::create;
#endif
}

class coro_file {
 public:
  coro_file(asio::io_context::executor_type executor,
            const std::string& filepath)
      : filepath_(filepath) {
    try {
      stream_file_ = std::make_unique<asio::stream_file>(executor);
    } catch (std::exception& ex) {
      std::cout << ex.what() << "\n";
      return;
    }

    buf_.resize(buf_size_);
#ifdef ASIO_HAS_LIB_AIO
    write_buf_.resize(buf_size_);
#endif
  }

  std::error_code open_file(size_t write_size = 0) {
    asio::file_base::flags flags =
        write_size ? write_flags() : asio::file_base::flags::read_only;
    std::error_code ec;
#ifdef ASIO_HAS_LIB_AIO
    if (write_size > 0 && write_size < 512) {
      flags = asio::stream_file::write_only | asio::stream_file::create;
    }
#endif

    stream_file_->open(filepath_, flags, ec);
    if (ec) {
      std::cout << ec.message() << "\n";
    }

    return ec;
  }

  bool eof() { return eof_; }

  async_simple::coro::Lazy<std::pair<std::error_code, std::string_view>>
  async_read() {
    if (!stream_file_->is_open()) {
      if (auto ec = open_file(); ec) {
        co_return std::make_pair(ec, "");
      }
    }
    auto [ec, size] = co_await asio_util::async_read_some(
        *stream_file_, asio::buffer(buf_.data(), buf_.size()));
    if (!ec) {
      if (size > buf_size_) {
        co_return std::make_pair(
            std::make_error_code(std::errc::invalid_argument), "");
      }
      read_total_ += size;
      eof_ = (size < buf_size_);
    }
    std::error_code seek_ec;
    stream_file_->seek(read_total_, asio::file_base::seek_basis::seek_set,
                       seek_ec);
    if (seek_ec) {
      std::cout << seek_ec.message() << "\n";
      co_return std::make_pair(std::make_error_code(std::errc::invalid_seek),
                               "");
    }
    co_return std::make_pair(ec, std::string_view(buf_.data(), size));
  }

  async_simple::coro::Lazy<std::error_code> async_write(const char* data,
                                                        size_t size) {
    if (!stream_file_->is_open()) {
      if (auto ec = open_file(size); ec) {
        co_return ec;
      }
    }
#ifdef ASIO_HAS_LIB_AIO
    if (size >= 512) {
      if (size % 512 != 0) {
        // must be multiple of 512
        co_return std::make_error_code(std::errc::invalid_argument);
      }
    }
    else {
      co_return co_await write_small_file(data, size);
    }

    if (size > write_buf_.size()) {
      write_buf_.resize(size);
    }
    size_t buf_size = write_buf_.size();
    memcpy(write_buf_.data(), data, size);
#endif
    size_t left_size = size;
    while (left_size) {
#ifdef ASIO_HAS_LIB_AIO
      auto [ec, write_size] = co_await asio_util::async_write_some(
          *stream_file_, asio::buffer(write_buf_.data(), size));
      if (ec) {
        co_return ec;
      }
      std::cout << "write_size " << write_size << "\n ";
      assert(write_size % 512 == 0);
      if (std::error_code seek_err = seek_file(write_size); seek_err) {
        co_return seek_err;
      }
#else
      auto [ec, write_size] = co_await asio_util::async_write_some(
          *stream_file_, asio::buffer(data, size));
      if (ec) {
        co_return ec;
      }

      if (std::error_code seek_err = seek_file(write_size); seek_err) {
        co_return seek_err;
      }
#endif
#ifdef ASIO_HAS_LIB_AIO
      if (write_size > buf_size) {
        co_return std::make_error_code(std::errc::io_error);
      }

      left_size -= (std::min)(write_size, size);
#else
      left_size -= write_size;
#endif
    }

    co_return std::error_code{};
  }

 private:
  std::error_code seek_file(size_t write_size) {
    offset_ += write_size;
    std::error_code seek_err;
    stream_file_->seek(offset_, asio::file_base::seek_basis::seek_set,
                       seek_err);
    return seek_err;
  }

#ifdef ASIO_HAS_LIB_AIO
  async_simple::coro::Lazy<std::error_code> write_small_file(const char* data,
                                                             size_t size) {
    std::error_code ec;
    size_t left_size = size;
    while (left_size) {
      size_t write_size =
          stream_file_->write_some(asio::buffer(data, size), ec);
      if (ec) {
        co_return ec;
      }
      left_size -= write_size;
    }

    co_return ec;
  }
#endif

  std::unique_ptr<asio::stream_file> stream_file_;
  std::string filepath_;

  size_t buf_size_ = 2048;
#ifdef ASIO_HAS_LIB_AIO
  std::vector<char, aligned_allocator<char, 512>> buf_;
  std::vector<char, aligned_allocator<char, 512>> write_buf_;
#else
  std::vector<char> buf_;
#endif
  size_t offset_ = 0;
  size_t read_total_ = 0;
  bool eof_ = false;
};
}  // namespace ylt