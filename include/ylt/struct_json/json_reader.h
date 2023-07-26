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

#include <iguana/json_reader.hpp>
namespace struct_json {
template <typename T, typename It>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end) {
  iguana::from_json(value, it, end);
}

template <typename T, typename It>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end,
                             std::error_code &ec) noexcept {
  iguana::from_json(value, it, end, ec);
}

template <typename T, typename View>
IGUANA_INLINE void from_json(T &value, const View &view) {
  iguana::from_json(value, view);
}

template <typename T, typename View>
IGUANA_INLINE void from_json(T &value, const View &view,
                             std::error_code &ec) noexcept {
  iguana::from_json(value, view, ec);
}

template <typename T, typename Byte>
IGUANA_INLINE void from_json(T &value, const Byte *data, size_t size) {
  iguana::from_json(value, data, size);
}

template <typename T, typename Byte>
IGUANA_INLINE void from_json(T &value, const Byte *data, size_t size,
                             std::error_code &ec) noexcept {
  iguana::from_json(value, data, size, ec);
}

template <typename T>
IGUANA_INLINE void from_json_file(T &value, const std::string &filename) {
  iguana::from_json_file(value, filename);
}

template <typename T>
IGUANA_INLINE void from_json_file(T &value, const std::string &filename,
                                  std::error_code &ec) noexcept {
  iguana::from_json_file(value, filename, ec);
}

using numeric_str = iguana::numeric_str;

// dom parse
using jvalue = iguana::jvalue;
using jarray = iguana::jarray;
using jobject = iguana::jarray;

template <typename It>
inline void parse(jvalue &result, It &&it, It &&end) {
  iguana::parse(result, it, end);
}

template <typename It>
inline void parse(jvalue &result, It &&it, It &&end, std::error_code &ec) {
  iguana::parse(result, it, end, ec);
}

template <typename T, typename View>
inline void parse(T &result, const View &view) {
  iguana::parse(result, view);
}

template <typename T, typename View>
inline void parse(T &result, const View &view, std::error_code &ec) {
  iguana::parse(result, view, ec);
}

}  // namespace struct_json