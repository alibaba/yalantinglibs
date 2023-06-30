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
#include <iguana/xml_reader.hpp>

namespace struct_xml {

template <int Flags = 0, typename T>
inline bool from_xml(T &&t, char *buf) {
  return iguana::from_xml<Flags>(std::forward<T>(t), buf);
}

template <int Flags = 0, typename T>
inline bool from_xml(T &&t, const std::string &str) {
  return from_xml<Flags>(std::forward<T>(t), str.data());
}

template <int Flags = 0, typename T>
inline bool from_xml(T &&t, std::string_view str) {
  return from_xml<Flags>(std::forward<T>(t), str.data());
}

inline std::string get_last_read_err() { return iguana::get_last_read_err(); }

using any_t = iguana::any_t;
}  // namespace struct_xml