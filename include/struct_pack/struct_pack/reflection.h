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
#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

#if __has_include(<span>)
#include <span>
#endif

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

namespace struct_pack {

template <typename T>
constexpr std::size_t members_count = 0;
template <typename T>
constexpr std::size_t min_alignment = 0;

namespace detail {
template <typename Type>
concept deserialize_view = requires(Type container) {
  container.size();
  container.data();
};

template <typename Type>
concept container_adapter = requires(Type container) {
  typename std::remove_cvref_t<Type>::value_type;
  container.size();
  container.pop();
};

template <typename Type>
concept container = requires(Type container) {
  typename std::remove_cvref_t<Type>::value_type;
  container.size();
  container.begin();
  container.end();
};

template <typename Type>
concept is_char_t = std::is_same_v<Type, signed char> ||
    std::is_same_v<Type, char> || std::is_same_v<Type, unsigned char> ||
    std::is_same_v<Type, wchar_t> || std::is_same_v<Type, char16_t> ||
    std::is_same_v<Type, char32_t> || std::is_same_v<Type, char8_t>;

template <typename Type>
concept string = container<Type> && requires(Type container) {
  requires is_char_t<typename std::remove_cvref_t<Type>::value_type>;
  container.length();
  container.data();
};

template <typename Type>
concept string_view = string<Type> && !requires(Type container) {
  container.resize(std::size_t{});
};

#if __cpp_lib_span >= 202002L

template <typename Type>
concept continuous_container = string<Type> ||
    (container<Type> &&requires(Type container) {
      std::span{container};
      container.resize(std::size_t{});
    });

#else

#include <string>
#include <vector>

template <typename Type>
constexpr inline bool is_std_basic_string_v = false;

template <typename... args>
constexpr inline bool is_std_basic_string_v<std::basic_string<args...>> = true;

template <typename Type>
constexpr inline bool is_std_vector_v = false;

template <typename... args>
constexpr inline bool is_std_vector_v<std::vector<args...>> = true;

template <typename Type>
concept continuous_container = container<Type> &&
    (is_std_vector_v<Type> || is_std_basic_string_v<Type>);
#endif

template <typename Type>
concept map_container = container<Type> && requires(Type container) {
  typename std::remove_cvref_t<Type>::mapped_type;
};

template <typename Type>
concept set_container = container<Type> && requires(Type container) {
  typename std::remove_cvref_t<Type>::key_type;
};

template <typename Type>
concept tuple = requires(Type tuple) {
  std::get<0>(tuple);
  sizeof(std::tuple_size<std::remove_cvref_t<Type>>);
};

template <typename Type>
concept tuple_size = requires(Type tuple) {
  sizeof(std::tuple_size<std::remove_cvref_t<Type>>);
};

template <typename Type>
concept array = requires(Type arr) {
  arr.size();
  std::tuple_size<std::remove_cvref_t<Type>>{};
};

// this version not work, can't checkout the is_xx_v in ```require(Type){...}```
template <typename Type>
concept c_array1 = requires(Type arr) {
  std::is_array_v<Type> == true;
};

template <class T>
concept c_array = std::is_array_v<T> && std::extent_v<std::remove_cvref_t<T>> >
0;

template <typename Type>
concept pair = requires(Type p) {
  typename std::remove_cvref_t<Type>::first_type;
  typename std::remove_cvref_t<Type>::second_type;
  p.first;
  p.second;
};

template <typename Type>
concept expected = requires(Type e) {
  typename std::remove_cvref_t<Type>::value_type;
  typename std::remove_cvref_t<Type>::error_type;
  typename std::remove_cvref_t<Type>::unexpected_type;
  e.has_value();
  e.error();
  requires std::is_same_v<void,
                          typename std::remove_cvref_t<Type>::value_type> ||
      requires(Type e) {
    e.value();
  };
};

template <typename Type>
concept optional = !expected<Type> && requires(Type optional) {
  optional.value();
  optional.has_value();
  optional.operator*();
  typename std::remove_cvref_t<Type>::value_type;
};

template <typename Type>
concept unique_ptr = requires(Type ptr) {
  ptr.operator*();
  typename std::remove_cvref_t<Type>::element_type;
}
&&!requires(Type ptr, Type ptr2) { ptr = ptr2; };

template <typename Type>
constexpr inline bool is_variant_v = false;

template <typename... args>
constexpr inline bool is_variant_v<std::variant<args...>> = true;

template <typename T>
concept variant = is_variant_v<T>;

struct UniversalType {
  template <typename T>
  operator T();
};

template <typename T>
concept integral = std::is_integral_v<T>;

struct UniversalIntegralType {
  template <integral T>
  operator T();
};

struct UniversalOptionalType {
  template <optional U>
  operator U();
};

struct UniversalNullptrType {
  operator std::nullptr_t();
};

template <typename T, typename... Args>
consteval std::size_t member_count_impl() {
  if constexpr (requires { T{{Args{}}..., {UniversalType{}}}; } == true) {
    return member_count_impl<T, Args..., UniversalType>();
  }
  else if constexpr (requires {
                       T{{Args{}}..., {UniversalOptionalType{}}};
                     } == true) {
    return member_count_impl<T, Args..., UniversalOptionalType>();
  }
  else if constexpr (requires {
                       T{{Args{}}..., {UniversalIntegralType{}}};
                     } == true) {
    return member_count_impl<T, Args..., UniversalIntegralType>();
  }
  else if constexpr (requires {
                       T{{Args{}}..., {UniversalNullptrType{}}};
                     } == true) {
    return member_count_impl<T, Args..., UniversalNullptrType>();
  }
  else {
    return sizeof...(Args);
  }
}

template <typename T>
consteval std::size_t member_count() {
  if constexpr (struct_pack::members_count < T >> 0) {
    return struct_pack::members_count<T>;
  }
  else if constexpr (tuple_size<T>) {
    return std::tuple_size<T>::value;
  }
  else {
    return member_count_impl<T>();
  }
}
// add extension like `members_count`
template <typename T>
consteval std::size_t min_align() {
  if constexpr (struct_pack::min_alignment < T >> 0) {
    return struct_pack::min_alignment<T>;
  }
  else {
    // don't use \0 as 0
    // due to \0 is a special flag for struct_pack
    // '0' ascii code is 48
    return '0';
  }
}
// similar to `min_align`
template <typename T>
consteval std::size_t max_align() {
  static_assert(std::alignment_of_v<T> > 0);
  return std::alignment_of_v<T>;
}

constexpr static auto MaxVisitMembers = 64;

constexpr decltype(auto) STRUCT_PACK_INLINE visit_members(auto &&object,
                                                          auto &&visitor) {
  using type = std::remove_cvref_t<decltype(object)>;
  constexpr auto Count = member_count<type>();
  if constexpr (Count == 0 && std::is_class_v<type> &&
                !std::is_same_v<type, std::monostate>) {
    static_assert(!sizeof(type), "empty struct/class is not allowed!");
  }
  static_assert(Count <= MaxVisitMembers, "exceed max visit members");
  // If you see any structured binding error in the follow line, it
  // means struct_pack can't calculate your struct's members count
  // correctly. You need to mark it manually.
  //
  // For example, there is a struct named Hello,
  // and it has 3 members.
  //
  // You can mark it as:
  //
  // template <>
  // constexpr size_t struct_pack::members_count<Hello> = 3;

  if constexpr (Count == 0) {
    return visitor();
  }
  else if constexpr (Count == 1) {
    auto &&[a1] = object;
    return visitor(a1);
  }
  else if constexpr (Count == 2) {
    auto &&[a1, a2] = object;
    return visitor(a1, a2);
  }
  else if constexpr (Count == 3) {
    auto &&[a1, a2, a3] = object;
    return visitor(a1, a2, a3);
  }
  else if constexpr (Count == 4) {
    auto &&[a1, a2, a3, a4] = object;
    return visitor(a1, a2, a3, a4);
  }
  else if constexpr (Count == 5) {
    auto &&[a1, a2, a3, a4, a5] = object;
    return visitor(a1, a2, a3, a4, a5);
  }
  else if constexpr (Count == 6) {
    auto &&[a1, a2, a3, a4, a5, a6] = object;
    return visitor(a1, a2, a3, a4, a5, a6);
  }
  else if constexpr (Count == 7) {
    auto &&[a1, a2, a3, a4, a5, a6, a7] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7);
  }
  else if constexpr (Count == 8) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8);
  }
  else if constexpr (Count == 9) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9);
  }
  else if constexpr (Count == 10) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  }
  else if constexpr (Count == 11) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
  }
  else if constexpr (Count == 12) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
  }
  else if constexpr (Count == 13) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
  }
  else if constexpr (Count == 14) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] =
        object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
  }
  else if constexpr (Count == 15) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] =
        object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15);
  }
  else if constexpr (Count == 16) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16);
  }
  else if constexpr (Count == 17) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17);
  }
  else if constexpr (Count == 18) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18);
  }
  else if constexpr (Count == 19) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19);
  }
  else if constexpr (Count == 20) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20);
  }
  else if constexpr (Count == 21) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21);
  }
  else if constexpr (Count == 22) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22);
  }
  else if constexpr (Count == 23) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23);
  }
  else if constexpr (Count == 24) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24);
  }
  else if constexpr (Count == 25) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25);
  }
  else if constexpr (Count == 26) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26);
  }
  else if constexpr (Count == 27) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] =
        object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27);
  }
  else if constexpr (Count == 28) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] =
        object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28);
  }
  else if constexpr (Count == 29) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29);
  }
  else if constexpr (Count == 30) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30);
  }
  else if constexpr (Count == 31) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31);
  }
  else if constexpr (Count == 32) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32);
  }
  else if constexpr (Count == 33) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33);
  }
  else if constexpr (Count == 34) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34);
  }
  else if constexpr (Count == 35) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35);
  }
  else if constexpr (Count == 36) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36);
  }
  else if constexpr (Count == 37) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37);
  }
  else if constexpr (Count == 38) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38);
  }
  else if constexpr (Count == 39) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39);
  }
  else if constexpr (Count == 40) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] =
        object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40);
  }
  else if constexpr (Count == 41) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] =
        object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41);
  }
  else if constexpr (Count == 42) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42);
  }
  else if constexpr (Count == 43) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43);
  }
  else if constexpr (Count == 44) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44);
  }
  else if constexpr (Count == 45) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45);
  }
  else if constexpr (Count == 46) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46);
  }
  else if constexpr (Count == 47) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47);
  }
  else if constexpr (Count == 48) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48);
  }
  else if constexpr (Count == 49) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49);
  }
  else if constexpr (Count == 50) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50);
  }
  else if constexpr (Count == 51) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51);
  }
  else if constexpr (Count == 52) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52);
  }
  else if constexpr (Count == 53) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53] =
        object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53);
  }
  else if constexpr (Count == 54) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54] =
        object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54);
  }
  else if constexpr (Count == 55) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55);
  }
  else if constexpr (Count == 56) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56);
  }
  else if constexpr (Count == 57) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56, a57] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56, a57);
  }
  else if constexpr (Count == 58) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56, a57, a58] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56, a57, a58);
  }
  else if constexpr (Count == 59) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56, a57, a58, a59] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56, a57, a58, a59);
  }
  else if constexpr (Count == 60) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56, a57, a58, a59, a60] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56, a57, a58, a59, a60);
  }
  else if constexpr (Count == 61) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56, a57, a58, a59, a60, a61] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61);
  }
  else if constexpr (Count == 62) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56, a57, a58, a59, a60, a61, a62] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62);
  }
  else if constexpr (Count == 63) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56, a57, a58, a59, a60, a61, a62, a63] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62,
                   a63);
  }
  else if constexpr (Count == 64) {
    auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
            a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
            a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
            a55, a56, a57, a58, a59, a60, a61, a62, a63, a64] = object;
    return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
                   a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26,
                   a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38,
                   a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50,
                   a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62,
                   a63, a64);
  }
}

template <template <size_t index, typename...> typename Func, typename... Args>
constexpr decltype(auto) STRUCT_PACK_INLINE template_switch(std::size_t index,
                                                            auto &...args) {
  static_assert(index <=256, "index shouldn't bigger than 256");
  return Func<index, Args...>::run(args...);
}
}  // namespace detail
}  // namespace struct_pack
