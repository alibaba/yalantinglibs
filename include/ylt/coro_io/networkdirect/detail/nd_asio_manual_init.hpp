/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
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

#include <asio/detail/winsock_init.hpp>

#include <iostream>
#include <system_error>

#include "ylt/coro_io/networkdirect/networkdirect.hpp"

namespace coro_io::detail {

struct nd_process_global_t {
  HRESULT hr_{S_OK};

  inline nd_process_global_t() noexcept : hr_(::NdStartup()) {}

  inline ~nd_process_global_t() {
    if (SUCCEEDED(hr_)) {
      auto const hr = ::NdCleanup();
      if (FAILED(hr)) {
        std::cerr << "Failed to call NdCleanup, exiting ...\n";
      }
    }
  }

  void throw_on_error() const {
    if (FAILED(hr_)) {
      throw std::system_error(hr_, std::system_category(), "NdStartup failed");
    }
  }

  nd_process_global_t(nd_process_global_t const&) = delete;
  nd_process_global_t& operator=(nd_process_global_t const&) = delete;
  nd_process_global_t(nd_process_global_t&&) = delete;
  nd_process_global_t& operator=(nd_process_global_t&&) = delete;
};

inline nd_process_global_t& nd_process_global() {
  static nd_process_global_t instance;
  return instance;
}

struct nd_process_global_initializer_t {
  inline nd_process_global_initializer_t() noexcept {
    (void)nd_process_global();
  }
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
inline asio::detail::winsock_init<>::manual nd_asio_manual_winsock_init;
inline nd_process_global_initializer_t nd_process_global_initializer;
#pragma warning(pop)
#else
inline asio::detail::winsock_init<>::manual nd_asio_manual_winsock_init
    __attribute__((init_priority(101)));
inline nd_process_global_initializer_t nd_process_global_initializer
    __attribute__((init_priority(102)));
#endif

struct nd_global_t {
  inline nd_global_t() {
    nd_process_global().throw_on_error();
  }

  nd_global_t(nd_global_t const&) = delete;
  nd_global_t& operator=(nd_global_t const&) = delete;
  nd_global_t(nd_global_t&&) = delete;
  nd_global_t& operator=(nd_global_t&&) = delete;
};

}  // namespace coro_io::detail
