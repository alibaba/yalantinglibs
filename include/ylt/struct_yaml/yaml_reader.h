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
#include <iguana/yaml_reader.hpp>

namespace struct_yaml {

template <typename T, typename View>
inline void from_yaml(T &value, const View &view) {
  iguana::from_yaml(value, view);
}

template <typename T, typename View>
inline void from_yaml(T &value, const View &view, std::error_code &ec) {
  iguana::from_yaml(value, view, ec);
}
}  // namespace struct_yaml