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

#include "asio/error.hpp"
#include "asio/file_base.hpp"
#include "asio_coro_util.hpp"
#include "async_simple/coro/Lazy.h"

namespace ylt {
inline asio::file_base::flags default_flags() {
  return asio::stream_file::read_write | asio::stream_file::create;
}

class coro_file {
 public:
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

  return ec;
}

  bool eof() {
  return eof_;
}

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
  stream_file_->seek(offset_, asio::file_base::seek_basis::seek_set, seek_err);
  return seek_err;
}

#ifdef ASIO_HAS_LIB_AIO
async_simple::coro::Lazy<std::error_code> write_small_file(const char* data,
                                                           size_t size) {
  std::error_code ec;
  size_t left_size = size;
  while (left_size) {
    size_t write_size = stream_file_->write_some(asio::buffer(data, size), ec);
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

size_t seek_offset_ = 0;
bool eof_ = false;
};
}  // namespace ylt