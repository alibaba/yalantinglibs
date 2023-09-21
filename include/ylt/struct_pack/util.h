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
#include <array>

#include "marco.h"
namespace struct_pack::detail {

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

[[noreturn]] inline void unreachable() {
  // Uses compiler specific extensions if possible.
  // Even if no extension is used, undefined behavior is still raised by
  // an empty function body and the noreturn attribute.
#ifdef __GNUC__  // GCC, Clang, ICC
  __builtin_unreachable();
#elif defined(_MSC_VER)  // msvc
  __assume(false);
#endif
}

template <typename T>
[[noreturn]] constexpr T declval() {
  unreachable();
}

template <typename T, std::size_t sz>
constexpr void STRUCT_PACK_INLINE compile_time_sort(std::array<T, sz> &array) {
  // FIXME: use faster compile-time sort
  for (std::size_t i = 0; i < array.size(); ++i) {
    for (std::size_t j = i + 1; j < array.size(); ++j) {
      if (array[i] > array[j]) {
        auto tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
      }
    }
  }
  return;
}

template <typename T, std::size_t sz>
constexpr std::size_t STRUCT_PACK_INLINE
calculate_uniqued_size(const std::array<T, sz> &input) {
  std::size_t unique_cnt = sz;
  for (std::size_t i = 1; i < input.size(); ++i) {
    if (input[i] == input[i - 1]) {
      --unique_cnt;
    }
  }
  return unique_cnt;
}

template <typename T, std::size_t sz1, std::size_t sz2>
constexpr void STRUCT_PACK_INLINE compile_time_unique(
    const std::array<T, sz1> &input, std::array<T, sz2> &output) {
  std::size_t j = 0;
  static_assert(sz1 != 0, "not allow empty input!");
  output[0] = input[0];
  for (std::size_t i = 1; i < input.size(); ++i) {
    if (input[i] != input[i - 1]) {
      output[++j] = input[i];
    }
  }
}
}  // namespace struct_pack::detail