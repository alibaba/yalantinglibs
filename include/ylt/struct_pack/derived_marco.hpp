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

#include "foreach_macro.h"

#pragma once
#define GET_STRUCT_PACK_ID_IMPL(type)                            \
  inline uint32_t type::get_struct_pack_id() const {             \
    return struct_pack::detail::get_struct_pack_id_impl<type>(); \
  }

#define GET_STRUCT_PACK_ID_IMPL_FOR_LOOP(idx, type)              \
  inline uint32_t type::get_struct_pack_id() const {             \
    return struct_pack::detail::get_struct_pack_id_impl<type>(); \
  }

#define STRUCT_PACK_DERIVED_IMPL(base, ...)                                    \
                                                                               \
  inline decltype(struct_pack::detail::derived_decl_impl<base, __VA_ARGS__>()) \
  struct_pack_derived_decl(base*);

#define STRUCT_PACK_DERIVED_DECL(base, ...)                         \
  STRUCT_PACK_EXPAND_EACH(, GET_STRUCT_PACK_ID_IMPL_FOR_LOOP, base, \
                          __VA_ARGS__)                              \
  STRUCT_PACK_DERIVED_IMPL(base, __VA_ARGS__)
