/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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
#include <string_view>

namespace coro_rpc {
template <auto func>
consteval std::string_view get_func_name() {
#ifdef __GNUC__
  constexpr std::string_view name = __PRETTY_FUNCTION__;
  constexpr std::string_view f = "func = ";
  size_t start = name.find(f) + f.length();
#if defined(__clang__) || defined(__INTEL_COMPILER)
  size_t end = name.rfind("]");
  constexpr std::string_view anonymous = "(anonymous namespace)::";
#else
  size_t end = name.rfind("; std::string_view = ");
  constexpr std::string_view anonymous = "{anonymous}::";
#endif
  if (name[start] == '&') {
    start += 1;
    if (name[start] == '(' || name[start] == '{') {
      start += anonymous.length();
    }
  }
  else if (name[start] == '{') {
    start += anonymous.length();
  }

  return name.substr(start, end - start);
#elif _MSC_VER
  constexpr std::string_view anonymous = "`anonymous-namespace'::";
  constexpr std::string_view name = __FUNCSIG__;
  std::string_view start_token = "get_func_name<";
  size_t start = name.find(start_token) + start_token.size();
  size_t end = name.rfind(">(void)");
  std::string_view str = name.substr(start, end - start);

  auto left = str.rfind(' ') + 1;
  if (str[left] == '`') {
    left += anonymous.length();
  }
  auto right = str.rfind('(');
  return str.substr(left, right - left);
#else
  static_assert(!sizeof(func), "don't support this compiler");
#endif
};
}  // namespace coro_rpc
