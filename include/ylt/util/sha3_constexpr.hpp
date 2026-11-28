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
 *
 * Author's Email: metabeyond@outlook.com
 * Author's Github: https://github.com/refvalue/
 * Paper Cited: https://doi.org/10.6028/NIST.FIPS.202
 * Description: this source file contains code for parsing function names from
 * their signatures, especially optimized for the MSVC implementation.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "meta_numeric_conversion.hpp"
#include "meta_string.hpp"
#include "sha3_detail.hpp"

namespace ylt::detail {
/**
 * A helper class that digests input data.
 * @tparam Type The type of SHA3.
 */
template <sha3_type Type>
class hash_digest {
 public:
  using context_type = hash_context<Type>;

  /**
   * Updates the context with a piece of data.
   * @tparam Size The size in bytes.
   * @param data The data.
   * @return The reference to self.
   */
  template <std::size_t Size>
  constexpr hash_digest& update(
      const std::array<std::uint8_t, Size>& data) noexcept {
    return update(data.data(), data.size());
  }

  /**
   * Updates the context with a piece of data.
   * @param data Updates the context with a piece of data.
   * @param size The size in bytes.
   * @return The reference to self.
   */
  constexpr hash_digest& update(const std::uint8_t* data,
                                std::size_t size) noexcept {
    auto source_ptr = data;
    const auto source_end_ptr = data + size;
    auto destination_ptr = context_.block.data() + context_.block_index;

    std::size_t real_size{};

    // Fills the internal buffer and updates the context if necessary.
    while (source_ptr < source_end_ptr) {
      real_size = std::min<std::size_t>(source_end_ptr - source_ptr,
                                        context_.block_remaining_size());
      destination_ptr =
          std::copy(source_ptr, source_ptr + real_size, destination_ptr);
      source_ptr += real_size;
      context_.block_index += real_size;

      // Updates the state when the internal buffer is full.
      if (context_.block_index >= context_type::block_size) {
        detail::sponge_step_6(context_);
        destination_ptr = context_.block.data();
      }
    }

    return *this;
  }

  /**
   * Finalizes the context (pads the data and calculates the final hash value).
   * @return The finalized data.
   */
  constexpr auto finalize() noexcept {
    return detail::sponge_finalize(context_);
  }

 private:
  context_type context_;
};
}  // namespace ylt::detail

namespace ylt {
template <sha3_type Type>
constexpr auto sha3_digest(std::span<const std::uint8_t> data) noexcept {
  return detail::hash_digest<Type>{}
      .update(data.data(), data.size())
      .finalize();
}

template <sha3_type Type>
constexpr auto sha3_digest(std::string_view data) noexcept {
  detail::hash_digest<Type> digest;
  std::array<std::uint8_t, 1024> buffer{};
  const auto blocks = data.size() / buffer.size();
  std::size_t index{};

  auto update = [&](std::size_t size) {
    std::copy_n(data.data() + index * buffer.size(),
                static_cast<std::ptrdiff_t>(size), buffer.data());
    digest.update(buffer.data(), size);
  };

  for (; index < blocks; ++index) {
    update(buffer.size());
  }

  if (const auto remaining_size = data.size() % buffer.size();
      remaining_size != 0) {
    update(remaining_size);
  }

  return digest.finalize();
}
}  // namespace ylt
