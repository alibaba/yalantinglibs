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

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <memory>
#include <numeric>
#include <ranges>
#include <type_traits>

#include "detail/nd_impl_types.hpp"
#include "detail/nd_small_sglist.hpp"
#include "nd_commons.hpp"
#include "nd_types.hpp"

namespace coro_io {

class mutable_buffer;

class const_buffer {
 public:
  using nd_buffer_tag = detail::nd_const_buffer_tag;

  const_buffer() = default;
  const_buffer(void const* addr, std::size_t length,
               std::uint32_t lkey) noexcept
      : addr_(addr), length_(length), lkey_(lkey) {}
  const_buffer(mutable_buffer const& b) noexcept;

  void const* addr() const noexcept { return addr_; }
  void const* data() const noexcept { return addr_; }
  std::size_t length() const noexcept { return length_; }
  std::uint32_t local_key() const noexcept { return lkey_; }

  friend const_buffer const* buffer_sequence_begin(
      const_buffer const& one) noexcept {
    return std::addressof(one);
  }
  friend const_buffer const* buffer_sequence_end(
      const_buffer const& one) noexcept {
    return std::addressof(one) + 1;
  }

 private:
  void const* addr_ = nullptr;
  std::size_t length_ = 0;
  std::uint32_t lkey_ = 0;
};

class mutable_buffer {
 public:
  using nd_buffer_tag = detail::nd_mutable_buffer_tag;

  mutable_buffer() = default;
  mutable_buffer(void* addr, std::size_t length, std::uint32_t lkey) noexcept
      : addr_(addr), length_(length), lkey_(lkey) {}

  void* addr() const noexcept { return addr_; }
  void* data() const noexcept { return addr_; }
  std::size_t length() const noexcept { return length_; }
  std::uint32_t local_key() const noexcept { return lkey_; }

  friend mutable_buffer const* buffer_sequence_begin(
      mutable_buffer const& one) noexcept {
    return std::addressof(one);
  }
  friend mutable_buffer const* buffer_sequence_end(
      mutable_buffer const& one) noexcept {
    return std::addressof(one) + 1;
  }

 private:
  void* addr_ = nullptr;
  std::size_t length_ = 0;
  std::uint32_t lkey_ = 0;
};

inline const_buffer::const_buffer(mutable_buffer const& b) noexcept
    : addr_(b.addr()), length_(b.length()), lkey_(b.local_key()) {}

using nd_const_buffer = const_buffer;
using nd_mutable_buffer = mutable_buffer;

template <typename MR>
concept memory_region_like = requires(MR& mr, std::size_t off, std::size_t n) {
  { mr.length() } -> std::convertible_to<std::size_t>;
  { mr.local_key() } -> std::convertible_to<std::uint32_t>;
  { mr.is_in_mr(off, n) } -> std::convertible_to<bool>;
};

template <typename MR>
  requires memory_region_like<MR> && (!std::is_const_v<MR>)
inline mutable_buffer buffer(MR& mr) {
  return mutable_buffer{mr.addr(), mr.length(), mr.local_key()};
}

template <typename MR>
  requires memory_region_like<MR>
inline const_buffer buffer(MR const& mr) {
  return const_buffer{mr.addr(), mr.length(), mr.local_key()};
}

template <typename MR>
  requires memory_region_like<MR> && (!std::is_const_v<MR>)
inline mutable_buffer buffer(MR& mr, std::size_t offset, std::size_t n) {
  if (!mr.is_in_mr(offset, n))
    return mutable_buffer{};
  return mutable_buffer{static_cast<std::uint8_t*>(mr.addr()) + offset, n,
                        mr.local_key()};
}

template <typename MR>
  requires memory_region_like<MR>
inline const_buffer buffer(MR const& mr, std::size_t offset, std::size_t n) {
  if (!mr.is_in_mr(offset, n))
    return const_buffer{};
  return const_buffer{static_cast<std::uint8_t const*>(mr.addr()) + offset, n,
                      mr.local_key()};
}

template <typename Buffer>
concept mr_buffer = requires { typename Buffer::nd_buffer_tag; };
template <typename BufferRef>
concept mr_buffer_ref = mr_buffer<std::remove_cvref_t<BufferRef>>;

template <typename Buffer>
concept const_mr_buffer = requires {
  requires mr_buffer<Buffer>;
  requires std::same_as<typename Buffer::nd_buffer_tag,
                        detail::nd_const_buffer_tag>;
};
template <typename BufferRef>
concept const_mr_buffer_ref = const_mr_buffer<std::remove_cvref_t<BufferRef>>;

template <typename Buffer>
concept mutable_mr_buffer = requires {
  requires mr_buffer<Buffer>;
  requires std::same_as<typename Buffer::nd_buffer_tag,
                        detail::nd_mutable_buffer_tag>;
};
template <typename BufferRef>
concept mutable_mr_buffer_ref =
    mutable_mr_buffer<std::remove_cvref_t<BufferRef>>;

template <typename BufferSequence>
concept mr_buffer_sequence = requires(BufferSequence bs) {
  { *bs.cbegin() } -> mr_buffer_ref;
  { *bs.cend() } -> mr_buffer_ref;
  { std::distance(bs.cbegin(), bs.cend()) };
};

template <mr_buffer_sequence BufferSequence>
inline decltype(auto) buffer_sequence_begin(BufferSequence const& bs) noexcept(
    noexcept(bs.cbegin())) {
  return bs.cbegin();
}

template <mr_buffer_sequence BufferSequence>
inline decltype(auto) buffer_sequence_end(BufferSequence const& bs) noexcept(
    noexcept(bs.cend())) {
  return bs.cend();
}

template <typename AdaptedBufferSequence>
concept mr_adapted_buffer_sequence = requires(AdaptedBufferSequence abs) {
  { *buffer_sequence_begin(abs) } -> mr_buffer_ref;
  { *buffer_sequence_end(abs) } -> mr_buffer_ref;
  { std::distance(buffer_sequence_begin(abs), buffer_sequence_end(abs)) };
};

template <typename BufferSequence>
concept mr_const_buffer_sequence = requires(BufferSequence bs) {
  { *buffer_sequence_begin(bs) } -> const_mr_buffer_ref;
  { *buffer_sequence_end(bs) } -> const_mr_buffer_ref;
  { std::distance(buffer_sequence_begin(bs), buffer_sequence_end(bs)) };
};

template <typename BufferSequence>
concept mr_mutable_buffer_sequence = requires(BufferSequence bs) {
  { *buffer_sequence_begin(bs) } -> mutable_mr_buffer_ref;
  { *buffer_sequence_end(bs) } -> mutable_mr_buffer_ref;
  { std::distance(buffer_sequence_begin(bs), buffer_sequence_end(bs)) };
};

namespace detail {

template <mr_adapted_buffer_sequence BufferSequence>
inline std::size_t buffer_size(BufferSequence const& buffers) noexcept {
  return std::accumulate(buffer_sequence_begin(buffers),
                         buffer_sequence_end(buffers), std::size_t{0},
                         [](std::size_t acc, auto const& buffer) {
                           return acc + buffer.length();
                         });
}

template <mr_adapted_buffer_sequence BufferSequence>
inline bool all_empty(BufferSequence const& buffers) noexcept {
  return std::ranges::all_of(buffer_sequence_begin(buffers),
                             buffer_sequence_end(buffers),
                             [](auto const& buffer) {
                               return buffer.length() == 0;
                             });
}

inline void fill_native_sge(native_sge_t& sge, const_buffer const& buffer) {
  sge.Buffer = const_cast<void*>(buffer.addr());
  sge.BufferLength = buffer.length();
  sge.MemoryRegionToken = buffer.local_key();
}

inline void fill_native_sge(native_sge_t& sge, mutable_buffer const& buffer) {
  sge.Buffer = buffer.addr();
  sge.BufferLength = buffer.length();
  sge.MemoryRegionToken = buffer.local_key();
}

template <mr_adapted_buffer_sequence BufferSequence>
inline built_sglist<native_sge_t> build_native_sglist(
    BufferSequence const& bs, nd_sglist_t& sglist, std::uint32_t max_sge = 0) {
  built_sglist<native_sge_t> built;
  sglist.clear();

  auto it = buffer_sequence_begin(bs);
  auto const end = buffer_sequence_end(bs);
  asio::error_code ec;
  for (; it != end; ++it) {
    if (max_sge != 0 && built.count >= max_sge) {
      built.too_many_sge = true;
      built.data = sglist.data();
      built.heap_spilled = sglist.uses_heap();
      return built;
    }
    auto& sge = sglist.append_uninitialized(ec);
    fill_native_sge(sge, *it);
    ++built.count;
    built.total_bytes += it->length();
    built.all_empty = built.all_empty && it->length() == 0;
  }

  built.data = sglist.data();
  built.heap_spilled = sglist.uses_heap();
  return built;
}

template <mr_adapted_buffer_sequence BufferSequence>
inline void buffers2sglist(BufferSequence const& bs, nd_sglist_t& sglist) {
  build_native_sglist(bs, sglist);
}

}  // namespace detail

}  // namespace coro_io
