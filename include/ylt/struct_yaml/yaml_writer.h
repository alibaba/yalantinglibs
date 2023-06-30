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
#include <iguana/yaml_writer.hpp>

namespace struct_yaml {
template <typename Stream, typename T>
inline void to_yaml(T &&t, Stream &s, size_t min_spaces = 0) {
  iguana::to_yaml(std::forward<T>(t), s, min_spaces);
}
}  // namespace struct_yaml