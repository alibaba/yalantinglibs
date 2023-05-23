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
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include "asio/file_base.hpp"
#include "asio_coro_util.hpp"
#include "async_simple/coro/Lazy.h"

namespace ylt {
class coro_file {
 public:
  coro_file(asio::io_context::executor_type executor,
            const std::string& filepath,
            asio::file_base::flags flags = asio::stream_file::direct |
                                           asio::stream_file::read_write)
      : file_path_(filepath) {
    std::error_code ec;
    stream_file_ = std::make_unique<asio::stream_file>(executor);
    stream_file_->open(filepath, flags, ec);
    if (ec) {
      std::cout << ec.message() << "\n";
    }
  }

  bool is_open() { return stream_file_->is_open(); }

  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read_some(
      char* data, size_t size) {
    co_return co_await asio_util::async_read_some(*stream_file_,
                                                  asio::buffer(data, size));
  }

 private:
  std::string file_path_;
  std::unique_ptr<asio::stream_file> stream_file_;
};
}  // namespace ylt