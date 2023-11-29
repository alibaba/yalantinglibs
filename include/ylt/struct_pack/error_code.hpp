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

#include <system_error>

namespace struct_pack {
enum class errc {
  ok = 0,
  no_buffer_space,
  invalid_buffer,
  hash_conflict,
  invalid_width_of_container_length,
};

namespace detail {

class struct_pack_category : public std::error_category {
 public:
  virtual const char *name() const noexcept override {
    return "struct_pack::category";
  }

  virtual std::string message(int err_val) const override {
    switch (static_cast<errc>(err_val)) {
      case errc::ok:
        return "ok";
      case errc::no_buffer_space:
        return "no buffer space";
      case errc::invalid_buffer:
        return "invalid argument";
      case errc::hash_conflict:
        return "hash conflict";

      default:
        return "(unrecognized error)";
    }
  }
};

inline const std::error_category &category() {
  static struct_pack::detail::struct_pack_category instance;
  return instance;
}
}  // namespace detail

}  // namespace struct_pack