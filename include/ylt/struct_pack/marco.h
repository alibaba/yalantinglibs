#pragma once
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
#if defined __clang__
#define STRUCT_PACK_INLINE __attribute__((always_inline)) inline
#define CONSTEXPR_INLINE_LAMBDA __attribute__((always_inline)) constexpr
#elif defined _MSC_VER
#define STRUCT_PACK_INLINE __forceinline
#define CONSTEXPR_INLINE_LAMBDA constexpr
#else
#define STRUCT_PACK_INLINE __attribute__((always_inline)) inline
#define CONSTEXPR_INLINE_LAMBDA constexpr __attribute__((always_inline))
#endif

#if defined STRUCT_PACK_OPTIMIZE
#define STRUCT_PACK_MAY_INLINE STRUCT_PACK_INLINE
#else
#define STRUCT_PACK_MAY_INLINE inline
#endif