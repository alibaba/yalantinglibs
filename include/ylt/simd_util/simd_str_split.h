#pragma once

#include "ylt_simd_dispatch.h"

#include INCLUDE_ARCH_FILE(str_split.h)

namespace ylt {
YLT_USING_ARCH_FUNC(simd_str_split);

static inline std::vector<std::string> split_str(std::string_view string,
                                                 const char delim) {
  return simd_str_split<std::string>(string, delim);
}

static inline std::vector<std::string_view> split_sv(std::string_view string,
                                                     const char delim) {
  return simd_str_split<std::string_view>(string, delim);
}
}  // namespace ylt