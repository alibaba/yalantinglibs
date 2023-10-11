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
#include "reflection.hpp"
#include "ylt/struct_pack/util.h"
namespace struct_pack::detail {

template <std::size_t block_size, writer_t writer_t>
void write_wrapper(writer_t& writer, const char* data) {
  if constexpr (is_system_little_endian || block_size == 1) {
    writer.write(data, block_size);
  }
  else if constexpr (block_size == 2) {
#ifdef _MSC_VER
    auto tmp = _byteswap_ushort(*data);
    writer.write(&tmp, block_size);
#elif defined(__clang__) || defined(__GNUC__)
    auto tmp = __builtin_bswap16(*data);
    writer.write(&tmp, block_size);
#else
    writer.write(data + 1, 1);
    writer.write(data, 1);
#endif
  }
  else if constexpr (block_size == 4) {
#ifdef _MSC_VER
    auto tmp = _byteswap_ulong(*data);
    writer.write(&tmp, block_size);
#elif defined(__clang__) || defined(__GNUC__)
    auto tmp = __builtin_bswap16(*data);
    writer.write(&tmp, block_size);
#else
    writer.write(data + 3, 1);
    writer.write(data + 2, 1);
    writer.write(data + 1, 1);
    writer.write(data, 1);
#endif
  }
  else if constexpr (block_size == 8) {
#ifdef _MSC_VER
    auto tmp = _byteswap_uint64(*data);
    writer.write(&tmp, block_size);
#elif defined(__clang__) || defined(__GNUC__)
    auto tmp = __builtin_bswap16(*data);
    writer.write(&tmp, block_size);
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
    auto tmp1 = _byteswap_uint64(*data), tmp2 = _byteswap_uint64(*data + 8);
    writer.write(&tmp2, block_size);
    writer.write(&tmp1, block_size);
#elif defined(__clang__) || defined(__GNUC__)
    auto tmp1 = __builtin_bswap64(*data), tmp2 = __builtin_bswap64(*data + 8);
    writer.write(&tmp2, block_size);
    writer.write(&tmp1, block_size);
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
    static_assert(sizeof(writer), "illegal block size(should be 1,2,4,8,16)");
  }
}
template <std::size_t block_size, writer_t writer_t>
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
    for (std::size_t i = 0; i < block_count; ++i) {
      write_wrapper<block_size>(writer, data);
      data += block_size;
    }
  }
}
template <std::size_t block_size, reader_t reader_t>
bool read_wrapper(reader_t& reader, char* SP_RESTRICT data) {
  if constexpr (is_system_little_endian || block_size == 1) {
    return static_cast<bool>(reader.read(data, block_size));
  }
  else {
    std::array<char, block_size> tmp;
    bool res = static_cast<bool>(reader.read(&tmp, block_size));
    if SP_UNLIKELY (res) {
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
      static_assert(sizeof(reader), "illegal block size(should be 1,2,4,8,16)");
    }
  }
}
template <std::size_t block_size, reader_t reader_t>
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
    for (std::size_t i = 0; i < block_count; ++i) {
      if SP_UNLIKELY (!read_wrapper<block_size>(reader, data)) {
        return false;
      }
      data += block_size;
    }
  }
}
};  // namespace struct_pack::detail