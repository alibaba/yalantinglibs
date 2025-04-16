#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ylt {

namespace common {
    template<typename T>
    concept StringLike = 
        std::same_as<T, std::string> || 
        std::same_as<T, std::string_view>;
    template<StringLike StringLike>
    inline std::vector<StringLike> simd_str_split(std::string_view s, const char delimiter) {
        size_t start = 0;
        size_t end = s.find_first_of(delimiter);

        std::vector<StringLike> output;

        while (end <= StringLike::npos) {
            output.emplace_back(s.substr(start, end - start));

            if (end == StringLike::npos)
                break;

            start = end + 1;
            end = s.find_first_of(delimiter, start);
        }

        return output;
    }
}

}