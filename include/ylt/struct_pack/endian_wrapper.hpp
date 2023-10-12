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
#include <type_traits>
#include <bit>

#include "reflection.hpp"
#include "ylt/struct_pack/util.h"

namespace struct_pack::detail {
#if __cpp_lib_endian >= 201907L
constexpr inline bool is_system_little_endian =
    (std::endian::little == std::endian::native);
static_assert(std::endian::native == std::endian::little ||
                  std::endian::native == std::endian::big,
              "struct_pack don't support middle-endian");
#else
#define BYTEORDER_LITTLE_ENDIAN 0  // Little endian machine.
#define BYTEORDER_BIG_ENDIAN 1     // Big endian machine.

//#define BYTEORDER_ENDIAN BYTEORDER_LITTLE_ENDIAN

#ifndef BYTEORDER_ENDIAN
// Detect with GCC 4.6's macro.
#if defined(__BYTE_ORDER__)
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define BYTEORDER_ENDIAN BYTEORDER_LITTLE_ENDIAN
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define BYTEORDER_ENDIAN BYTEORDER_BIG_ENDIAN
#else
#error \
    "Unknown machine byteorder endianness detected. User needs to define BYTEORDER_ENDIAN."
#endif
// Detect with GLIBC's endian.h.
#elif defined(__GLIBC__)
#include <endian.h>
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define BYTEORDER_ENDIAN BYTEORDER_LITTLE_ENDIAN
#elif (__BYTE_ORDER == __BIG_ENDIAN)
#define BYTEORDER_ENDIAN BYTEORDER_BIG_ENDIAN
#else
#error \
    "Unknown machine byteorder endianness detected. User needs to define BYTEORDER_ENDIAN."
#endif
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro.
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#define BYTEORDER_ENDIAN BYTEORDER_LITTLE_ENDIAN
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#define BYTEORDER_ENDIAN BYTEORDER_BIG_ENDIAN
// Detect with architecture macros.
#elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || \
    defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || \
    defined(__hppa) || defined(_MIPSEB) || defined(_POWER) ||      \
    defined(__s390__)
#define BYTEORDER_ENDIAN BYTEORDER_BIG_ENDIAN
#elif defined(__i386__) || defined(__alpha__) || defined(__ia64) ||  \
    defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) ||     \
    defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) ||   \
    defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || \
    defined(_M_X64) || defined(__bfin__)
#define BYTEORDER_ENDIAN BYTEORDER_LITTLE_ENDIAN
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#define BYTEORDER_ENDIAN BYTEORDER_LITTLE_ENDIAN
#else
#error \
    "Unknown machine byteorder endianness detected. User needs to define BYTEORDER_ENDIAN."
#endif
#endif
constexpr inline bool is_system_little_endian =
    (BYTEORDER_ENDIAN == BYTEORDER_LITTLE_ENDIAN);
#endif

template <std::size_t block_size>
constexpr inline bool is_little_endian_copyable =
    is_system_little_endian || block_size == 1;

template <std::size_t block_size, typename writer_t>
void write_wrapper(writer_t& writer, const char* data) {
  if constexpr (is_system_little_endian || block_size == 1) {
    writer.write(data, block_size);
  }
  else if constexpr (block_size == 2) {
#ifdef _MSC_VER
    auto tmp = _byteswap_ushort(*(uint16_t*)data);
    writer.write(&tmp, block_size);
#elif defined(__clang__) || defined(__GNUC__)
    auto tmp = __builtin_bswap16(*(uint16_t*)data);
    writer.write((char*)&tmp, block_size);
#else
    writer.write(data + 1, 1);
    writer.write(data, 1);
#endif
  }
  else if constexpr (block_size == 4) {
#ifdef _MSC_VER
    auto tmp = _byteswap_ulong(*(uint32_t*)data);
    writer.write(&tmp, block_size);
#elif defined(__clang__) || defined(__GNUC__)
    auto tmp = __builtin_bswap32(*(uint32_t*)data);
    writer.write((char*)&tmp, block_size);
#else
    writer.write(data + 3, 1);
    writer.write(data + 2, 1);
    writer.write(data + 1, 1);
    writer.write(data, 1);
#endif
  }
  else if constexpr (block_size == 8) {
#ifdef _MSC_VER
    auto tmp = _byteswap_uint64(*(uint64_t*)data);
    writer.write(&tmp, block_size);
#elif defined(__clang__) || defined(__GNUC__)
    auto tmp = __builtin_bswap64(*(uint64_t*)data);
    writer.write((char*)&tmp, block_size);
#else
    writer.write(data + 7, 1);
    writer.write(data + 6, 1);
    writer.write(data + 5, 1);
    writer.write(data + 4, 1);
    writer.write(data + 3, 1);
    writer.write(data + 2, 1);
    writer.write(data + 1, 1);
    writer.write(data, 1);
#endif
  }
  else if constexpr (block_size == 16) {
#ifdef _MSC_VER
    auto tmp1 = _byteswap_uint64(*(uint64_t*)data), tmp2 = _byteswap_uint64(*(uint64_t*)(data + 8));
    writer.write(&tmp2, block_size);
    writer.write(&tmp1, block_size);
#elif defined(__clang__) || defined(__GNUC__)
    auto tmp1 = __builtin_bswap64(*data), tmp2 = __builtin_bswap64(*data + 8);
    writer.write((char*)&tmp2, 8);
    writer.write((char*)&tmp1, 8);
#else
    writer.write(data + 15, 1);
    writer.write(data + 14, 1);
    writer.write(data + 13, 1);
    writer.write(data + 12, 1);
    writer.write(data + 11, 1);
    writer.write(data + 10, 1);
    writer.write(data + 9, 1);
    writer.write(data + 8, 1);
    writer.write(data + 7, 1);
    writer.write(data + 6, 1);
    writer.write(data + 5, 1);
    writer.write(data + 4, 1);
    writer.write(data + 3, 1);
    writer.write(data + 2, 1);
    writer.write(data + 1, 1);
    writer.write(data, 1);
#endif
  }
  else {
    static_assert(!sizeof(writer), "illegal block size(should be 1,2,4,8,16)");
  }
}
template <std::size_t block_size, typename writer_t>
void write_wrapper(writer_t& writer, const char* data,
                   std::size_t block_count) {
  if constexpr (is_system_little_endian || block_size == 1) {
    auto total_sz = block_count * block_size;
    if SP_UNLIKELY (total_sz >= PTRDIFF_MAX)
      unreachable();
    else
      writer.write(data, total_sz);
  }
  else {
    static_assert(!sizeof(writer_t), "illegal use");
  }
}
template <std::size_t block_size, typename reader_t>
bool read_wrapper(reader_t& reader, char* SP_RESTRICT data) {
  if constexpr (is_system_little_endian || block_size == 1) {
    return static_cast<bool>(reader.read(data, block_size));
  }
  else {
    std::array<char, block_size> tmp;
    bool res = static_cast<bool>(reader.read((char*)&tmp, block_size));
    if SP_UNLIKELY (!res) {
      return res;
    }
    if constexpr (block_size == 2) {
#ifdef _MSC_VER
      *(uint16_t*)data = _byteswap_ushort(*(uint16_t*)&tmp);
#elif defined(__clang__) || defined(__GNUC__)
      *(uint16_t*)data = __builtin_bswap16(*(uint16_t*)&tmp);
#else
      data[0] = tmp[1];
      data[1] = tmp[0];
#endif
    }
    else if constexpr (block_size == 4) {
#ifdef _MSC_VER
      *(uint32_t*)data = _byteswap_ulong(*(uint32_t*)&tmp);
#elif defined(__clang__) || defined(__GNUC__)
      *(uint32_t*)data = __builtin_bswap32(*(uint32_t*)&tmp);
#else
      data[0] = tmp[3];
      data[1] = tmp[2];
      data[2] = tmp[1];
      data[3] = tmp[0];
#endif
    }
    else if constexpr (block_size == 8) {
#ifdef _MSC_VER
      *(uint64_t*)data = _byteswap_uint64(*(uint64_t*)&tmp);
#elif defined(__clang__) || defined(__GNUC__)
      *(uint64_t*)data = __builtin_bswap64(*(uint64_t*)&tmp);
#else
      data[0] = tmp[7];
      data[1] = tmp[6];
      data[2] = tmp[5];
      data[3] = tmp[4];
      data[4] = tmp[3];
      data[5] = tmp[2];
      data[6] = tmp[1];
      data[7] = tmp[0];
#endif
    }
    else if constexpr (block_size == 16) {
#ifdef _MSC_VER
      *(uint64_t*)(data + 8) = _byteswap_uint64(*(uint64_t*)&tmp);
      *(uint64_t*)data = _byteswap_uint64(*(uint64_t*)(&tmp + 8));
#elif defined(__clang__) || defined(__GNUC__)
      *(uint64_t*)(data + 8) = __builtin_bswap64(*(uint64_t*)&tmp);
      *(uint64_t*)data = __builtin_bswap64(*(uint64_t*)(&tmp + 8));
#else
      data[0] = tmp[15];
      data[1] = tmp[14];
      data[2] = tmp[13];
      data[3] = tmp[12];
      data[4] = tmp[11];
      data[5] = tmp[10];
      data[6] = tmp[9];
      data[7] = tmp[8];
      data[8] = tmp[7];
      data[9] = tmp[6];
      data[10] = tmp[5];
      data[11] = tmp[4];
      data[12] = tmp[3];
      data[13] = tmp[2];
      data[14] = tmp[1];
      data[15] = tmp[0];
#endif
    }
    else {
      static_assert(!sizeof(reader),
                    "illegal block size(should be 1,2,4,8,16)");
    }
    return true;
  }
}
template <std::size_t block_size, typename reader_t>
bool read_wrapper(reader_t& reader, char* SP_RESTRICT data,
                  std::size_t block_count) {
  if constexpr (is_system_little_endian || block_size == 1) {
    auto total_sz = block_count * block_size;
    if SP_UNLIKELY (total_sz >= PTRDIFF_MAX)
      unreachable();
    else
      return static_cast<bool>(reader.read(data, total_sz));
  }
  else {
    static_assert(!sizeof(reader_t), "illegal use");
  }
}
};  // namespace struct_pack::detail