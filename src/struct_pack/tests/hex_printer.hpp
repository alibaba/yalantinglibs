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
#include <iostream>
#include <string>
// useful for debugging
inline void print_hex(uint8_t v) {
  static std::string hex = "0123456789abcedf";
  if (v < 16) {
    std::cout << "0";
    std::cout << hex[v];
  }
  else {
    auto first = v / 16;
    auto last = v % 16;
    std::cout << hex[first] << hex[last];
  }
}
inline void print_hex(std::string_view sv) {
  for (char ch : sv) {
    uint8_t v = ch;
    print_hex(v);
    std::cout << " ";
  }
  std::cout << std::endl;
}
struct hex_helper {
  hex_helper(std::string_view sv) : sv(sv) {}
  friend std::ostream& operator<<(std::ostream& os, const hex_helper& helper) {
    static std::string hex = "0123456789abcedf";
    auto to_hex = [&os](std::string_view hex, uint8_t v) {
      if (v < 16) {
        os << "0";
        os << hex[v];
      }
      else {
        auto first = v / 16;
        auto last = v % 16;
        os << hex[first] << hex[last];
      }
    };
    for (char ch : helper.sv) {
      uint8_t v = ch;
      to_hex(hex, v);
      os << " ";
    }
    return os;
  }
  bool operator==(const hex_helper&) const = default;
  std::string_view sv;
};