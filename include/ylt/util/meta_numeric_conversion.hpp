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
 * Description: this source file contains code for parsing function names from
 * their signatures, especially optimized for the MSVC implementation.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "meta_math.hpp"
#include "meta_string.hpp"

namespace ylt::detail {
template <typename T>
requires std::is_standard_layout_v<T>
inline constexpr std::size_t hexadecimal_character_size_v = sizeof(T) * 2;

inline constexpr std::string_view hexadecimal_characters{"0123456789ABCDEF"};

constexpr auto to_hexadecimal_character(std::uint8_t byte) noexcept {
  return refvalue::meta_string{hexadecimal_characters[byte >> 4],
                               hexadecimal_characters[byte & 0xF]};
}
}  // namespace ylt::detail

namespace ylt {
/**
 * Converts a number to a hexadecimal meta_string.
 * @tparam Number The numeric type
 * @param number The number
 * @param big_endian A boolean that indicates whether the byte order is
 *                   big-endian
 * @return The hexadecimal meta_string
 */
template <typename Number>
requires std::is_arithmetic_v<Number>
constexpr auto to_hexadecimal_meta_string(Number number,
                                          bool big_endian = true) noexcept {
  return split_number(
      number,
      [](auto... bytes) {
        return refvalue::meta_string{
            detail::to_hexadecimal_character(bytes)...};
      },
      big_endian);
}

/**
 * Converts a buffer to hexadecimal meta_string.
 * @tparam N The size of the buffer.
 * @param data The data.
 * @return The hexadecimal meta_string
 */
template <std::size_t N>
constexpr auto to_hexadecimal_meta_string(
    std::span<const std::uint8_t, N> data) noexcept {
  return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    return refvalue::meta_string{detail::to_hexadecimal_character(data[Is])...};
  }
  (std::make_index_sequence<N>{});
}

/**
 * Converts a buffer to hexadecimal meta_string.
 * @tparam N The size of the buffer.
 * @param data The data.
 * @return The hexadecimal meta_string
 */
template <std::size_t N>
constexpr auto to_hexadecimal_meta_string(
    const std::array<std::uint8_t, N>& data) noexcept {
  return to_hexadecimal_meta_string(std::span<const std::uint8_t, N>{data});
}
}  // namespace ylt
