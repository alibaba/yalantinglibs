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

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include "meta_math.hpp"

namespace ylt {
enum class sha3_type { sha3_224, sha3_256, sha3_384, sha3_512, sha3_type_size };
}  // namespace ylt

namespace ylt::detail {
struct sha3_type_traits {
  /**
  * The block size of a SHA3 algorithm, in bytes, defined in Table 3,
    Section 7. The comment in the footer notes that in general, the input
    block size of a sponge function is its rate (also known as r).
   */
  std::size_t block_size{};

  /**
   * The final hash size in bytes.
   */
  std::size_t final_hash_size{};
};

/**
* A std::uint64_t is 64-bit which has an identical bit length to para
  (also known as the length of z coordinate). The definition is in
  Section 5.2.
 */
using word_type = std::uint64_t;

/**
 * The number of rounds.
 */
inline constexpr std::size_t round_size = 24;

/**
 * The size of a word, in bits.
 */
inline constexpr std::size_t word_bits = std::numeric_limits<word_type>::digits;

/**
 * l = log2(w).
 */
inline constexpr std::size_t log2_word_bits = log2(word_bits);

/**
 * A common factor which is commonly used, i.e. number 5.
 */
inline constexpr std::size_t common_factor = 5;

/**
 * The size of a sponge, in words, defined in Section 5.2.
 */
inline constexpr std::size_t sponge_words =
    1600 / byte_bits / sizeof(word_type);

/**
 * Available traits of available SHA3 algorithms.
 */
inline constexpr std::array<sha3_type_traits,
                            static_cast<std::size_t>(sha3_type::sha3_type_size)>
    available_sha3_type_traits{sha3_type_traits{144, 224 / byte_bits},
                               sha3_type_traits{136, 256 / byte_bits},
                               sha3_type_traits{104, 384 / byte_bits},
                               sha3_type_traits{72, 512 / byte_bits}};

/**
* Represents a state array defined in Section 3.1, of which the data are
  word-aligned (std::uint64_t).
 */
class state_array {
 public:
  constexpr state_array() noexcept : data_{} {}

  [[nodiscard]] constexpr std::size_t size() const noexcept {
    return data_.size();
  }

  constexpr void reset() noexcept { data_ = {}; }

  constexpr word_type& operator[](std::size_t index) noexcept {
    return data_[index];
  }

  constexpr const word_type& operator[](std::size_t index) const noexcept {
    return data_[index];
  }

  constexpr word_type& operator()(std::size_t x, std::size_t y) noexcept {
    return data_[calculate_index(x, y)];
  }

  constexpr const word_type& operator()(std::size_t x,
                                        std::size_t y) const noexcept {
    return data_[calculate_index(x, y)];
  }

  template <std::size_t Size, typename = std::enable_if_t<
                                  Size <= sponge_words * sizeof(word_type)>>
  constexpr void truncate_as_bytes(
      std::array<std::uint8_t, Size>& buffer) noexcept {
    constexpr std::size_t buffer_words = Size / sizeof(word_type);
    constexpr std::size_t remaining_bytes = Size % sizeof(word_type);
    std::size_t index = 0;

    for (std::size_t i = 0; i < buffer_words; i++) {
      // According to the NIST standard, "strings" defined in that are
      // little-endian.
      split_number(
          data_[i],
          [&](auto... bytes) {
            ((buffer[index++] = bytes), ...);
          },
          false);
    }

    // Add the remaining bytes in the last incomplete word.
    if constexpr (remaining_bytes != 0) {
      split_number(
          data_[buffer_words],
          [&](auto... bytes) {
            ((buffer[index++] = bytes), ...);
          },
          false, std::integral_constant<std::size_t, remaining_bytes>{});
    }
  }

  static constexpr std::size_t calculate_index(std::size_t x,
                                               std::size_t y) noexcept {
    // A[x, y, z] = S[w(5y + x) + z] defined in Section 3.1.2.
    // The calculation is simplified as follows because values of a lane (values
    // along z coordinate) are combined into a word (std::uint64_t).
    return common_factor * (y % common_factor) + (x % common_factor);
  }

 private:
  std::array<word_type, sponge_words> data_;
};

/**
 * The context of a hash algorithm.
 * @tparam Type The type of the SHA3.
 */
template <sha3_type Type>
struct hash_context {
  static constexpr std::size_t block_size =
      available_sha3_type_traits[static_cast<std::size_t>(Type)].block_size;
  static constexpr std::size_t final_hash_size =
      available_sha3_type_traits[static_cast<std::size_t>(Type)]
          .final_hash_size;

  state_array state;
  state_array intermediate;
  std::size_t block_index;
  std::array<word_type, 5> tmp;
  std::array<std::uint8_t, block_size> block;

  constexpr hash_context() noexcept : block_index{}, tmp{}, block{} {}

  [[nodiscard]] constexpr std::size_t block_remaining_size() const noexcept {
    return block_size - block_index;
  }

  constexpr void reset() noexcept {
    tmp = {};
    block = {};
    state.reset();
    intermediate.reset();
    block_index = 0;
  }
};

/**
 * A helper function for rc(A) below defined in Section 3.2.5.
 * @param number The number.
 * @return The result.
 */
constexpr std::uint8_t step_mapping_helper_rc(word_type number) noexcept {
  // If t mod 255 = 0, return 1.
  if (number % 0xFF == 0) {
    return 1;
  }

  // Let R = 1000'0000.
  // Here all numbers are inverted for convenience.
  std::uint8_t bit = 0;
  word_type result = 0b0000'0001;

  for (std::size_t i = 1; i <= number % 0xFF; i++) {
    // For i from 1 to t mod 255, let:
    // a.R = 0 || R;
    // b.R[0] = R[0] ⊕ R[8];
    // c.R[4] = R[4] ⊕ R[8];
    // d.R[5] = R[5] ⊕ R[8];
    // e.R[6] = R[6] ⊕ R[8];
    // f.R = Trunc8[R].
    result <<= 1;
    bit = get_number_bit(result, 8);

    set_number_bit(result, 0, get_number_bit(result, 0) ^ bit);
    set_number_bit(result, 4, get_number_bit(result, 4) ^ bit);
    set_number_bit(result, 5, get_number_bit(result, 5) ^ bit);
    set_number_bit(result, 6, get_number_bit(result, 6) ^ bit);

    result &= 0xFF;
  }

  // Return R[0].
  return static_cast<std::uint8_t>(result & 0x01);
}

/**
* Generates a table of rotation bits for ��(A) at compile-time, defined in
  Section 3.2.2.
 */
inline constexpr auto step_mapping_rho_rotation_bits = [] {
  std::array<std::int32_t, sponge_words> result{};

  /// (x, y) = (1, 0)
  /// For t from 0 to 23, (t + 1)(t + 2) / 2 mod w
  for (std::size_t x = 1, y = 0, t = 0, tmp = 0; t < 24; t++) {
    result[state_array::calculate_index(x, y)] =
        static_cast<std::int32_t>((((t + 1) * (t + 2)) >> 1) % word_bits);

    // (x, y) = (y, (2x + 3y) mod 5)
    tmp = y;
    y = (2 * x + 3 * y) % common_factor;
    x = tmp;
  }

  return result;
}();

/**
 * Generates an RC table for tau(A) at compile-time.
 */
inline constexpr auto step_mapping_tau_rc_table =
    []<std::size_t... Is>(std::index_sequence<Is...>) {
      constexpr auto func_rc = [](std::size_t round) {
        // Let RC = 0w.
        // For j from 0 to l = log2(w), let RC[2 ^ j – 1] = rc(j + 7ir).
        word_type result = 0;

        for (std::size_t i = 0; i <= log2_word_bits; i++) {
          set_number_bit(result, (1 << i) - 1,
                         step_mapping_helper_rc(i + 7 * round));
        }

        return result;
      };

      return std::array<word_type, sizeof...(Is)>{func_rc(Is)...};
    }(std::make_index_sequence<round_size>{});

/**
 * A step mapping function named theta(A) defined in Section 3.2.1.
 * @tparam Type The type of SHA3.
 * @param context The hash context.
 */
template <sha3_type Type>
constexpr void step_mapping_theta(hash_context<Type>& context) noexcept {
  // For all pairs (x, z) such that 0 ≤ x < 5 and 0 ≤ z < w, let C[x, z] = A[x,
  // 0, z] ⊕ A[x, 1, z] ⊕ A[x, 2, z] ⊕ A[x, 3, z] ⊕ A[x, 4, z]. Here a lane
  // (values along z coordinate) is represented as a word (std::uint64_t).
  for (std::size_t x = 0; x < common_factor; x++) {
    context.tmp[x] = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      return (context.state(x, Is) ^ ...);
    }(std::make_index_sequence<common_factor>{});
  }

  // For all pairs (x, z) such that 0 ≤ x < 5 and 0 ≤ z < w, let D[x, z] = C[(x
  // - 1) mod 5, z] ⊕ C[(x + 1) mod 5, (z - 1) mod w]. For all triples(x, y, z)
  // such that 0 ≤ x < 5, 0 ≤ y < 5, and 0 ≤ z < w, let A′[x, y, z] = A[x, y, z]
  // ⊕ D[x, z]. Figure 3 in Section 3.2.1 is intuitive and (z - 1) mod w is
  // equivalent to rotating a word to the left by one bit.
  for (std::size_t x = 0; x < common_factor; x++) {
    for (std::size_t y = 0; y < common_factor; y++) {
      context.state(x, y) ^=
          (context.tmp[minus_mod_unsigned<std::size_t>(x, 1, common_factor)] ^
           rotl(context.tmp[(x + 1) % common_factor], 1));
    }
  }
}

/**
 * A step mapping function named rho(A) defined in Section 3.2.2.
 * @tparam Type The type of sha3.
 * @param context The hash context.
 */
template <sha3_type Type>
constexpr void step_mapping_rho(hash_context<Type>& context) noexcept {
  // For all z such that 0 �� z �� w, let A'[0, 0, z] = A[0, 0, z].
  context.intermediate(0, 0) = context.state(0, 0);

  // (x, y) = (1, 0)
  // For t from 0 to 23.
  [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    // For all z such that 0 ≤ z < w, let A′[x, y, z] = A[x, y, (z – (t + 1)(t +
    // 2) / 2) mod w]. Here a lane (values along z coordinate) is represented as
    // a word (std::uint64_t). (x, y) = (y, (2x + 3y) mod 5)
    ((context.intermediate[Is] =
          rotl(context.state[Is], step_mapping_rho_rotation_bits[Is])),
     ...);
  }(std::make_index_sequence<step_mapping_rho_rotation_bits.size()>{});
}

/**
 * A step mapping function named pi(A) defined in Section 3.2.3.
 * @tparam Type The type of SHA3.
 * @param context The hash context.
 */
template <sha3_type Type>
constexpr void step_mapping_pi(hash_context<Type>& context) noexcept {
  for (std::size_t x = 0; x < common_factor; x++) {
    for (std::size_t y = 0; y < common_factor; y++) {
      // For all triples (x, y, z) such that 0 ≤ x < 5, 0 ≤ y < 5, and 0 ≤ z <
      // w, let A′[x, y, z] = A[(x + 3y) mod 5, x, z]. Here a lane (values along
      // z coordinate) is represented as a word (std::uint64_t).
      context.state(x, y) = context.intermediate(x + 3 * y, x);
    }
  }
}

/**
 * A step mapping function named chi(A) defined in Section 3.2.4.
 * @tparam Type The type of SHA3.
 * @param context The hash context.
 */
template <sha3_type Type>
constexpr void step_mapping_chi(hash_context<Type>& context) noexcept {
  for (std::size_t x = 0; x < common_factor; x++) {
    for (std::size_t y = 0; y < common_factor; y++) {
      // For all triples (x, y, z) such that 0 ≤ x < 5, 0 ≤ y < 5, and 0 ≤ z <
      // w, let A′[x, y, z] = A[x, y, z] ⊕((A[(x + 1) mod 5, y, z] ⊕ 1) · A[(x +
      // 2) mod 5, y, z]). Here a lane (values along z coordinate) is
      // represented as a word (std::uint64_t).
      context.intermediate(x, y) =
          context.state(x, y) ^
          (~context.state(x + 1, y) & context.state(x + 2, y));
    }
  }
}

/**
 *  A step mapping function named tau(A) defined in Section 3.2.5.
 * @tparam Type The type of SHA3.
 * @param context The hash context.
 * @param round The round index.
 */
template <sha3_type Type>
constexpr void step_mapping_tau(hash_context<Type>& context,
                                std::size_t round) noexcept {
  // For all triples (x, y, z) such that 0 ≤ x < 5, 0 ≤ y < 5, and 0 ≤ z < w,
  // let A′[x, y, z] = A[x, y, z]. For all z such that 0 ≤ z < w, let A′ [0, 0,
  // z] = A′ [0, 0, z] ⊕ RC[z]. Here a lane (values along z coordinate) is
  // represented as a word (std::uint64_t).
  context.state = context.intermediate;
  context.state(0, 0) ^= step_mapping_tau_rc_table[round];
}

/**
 * A function that implements the KECCAK-p[b, nr] permutation.
 * @tparam Type The type of SHA3.
 * @param context The hash context.
 */
template <sha3_type Type>
constexpr void keccak_p(hash_context<Type>& context) noexcept {
  for (std::size_t i = 0; i < round_size; i++) {
    step_mapping_theta(context);
    step_mapping_rho(context);
    step_mapping_pi(context);
    step_mapping_chi(context);
    step_mapping_tau(context, i);
  }
}

/**
* A function that implements pad10*1(x, m) defined in Section 5.1 and M || 01
  defined in Section 6.1.
 * @tparam Type The type of SHA3.
 * @param context The hash context.
 */
template <sha3_type Type>
constexpr void pad10_1_and_append_01(hash_context<Type>& context) noexcept {
  // pad10*1(x, m) = string P such that m + len(P) is a positive multiple of x.
  // 1. Let j = (– m – 2) mod x.
  // 2. Return P = 1 || 0j || 1.
  // For a byte buffer, the padding bytes are 0x01(0b0000'0001) and
  // 0x80(0b1000'0000). Considering M || 01 defined in Section 6.1, the final
  // padding bytes are 0x06((0b0000'0001 << 2) | 0b0000'0010, which shortens the
  // padding zeros by 2 bits) and 0x80;
  if (context.block_index + 1 == hash_context<Type>::block_size) {
    // For only one single byte, just combines two parts as a result of
    // 0x86(0b1000'0000 | 0b0000'0110).
    context.block[context.block_index] = 0x86;
  }
  else {
    context.block[context.block_index] = 0x06;

    for (std::size_t i = context.block_index + 1;
         i < hash_context<Type>::block_size - 1; ++i) {
      context.block[i] = 0;
    }

    context.block.back() = 0x80;
  }
}

/**
* A function that updates the current state by SPONGE[f, pad, r](N, d) (when r
  = block size) defined in Step 6, Algorithm 8, Section 4.
 * @tparam Type The type of SHA3.
 * @param context The hash context.
 */
template <sha3_type Type>
constexpr void sponge_step_6(hash_context<Type>& context) noexcept {
  // For i from 0 to n - 1, let S = f(S ⊕ (Pi || 0c)).
  // Here n refers to the block at index n.
  constexpr std::size_t block_words =
      hash_context<Type>::block_size / sizeof(word_type);

  for (std::size_t i = 0; i < block_words; i++) {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      // According to the NIST standard, "strings" defined in that are
      // little-endian.
      context.state[i] ^= make_number<word_type>(
          {context.block[i * sizeof(word_type) + Is]...}, false);
    }(std::make_index_sequence<sizeof(word_type)>{});
  }

  // f = KECCAK-p[b, nr]
  keccak_p(context);
  context.block_index = 0;
}

/**
* A function that finalizes the state by SPONGE[f, pad, r](N, d) (when r =
  block size) defined in Algorithm 8, Section 4 and Section 4.
 * @tparam Type The type of SHA3.
 * @param context The hash context.
 * @return The finalized data.
 */
template <sha3_type Type>
constexpr auto sponge_finalize(hash_context<Type>& context) noexcept {
  // 1. Let P = N || pad(r, len(N)).
  // 2. Let n = len(P) / r.
  // 3. Let c = b - r.
  // 4. Let P0, ... , Pn - 1 be the unique sequence of strings of length r such
  // that P = P0 || … || Pn-1.
  // 5. Let S = 0b.
  // 6. For i from 0 to n - 1, let S = f(S ⊕(Pi || 0c)).
  // 7. Let Z be the empty string.
  // 8. Let Z = Z || Trunc_r(S).
  // 9. If d ≤ |Z| , then return Trunc d(Z); else continue.
  // 10. Let S = f(S), and continue with Step 8.
  pad10_1_and_append_01(context);
  sponge_step_6(context);

  std::array<std::uint8_t, hash_context<Type>::final_hash_size> result{};

  // The final hash size denoted by d is always smaller than |Z|, so "else" in
  // Step 9, and Step 10 are omitted here.
  context.state.truncate_as_bytes(result);
  context.reset();

  return result;
}
}  // namespace ylt::detail
