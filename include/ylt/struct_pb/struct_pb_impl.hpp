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
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "ylt/struct_pb.hpp"

namespace struct_pb {

namespace internal {
STRUCT_PB_NODISCARD STRUCT_PB_INLINE uint32_t encode_zigzag(int32_t v) {
  return (static_cast<uint32_t>(v) << 1U) ^
         static_cast<uint32_t>(
             -static_cast<int32_t>(static_cast<uint32_t>(v) >> 31U));
}
STRUCT_PB_NODISCARD STRUCT_PB_INLINE uint64_t encode_zigzag(int64_t v) {
  return (static_cast<uint64_t>(v) << 1U) ^
         static_cast<uint64_t>(
             -static_cast<int64_t>(static_cast<uint64_t>(v) >> 63U));
}

STRUCT_PB_NODISCARD STRUCT_PB_INLINE int64_t decode_zigzag(uint64_t u) {
  return static_cast<int64_t>((u >> 1U)) ^
         static_cast<uint64_t>(-static_cast<int64_t>(u & 1U));
}
STRUCT_PB_NODISCARD STRUCT_PB_INLINE int64_t decode_zigzag(uint32_t u) {
  return static_cast<int64_t>((u >> 1U)) ^
         static_cast<uint64_t>(-static_cast<int64_t>(u & 1U));
}

STRUCT_PB_NODISCARD STRUCT_PB_INLINE std::size_t calculate_varint_size(
    uint64_t v) {
  std::size_t ret = 0;
  do {
    ret++;
    v >>= 7;
  } while (v != 0);
  return ret;
}

[[nodiscard]] STRUCT_PB_INLINE bool decode_varint(const char* data,
                                                  std::size_t& pos_,
                                                  std::size_t size_,
                                                  uint64_t& v) {
  // fix test failed on arm due to different char definition
  const signed char* data_ = reinterpret_cast<const signed char*>(data);
  // from https://github.com/facebook/folly/blob/main/folly/Varint.h
  if (pos_ < size_ && (static_cast<uint64_t>(data_[pos_]) & 0x80U) == 0) {
    v = static_cast<uint64_t>(data_[pos_]);
    pos_++;
    return true;
  }
  constexpr const int8_t max_varint_length = sizeof(uint64_t) * 8 / 7 + 1;
  uint64_t val = 0;
  if (size_ - pos_ >= max_varint_length) [[likely]] {
    do {
      // clang-format off
        int64_t b = data_[pos_++];
                           val  = ((uint64_t(b) & 0x7fU)       ); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) <<  7U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 14U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 21U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 28U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 35U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 42U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 49U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 56U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x01U) << 63U); if (b >= 0) { break; }
      // clang-format on
      return false;
    } while (false);
  }
  else {
    unsigned int shift = 0;
    while (pos_ != size_ && int64_t(data_[pos_]) < 0) {
      val |= (uint64_t(data_[pos_++]) & 0x7fU) << shift;
      shift += 7;
    }
    if (pos_ == size_) {
      return false;
    }
    val |= uint64_t(data_[pos_++]) << shift;
  }
  v = val;
  return true;
}
STRUCT_PB_INLINE void serialize_varint(char* const data, std::size_t& pos,
                                       std::size_t size, uint64_t v) {
  while (v >= 0x80) {
    assert(pos < size);
    data[pos++] = static_cast<uint8_t>(v | 0x80);
    v >>= 7;
  }
  data[pos++] = static_cast<uint8_t>(v);
}
STRUCT_PB_NODISCARD STRUCT_PB_INLINE bool deserialize_varint(const char* data,
                                                             std::size_t& pos,
                                                             std::size_t size,
                                                             uint64_t& v) {
  return decode_varint(data, pos, size, v);
}
STRUCT_PB_NODISCARD STRUCT_PB_INLINE bool read_tag(const char* data,
                                                   std::size_t& pos,
                                                   std::size_t size,
                                                   uint64_t& tag) {
  return deserialize_varint(data, pos, size, tag);
}
inline bool deserialize_unknown(const char* data, std::size_t& pos,
                                std::size_t size, uint32_t tag,
                                UnknownFields& unknown_fields) {
  uint32_t field_number = tag >> 3;
  if (field_number == 0) {
    return false;
  }
  auto offset = internal::calculate_varint_size(tag);
  auto start = pos - offset;
  uint32_t wire_type = tag & 0b0000'0111;
  switch (wire_type) {
    case 0: {
      uint64_t t;
      auto ok = internal::deserialize_varint(data, pos, size, t);
      if (!ok) [[unlikely]] {
        return false;
      }
      unknown_fields.add_field(data, start, pos);
      break;
    }
    case 1: {
      static_assert(sizeof(double) == 8);
      if (pos + 8 > size) [[unlikely]] {
        return false;
      }
      pos += 8;
      unknown_fields.add_field(data, start, pos);
      break;
    }
    case 2: {
      uint64_t sz;
      auto ok = internal::deserialize_varint(data, pos, size, sz);
      if (!ok) [[unlikely]] {
        return false;
      }
      if (pos + sz > size) [[unlikely]] {
        return false;
      }
      pos += sz;
      unknown_fields.add_field(data, start, pos);
      break;
    }
    case 5: {
      static_assert(sizeof(float) == 4);
      if (pos + 4 > size) [[unlikely]] {
        return false;
      }
      pos += 4;
      unknown_fields.add_field(data, start, pos);
      break;
    }
    default: {
      assert(false && "error path");
    }
  }
  return true;
}
}  // namespace internal

}  // namespace struct_pb