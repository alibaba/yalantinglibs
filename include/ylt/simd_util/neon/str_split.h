#pragma once

#include <arm_neon.h>

#include <string>
#include <string_view>
#include <vector>

namespace ylt {
namespace neon {

template <typename T>
concept StringLike =
    std::same_as<T, std::string> || std::same_as<T, std::string_view>;

template <StringLike StringLike>
inline std::vector<StringLike> simd_str_split(std::string_view string,
                                              const char delim) {
  auto* pstr = string.data();
  size_t size = string.size();
  size_t start = 0;

  std::vector<StringLike> output;
  // Similar to memchr implementation, the first round of 256-bit detection
  size_t aligned32_size = size & 0xFFFFFFFFFFFFFFE0UL;
  uint8x16_t match = vmovq_n_u8(delim);
  for (size_t i = 0; i < aligned32_size; i += 32) {
    uint8x16_t data1 = vld1q_u8(reinterpret_cast<const uint8_t*>(&pstr[i]));
    uint8x16_t data2 =
        vld1q_u8(reinterpret_cast<const uint8_t*>(&pstr[i + 16]));
    uint8x16_t result1 = vceqq_u8(data1, match);
    uint8x16_t result2 = vceqq_u8(data2, match);
    // Quickly fold the 256-bit detection results to 64-bit. It cannot be
    // accurately located, but can be used for quick skipping.
    uint64x2_t result64 = vreinterpretq_u64_u8(vorrq_u8(result1, result2));
    result64 = vpaddq_u64(result64, result64);
    if (result64[0] != 0) {
      // Convert the detection result from 0xFF to 0x01, 0x04, 0x10, 0x40
      // The final fold will form 32 hit marks on 64 bits, and the alternate
      // bits will take effect. For example, if all hits are found, the result
      // will be 0x55555555555555555UL
      uint8x16_t vmask =
          vreinterpretq_u8_u64(vdupq_n_u64(0x4010040140100401UL));
      result1 = vandq_u8(result1, vmask);
      result2 = vandq_u8(result2, vmask);
      result1 = vpaddq_u8(result1, result2);
      result1 = vpaddq_u8(result1, result1);
      uint64_t mask = vreinterpretq_u64_u8(result1)[0];
      while (mask != 0) {
        auto j = __builtin_ctzll(mask) >> 1;
        output.emplace_back(&pstr[start], i + j - start);
        start = i + j + 1;
        mask &= mask - 1;
      }
    }
  }
  size_t aligned16_size = size & 0xFFFFFFFFFFFFFFF0UL;
  if (aligned32_size < aligned16_size) {
    uint8x16_t data =
        vld1q_u8(reinterpret_cast<const uint8_t*>(&pstr[aligned32_size]));
    uint8x16_t result = vceqq_u8(data, match);
    uint64x2_t result64 = vreinterpretq_u64_u8(result);
    result64 = vpaddq_u64(result64, result64);
    if (result64[0] != 0) {
      uint8x16_t vmask =
          vreinterpretq_u8_u64(vdupq_n_u64(0x4010040140100401UL));
      result = vandq_u8(result, vmask);
      result = vpaddq_u8(result, result);
      result = vpaddq_u8(result, result);
      uint32_t mask = vreinterpretq_u32_u8(result)[0];
      while (mask != 0) {
        auto j = __builtin_ctzl(mask) >> 1;
        output.emplace_back(&pstr[start], aligned32_size + j - start);
        start = aligned32_size + j + 1;
        mask &= mask - 1;
      }
    }
  }

  size_t i = aligned16_size;
  do {
    while (pstr[i] != delim && i != size) {
      ++i;
    }
    output.emplace_back(&pstr[start], i - start);
    start = i = i + 1;
  } while (i <= size);
  return output;
}

}  // namespace neon
}  // namespace ylt