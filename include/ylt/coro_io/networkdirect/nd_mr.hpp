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

#include <cstddef>
#include <cstdint>
#include <iostream>

#include "nd_buffer.hpp"
#include "nd_device.hpp"
#include "detail/nd_ops_verbs.hpp"

namespace coro_io {

// memory region raii type
class nd_memory_region {
 private:
  detail::nd2_memory_region_ptr mr_;
  void* addr_;
  std::size_t length_;
  mr_acccess_flag_t flag_;
  int extra_flag_;

 public:
  inline explicit nd_memory_region(
      nd_device_ptr const& device, void* addr, std::size_t length,
      mr_acccess_flag_t flag = mr_access_remote_write, int extra_flag = 0);

  inline ~nd_memory_region();

  nd_memory_region(nd_memory_region const&) = delete;
  nd_memory_region& operator=(nd_memory_region const&) = delete;
  nd_memory_region(nd_memory_region&&) = default;
  nd_memory_region& operator=(nd_memory_region&&) = default;

  std::uint32_t local_key() const {
    if (!mr_) {
      asio::detail::throw_error(rdma_errc::invalid_handle);
    }
    return mr_->GetLocalToken();
  }

  std::uint32_t remote_key() const {
    if (!mr_) {
      asio::detail::throw_error(rdma_errc::invalid_handle);
    }
    return mr_->GetRemoteToken();
  }

  bool is_in_mr(void const* addr, std::size_t length) const noexcept {
    if (!mr_) {
      return false;
    }
    auto const diff = static_cast<char const*>(addr) -
                      static_cast<char const*>(addr_);
    if (diff < 0 || static_cast<std::size_t>(diff) >= length_) {
      return false;
    }
    return static_cast<std::size_t>(diff) + length <= length_;
  }

  bool is_in_mr(std::size_t offset, std::size_t length) const noexcept {
    return offset + length <= this->length();
  }

  void const* addr() const noexcept {
    return addr_;
  }

  void* addr() noexcept {
    return addr_;
  }

  std::size_t length() const noexcept {
    return length_;
  }

  // Advertise a sub-range of this MR to the peer (the rkey/remote-access role,
  // distinct from the local-SGE buffer elements which only carry lkey/local
  // token). The returned {addr, remote token} is what the peer's RDMA
  // read/write targets.
  inline nd_remote_addr_t remote_addr(std::size_t offset,
                                      std::size_t length) const;

 private:
  inline static detail::nd2_memory_region_ptr throw_reg_mr(
      nd_device_ptr const& device, void* addr, std::size_t length,
      mr_acccess_flag_t flag, int extra_flag);

 public:
  // slice/cslice return the shared, value-semantic coro_io::{mutable,const}_buffer.
  inline mutable_buffer slice(std::size_t offset, std::size_t length);

  inline const_buffer slice(std::size_t offset,
                            std::size_t length) const;

  inline mutable_buffer slice(void* addr, std::size_t length);

  inline const_buffer slice(void const* addr, std::size_t length) const;

  inline const_buffer cslice(std::size_t offset, std::size_t length);

  inline const_buffer cslice(std::size_t offset,
                             std::size_t length) const;

  inline const_buffer cslice(void const* addr, std::size_t length);

  inline const_buffer cslice(void const* addr,
                             std::size_t length) const;
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

inline nd_memory_region::nd_memory_region(nd_device_ptr const& device, void* addr,
                                          std::size_t length,
                                          mr_acccess_flag_t flag, int extra_flag)
    : mr_(throw_reg_mr(device, addr, length, flag, extra_flag))
    , addr_(addr)
    , length_(length)
    , flag_(flag)
    , extra_flag_(extra_flag) {}

inline nd_memory_region::~nd_memory_region() {
  if (!mr_) {
    return;
  }

  asio::error_code ec{};
  detail::verbs_ops::dereg_mr(mr_.Get(), ec);
  if (ec) {
    std::cerr << "Failed to deregister memory region, errc(" << ec.value()
              << ") message(" << ec.message() << ")\n";
  }
}

inline nd_remote_addr_t nd_memory_region::remote_addr(std::size_t offset,
                                                      std::size_t length) const {
  if (!is_in_mr(offset, length)) {
    return nd_remote_addr_t{0, 0};
  }
  return nd_remote_addr_t{
      reinterpret_cast<std::uint64_t>(addr_) + offset, remote_key()};
}

inline detail::nd2_memory_region_ptr nd_memory_region::throw_reg_mr(
    nd_device_ptr const& device, void* addr, std::size_t length,
    mr_acccess_flag_t flag, int extra_flag) {
  if (!device) {
    asio::detail::throw_error(rdma_errc::invalid_device);
  }
  asio::error_code ec{};
  detail::nd2_memory_region_ptr result{detail::verbs_ops::reg_mr(
      device->pd_.get(), addr, length, flag, extra_flag, ec)};
  asio::detail::throw_error(ec);
  return result;
}

inline mutable_buffer nd_memory_region::slice(std::size_t offset,
                                              std::size_t length) {
  return coro_io::buffer(*this, offset, length);
}

inline const_buffer nd_memory_region::slice(std::size_t offset,
                                            std::size_t length) const {
  return coro_io::buffer(*this, offset, length);
}

inline mutable_buffer nd_memory_region::slice(void* addr, std::size_t length) {
  auto const ptr_diff = reinterpret_cast<std::uint8_t*>(addr) -
                        reinterpret_cast<std::uint8_t*>(this->addr());
  if (ptr_diff > 0) {
    return slice(static_cast<std::size_t>(ptr_diff), length);
  }
  return mutable_buffer{};
}

inline const_buffer nd_memory_region::slice(void const* addr,
                                            std::size_t length) const {
  auto const ptr_diff = reinterpret_cast<std::uint8_t const*>(addr) -
                        reinterpret_cast<std::uint8_t const*>(this->addr());
  if (ptr_diff > 0) {
    return cslice(static_cast<std::size_t>(ptr_diff), length);
  }
  return const_buffer{};
}

inline const_buffer nd_memory_region::cslice(std::size_t offset,
                                             std::size_t length) {
  return const_cast<nd_memory_region const*>(this)->slice(offset, length);
}

inline const_buffer nd_memory_region::cslice(std::size_t offset,
                                             std::size_t length) const {
  return this->slice(offset, length);
}

inline const_buffer nd_memory_region::cslice(void const* addr,
                                             std::size_t length) {
  return const_cast<nd_memory_region const*>(this)->slice(addr, length);
}

inline const_buffer nd_memory_region::cslice(void const* addr,
                                             std::size_t length) const {
  return this->slice(addr, length);
}

}  // namespace coro_io
