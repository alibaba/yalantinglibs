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
#include <iguana/util.hpp>
#include <iguana/xml_reader.hpp>

namespace struct_xml {

template <typename T, typename View>
inline void from_xml(T &&t, const View &str) {
  iguana::from_xml(std::forward<T>(t), str);
}

template <typename Num>
inline Num get_number(std::string_view str) {
  return iguana::get_number<Num>(str);
}

template <typename T, typename map_type = std::unordered_map<std::string_view,
                                                             std::string_view>>
using xml_attr_t = iguana::xml_attr_t<T, map_type>;

template <typename T>
inline constexpr std::string_view type_string() {
  return iguana::type_string<T>();
}

template <auto T>
inline constexpr std::string_view enum_string() {
  return iguana::enum_string<T>();
}
}  // namespace struct_xml