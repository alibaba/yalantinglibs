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
#include <array>
#include <cstddef>
#include <memory>

#include "asio/error.hpp"

namespace coro_io::detail {

template <class NativeSge>
struct built_sglist {
  NativeSge* data = nullptr;
  std::size_t count = 0;
  std::size_t total_bytes = 0;
  bool all_empty = true;
  bool too_many_sge = false;
  bool heap_spilled = false;
};

template <class NativeSge, std::size_t InlineCount = 8>
class small_sglist {
 public:
  static constexpr std::size_t inline_sge_count = InlineCount;

  small_sglist() = default;
  ~small_sglist() = default;
  small_sglist(small_sglist const&) = delete;
  small_sglist& operator=(small_sglist const&) = delete;

  void clear() noexcept {
    data_ = nullptr;
    size_ = 0;
  }

  bool reserve(std::size_t count, asio::error_code& ec) {
    if (count <= inline_sge_count) {
      ec.clear();
      return true;
    }

    auto const was_inline = data_ == inline_.data();
    auto const was_heap = heap_ && data_ == heap_.get();
    if (count > heap_capacity_) {
      auto heap = std::make_unique<NativeSge[]>(count);
      if (was_heap) {
        // Heap -> larger heap: preserve existing entries before replacing it.
        std::copy_n(heap_.get(), size_, heap.get());
      }
      heap_ = std::move(heap);
      heap_capacity_ = count;
    }

    if (was_inline) {
      // Inline stack -> heap: this is needed even when heap capacity exists.
      std::copy_n(inline_.data(), size_, heap_.get());
    }
    ec.clear();
    data_ = heap_.get();
    return true;
  }

  void resize(std::size_t count) {
    if (count == 0) {
      clear();
      return;
    }

    if (count <= inline_sge_count) {
      if (uses_heap()) {
        // Heap -> inline stack when resizing back under the inline capacity.
        std::copy_n(heap_.get(), (std::min)(size_, count), inline_.data());
      }
      data_ = inline_.data();
      size_ = count;
      return;
    }

    asio::error_code ec{};
    reserve(count, ec);
    size_ = count;
  }

  NativeSge& append_uninitialized(asio::error_code& ec) {
    auto const index = size_;
    auto const target = size_ + 1;
    if (target > inline_sge_count) {
      auto capacity = target;
      if (target > heap_capacity_) {
        auto const doubled = heap_capacity_ == 0 ? inline_sge_count * 2
                                                 : heap_capacity_ * 2;
        capacity = (std::max)(target, doubled);
      }
      reserve(capacity, ec);
    }
    else {
      if (!data_) {
        data_ = inline_.data();
      }
      ec.clear();
    }
    size_ = target;
    ec.clear();
    return data_[index];
  }

  NativeSge* data() noexcept { return data_; }
  NativeSge const* data() const noexcept { return data_; }
  std::size_t size() const noexcept { return size_; }
  std::size_t capacity() const noexcept {
    return uses_heap() ? heap_capacity_ : inline_sge_count;
  }
  bool uses_heap() const noexcept { return heap_ && data_ == heap_.get(); }
  NativeSge& operator[](std::size_t i) noexcept { return data_[i]; }
  NativeSge const& operator[](std::size_t i) const noexcept {
    return data_[i];
  }

 private:
  std::array<NativeSge, inline_sge_count> inline_{};
  std::unique_ptr<NativeSge[]> heap_;
  std::size_t heap_capacity_ = 0;
  NativeSge* data_ = nullptr;
  std::size_t size_ = 0;
};

}  // namespace coro_io::detail
