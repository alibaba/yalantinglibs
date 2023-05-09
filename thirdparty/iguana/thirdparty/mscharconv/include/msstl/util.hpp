// Copyright (c) 2021 Borislav Stanimirov
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#pragma once

#include <string_view>

namespace msstl::util {

template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
bool from_string(std::string_view str, T &out, int base = 10) {
  auto end = str.data() + str.length();
  auto ret = from_chars(str.data(), end, out, base);
  if (ret.ec != std::errc{})
    return false;
  return ret.ptr == end;
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
bool from_string(std::string_view str, T &out) {
  auto end = str.data() + str.length();
  auto ret = from_chars(str.data(), end, out);
  if (ret.ec != std::errc{})
    return false;
  return ret.ptr == end;
}

} // namespace msstl::util
