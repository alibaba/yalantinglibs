#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <immintrin.h>



namespace ylt {
namespace sse {

    template<typename T>
    concept StringLike = 
        std::same_as<T, std::string> || 
        std::same_as<T, std::string_view>;
    template<StringLike StringLike>
    __attribute__((__target__("sse4.2")))
    inline std::vector<StringLike> simd_str_split(std::string_view string, const char delim) {
        auto* pstr = string.data();
        size_t size = string.size();
        size_t start = 0;
    
        std::vector<StringLike> output;
        size_t aligned16_size = size & 0xFFFFFFFFFFFFFFF0UL;

        for (size_t i = 0; i < aligned16_size; i += 16) {
            __m128i data = _mm_lddqu_si128(reinterpret_cast<const __m128i*>(&pstr[i]));
            const __m128i match = _mm_set1_epi8(delim);
            uint32_t mask = _mm_movemask_epi8(_mm_cmpeq_epi8(data, match));
            while (mask != 0) {
                auto j = __builtin_ctzl(mask);
                output.emplace_back(&pstr[start], i + j - start);
                start = i + j + 1;
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
    
}
}