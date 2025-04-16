#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ylt {

namespace common {
    inline std::vector<std::string_view> simd_str_split(std::string_view s,
        const char delim) {
        size_t start = 0;
        size_t end = s.find_first_of(delim);

        std::vector<std::string_view> output;

        while (end <= std::string_view::npos) {
            output.emplace_back(s.substr(start, end - start));

            if (end == std::string_view::npos)
            break;

            start = end + 1;
            end = s.find_first_of(delim, start);
        }

        return output;
    }
}

}