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

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace ylt {
inline constexpr std::size_t byte_bits =
    std::numeric_limits<std::uint8_t>::digits;

}  // namespace ylt

namespace ylt::detail {
template <typename Number, typename Callable, std::size_t... Indexes>
requires std::is_arithmetic_v<Number>
constexpr auto number_move_bits_helper(std::index_sequence<Indexes...>,
                                       bool big_endian,
                                       Callable&& handler) noexcept {
  constexpr std::size_t total_bits = sizeof(Number) * byte_bits;
  constexpr std::size_t max_move_bits = total_bits - byte_bits;
  std::ptrdiff_t baseline_move_bits =
      big_endian ? static_cast<std::ptrdiff_t>(max_move_bits) : 0;
  std::ptrdiff_t sign = big_endian ? -1 : 1;

  return std::forward<Callable>(handler)((std::pair{
      Indexes, baseline_move_bits + sign * static_cast<std::ptrdiff_t>(
                                               Indexes * byte_bits)})...);
}

template <bool Left, typename UnsignedNumber>
requires std::is_unsigned_v<UnsignedNumber>
constexpr auto bitwise_rotate_impl(UnsignedNumber number, int bits) noexcept {
  using limits_type = std::numeric_limits<UnsignedNumber>;
  using rotate_impl_type = UnsignedNumber (*)(UnsignedNumber number, int bits);

  constexpr auto rotl_impl = [](UnsignedNumber number, int bits) {
    return (number << bits) | (number >> (limits_type::digits - bits));
  };
  constexpr auto rotr_impl = [](UnsignedNumber number, int bits) {
    return (number >> bits) | (number << (limits_type::digits - bits));
  };
  constexpr auto rotate_impl =
      Left ? static_cast<rotate_impl_type>(rotl_impl) : rotr_impl;

  if (bits == 0) {
    return number;
  }

  bits %= limits_type::digits;

  if (bits > 0) {
    return rotate_impl(number, bits);
  }
  else {
    return bitwise_rotate_impl<!Left>(number, -bits);
  }
}
}  // namespace ylt::detail

namespace ylt {
template <auto Left, auto Right>
struct get_min : std::integral_constant<
                     std::common_type_t<decltype(Left), decltype(Right)>,
                     std::min(Left, Right)> {};

template <auto Left, auto Right>
inline constexpr auto get_min_v = get_min<Left, Right>::value;

template <auto Left, auto Right>
struct get_max : std::integral_constant<
                     std::common_type_t<decltype(Left), decltype(Right)>,
                     std::max(Left, Right)> {};

template <auto Left, auto Right>
inline constexpr auto get_max_v = get_max<Left, Right>::value;

/**
 * Calculates the binary logarithm of an unsigned number.
 * @tparam UnsignedNumber The numeric type
 * @param number The number
 * @return The result
 */
template <typename UnsignedNumber>
requires std::is_unsigned_v<UnsignedNumber>
constexpr auto log2(UnsignedNumber number) noexcept {
  UnsignedNumber result{};

  while ((number >>= 1) != 0) {
    ++result;
  }

  return result;
}

/**
 * Splits a number into one or more bytes.
 * @tparam Number The numeric type
 * @tparam Callable The callable type
 * @tparam TruncateSize The size in bytes of the sequence to be passed to
 * @param number The number
 * @param handler The handler whose arguments are the bytes of the number
 * @param big_endian A boolean that indicates whether the byte order is
 *                   big-endian
 * @return The return value of the handler
 */
template <typename Number, typename Callable,
          std::size_t TruncateSize = sizeof(Number)>
requires std::is_arithmetic_v<Number>
constexpr decltype(auto) split_number(
    Number number, Callable&& handler, bool big_endian = true,
    std::integral_constant<std::size_t, TruncateSize> = {}) noexcept {
  return detail::number_move_bits_helper<Number>(
      std::make_index_sequence<get_min_v<TruncateSize, sizeof(Number)>>{},
      big_endian, [&]<typename... Args>(Args&&... parts) {
        return std::forward<Callable>(handler)(
            static_cast<std::uint8_t>((static_cast<std::uintmax_t>(number) >>
                                       std::forward<Args>(parts).second) &
                                      0xFF)...);
      });
}

/**
 * Gets a certain bit of an unsigned number.
 * @tparam UnsignedNumber The numeric type
 * @param number The number
 * @param offset The bit offset
 * @return The bit value
 */
template <typename UnsignedNumber>
requires std::is_unsigned_v<UnsignedNumber>
constexpr std::uint8_t get_number_bit(UnsignedNumber number,
                                      int offset) noexcept {
  return static_cast<std::uint8_t>((number >> offset) & 0x01);
}

/**
 * ets a certain bit of an unsigned number.
 * @tparam UnsignedNumber The numeric type
 * @param number The number
 * @param offset The bit offset
 * @param bit The bit value
 */
template <typename UnsignedNumber>
requires std::is_unsigned_v<UnsignedNumber>
constexpr void set_number_bit(UnsignedNumber& number, int offset,
                              std::uint8_t bit) noexcept {
  number = (number & ~(static_cast<UnsignedNumber>(1) << offset)) |
           (static_cast<UnsignedNumber>(bit) << offset);
}

/**
 * Computes the result of bitwise left-rotating the value of "number" by "bits"
 * positions. This operation is also known as a left circular shift.
 * @tparam UnsignedNumber The unsigned numeric type
 * @param number The unsigned number
 * @param bits The bits
 * @return The result
 */
template <typename UnsignedNumber>
requires std::is_unsigned_v<UnsignedNumber>
constexpr auto rotl(UnsignedNumber number, int bits) noexcept {
  return detail::bitwise_rotate_impl<true>(number, bits);
}

/**
 * Computes the result of bitwise right-rotating the value of "number" by
 * "bits" positions. This operation is also known as a right circular shift.
 * @tparam UnsignedNumber The unsigned numeric type
 * @param number The unsigned number
 * @param bits The bits
 * @return The result
 */
template <typename UnsignedNumber>
requires std::is_unsigned_v<UnsignedNumber>
constexpr auto rotr(UnsignedNumber number, int bits) noexcept {
  return detail::bitwise_rotate_impl<false>(number, bits);
}

/**
 * Calculates (a - b) % c in which a, b and c are unsigned numbers.
 * Overflow is fixed up here.
 * @tparam UnsignedNumber The numeric type
 * @param minuend The minuend
 * @param subtrahend The subtrahend
 * @param divisor The divisor
 * @return The result
 */
template <typename UnsignedNumber>
requires std::is_unsigned_v<UnsignedNumber>
constexpr auto minus_mod_unsigned(UnsignedNumber minuend,
                                  UnsignedNumber subtrahend,
                                  UnsignedNumber divisor) noexcept {
  if (minuend >= subtrahend) {
    return (minuend - subtrahend) % divisor;
  }

  UnsignedNumber subtraction = minuend - subtrahend;
  UnsignedNumber addition =
      (subtraction / divisor + (subtraction % divisor != 0 ? 1 : 0)) * divisor;

  return (subtraction + addition) % divisor;
}

/**
 * Combines multiple bytes into a number.
 * @tparam Number The numeric type
 * @param data The bytes
 * @param big_endian A boolean that indicates whether the byte order is
 * big-endian
 * @return The number
 */
template <typename Number>
requires std::is_arithmetic_v<Number>
constexpr auto make_number(const std::array<std::uint8_t, sizeof(Number)>& data,
                           bool big_endian = true) noexcept {
  return detail::number_move_bits_helper<Number>(
      std::make_index_sequence<sizeof(Number)>{}, big_endian,
      [&]<typename... Args>(Args&&... parts) {
        return static_cast<Number>(
            ((static_cast<std::uintmax_t>(data[std::forward<Args>(parts).first])
              << std::forward<Args>(parts).second) +
             ...));
      });
}
}  // namespace ylt
