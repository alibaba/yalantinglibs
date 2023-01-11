#pragma once
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#if defined __clang__
#define STRUCT_PB_INLINE __attribute__((always_inline)) inline
#elif defined _MSC_VER
#define STRUCT_PB_INLINE __forceinline
#else
#define STRUCT_PB_INLINE __attribute__((always_inline)) inline
#endif
#define STRUCT_PB_NODISCARD [[nodiscard]]
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

[[nodiscard]] STRUCT_PB_INLINE bool decode_varint_v2(const char* data_,
                                                     std::size_t& pos_,
                                                     std::size_t size_,
                                                     uint64_t& v) {
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
                           val  = ((uint64_t(b) & 0x7fU)       ); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) <<  7U); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 14U); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 21U); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 28U); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 35U); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 42U); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 49U); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 56U); if (!(b & 0b10000000)) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x01U) << 63U); if (!(b & 0b10000000)) { break; }
      // clang-format on
      return false;
    } while (false);
  }
  else {
    unsigned int shift = 0;
    while (pos_ != size_ && (data_[pos_] & 0b10000000)) {
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
  return decode_varint_v2(data, pos, size, v);
}
STRUCT_PB_NODISCARD STRUCT_PB_INLINE bool read_tag(const char* data,
                                                   std::size_t& pos,
                                                   std::size_t size,
                                                   uint64_t& tag) {
  return deserialize_varint(data, pos, size, tag);
}

}  // namespace internal
}  // namespace struct_pb