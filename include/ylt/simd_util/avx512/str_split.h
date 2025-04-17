#pragma once
#include <immintrin.h>

#include <string>
#include <string_view>
#include <vector>
namespace ylt {
namespace avx512 {

template <typename T>
concept StringLike =
    std::same_as<T, std::string> || std::same_as<T, std::string_view>;
template <StringLike StringLike>
__attribute__((__target__("avx512bw,bmi")))
// auto chose target
inline std::vector<StringLike>
simd_str_split(std::string_view string, const char delim) {
  auto* pstr = string.data();
  size_t size = string.size();
  size_t start = 0;

  std::vector<StringLike> output;

  size_t aligned64_size = size & 0xFFFFFFFFFFFFFFC0UL;
  for (size_t i = 0; i < aligned64_size; i += 64) {
    __m512i data = _mm512_loadu_si512(&pstr[i]);
    const __m512i match = _mm512_set1_epi8(delim);
    uint64_t mask = _mm512_cmpeq_epi8_mask(data, match);
    while (mask != 0) {
      auto j = __builtin_ctzll(mask);
      output.emplace_back(&pstr[start], i + j - start);
      start = i + j + 1;
      mask &= mask - 1;
    }
  }

  size_t aligned32_size = size & 0xFFFFFFFFFFFFFFE0UL;
  if (aligned64_size < aligned32_size) {
    __m256i data = _mm256_lddqu_si256(
        reinterpret_cast<const __m256i*>(&pstr[aligned64_size]));
    const __m256i match = _mm256_set1_epi8(delim);
    uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(data, match));
    while (mask != 0) {
      auto j = __builtin_ctzl(mask);
      output.emplace_back(&pstr[start], aligned64_size + j - start);
      start = aligned64_size + j + 1;
      mask &= mask - 1;
    }
  }

  size_t aligned16_size = size & 0xFFFFFFFFFFFFFFF0UL;
  if (aligned32_size < aligned16_size) {
    __m128i data = _mm_lddqu_si128(
        reinterpret_cast<const __m128i*>(&pstr[aligned32_size]));
    const __m128i match = _mm_set1_epi8(delim);
    uint32_t mask = _mm_movemask_epi8(_mm_cmpeq_epi8(data, match));
    while (mask != 0) {
      auto j = __builtin_ctzl(mask);
      output.emplace_back(&pstr[start], aligned32_size + j - start);
      start = aligned32_size + j + 1;
      mask &= mask - 1;
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
}  // namespace avx512
}  // namespace ylt