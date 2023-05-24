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

class coro_file {
 public:
  coro_file(asio::io_context::executor_type executor,
            const std::string& filepath,
            asio::file_base::flags flags = asio::stream_file::direct |
                                           asio::stream_file::read_write) {
    std::error_code ec;
    stream_file_ = std::make_unique<asio::stream_file>(executor);
    stream_file_->open(filepath, flags, ec);
    if (ec) {
      std::cout << ec.message() << "\n";
    }
    buf_.resize(buf_size_);
  }

  bool set_buf_size(size_t size) {
    if (size < 512) {
      return false;
    }

    buf_size_ = size;
    buf_.resize(buf_size_);
    return true;
  }

  bool is_open() { return stream_file_->is_open(); }

  bool eof() { return eof_; }

  async_simple::coro::Lazy<std::pair<std::error_code, std::string_view>>
  async_read_some() {
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

 private:
  std::unique_ptr<asio::stream_file> stream_file_;

  size_t buf_size_ = 2048;
  std::vector<char, aligned_allocator<char, 512>> buf_;
  size_t read_total_ = 0;
  bool eof_ = false;
};
}  // namespace ylt