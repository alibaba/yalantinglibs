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

#include <cinatra/utils.hpp>

namespace ylt {
using time_format = cinatra::time_format;

template <time_format Format = time_format::http_format>
inline std::pair<bool, std::time_t> get_timestamp(
    const std::string &gmt_time_str) {
  return cinatra::get_timestamp<Format>(gmt_time_str);
}

inline std::string get_gmt_time_str(std::time_t t) {
  return cinatra::get_gmt_time_str(t);
}
}  // namespace ylt