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
#include <cstring>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

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

#if defined STRUCT_PACK_OPTIMIZE
#define STRUCT_PACK_MAY_INLINE STRUCT_PACK_INLINE
#else
#define STRUCT_PACK_MAY_INLINE inline
#endif

namespace struct_pack {

template <typename T>
constexpr std::size_t members_count = 0;
template <typename T>
constexpr std::size_t min_alignment = 0;

template <typename T>
concept writer_t = requires(T t) {
  t.write((const char *)nullptr, std::size_t{});
};

template <typename T>
concept reader_t = requires(T t) {
  t.read((char *)nullptr, std::size_t{});
  t.ignore(std::size_t{});
  t.tellg();
};

template <typename T>
concept view_reader_t = reader_t<T> && requires(T t) {
  { t.read_view(std::size_t{}) } -> std::convertible_to<const char *>;
};

template <typename T>
concept seek_reader_t = reader_t<T> && requires(T t) {
  t.seekg(std::size_t{});
};

template <typename T, uint64_t version = 0>
struct compatible;

// clang-format off
namespace detail {
  template <typename Type>
  concept deserialize_view = requires(Type container) {
    container.size();
    container.data();
  };

  struct memory_writer {
    char *buffer;
    STRUCT_PACK_INLINE void write(const char *data, std::size_t len) {
      memcpy(buffer, data, len);
      buffer += len;
    }
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


  template <typename Type>
  concept span = container<Type> && requires(Type t) {
    t=Type{(typename Type::value_type*)nullptr ,std::size_t{} };
    t.subspan(std::size_t{},std::size_t{});
  };



  template <typename Type>
  concept dynamic_span = span<Type> && std::is_same_v<std::integral_constant<std::size_t,Type::extent>,std::integral_constant<std::size_t,SIZE_MAX>>;

  template <typename Type>
  concept static_span = span<Type> && !dynamic_span<Type>;


#if __cpp_lib_span >= 202002L

  template <typename Type>
  concept continuous_container =
      string<Type> || (container<Type> && requires(Type container) {
        std::span{container};
      });

#else

#include <string>
#include <vector>

  template <typename Type>
  constexpr inline bool is_std_basic_string_v = false;

  template <typename... args>
  constexpr inline bool is_std_basic_string_v<std::basic_string<args...>> =
      true;

  template <typename Type>
  constexpr inline bool is_std_vector_v = false;

  template <typename... args>
  constexpr inline bool is_std_vector_v<std::vector<args...>> = true;

  template <typename Type>
  concept continuous_container =
      container<Type> &&(is_std_vector_v<Type> || is_std_basic_string_v<Type>);
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

  // this version not work, can't checkout the is_xx_v in
  // ```require(Type){...}```
  template <typename Type>
  concept c_array1 = requires(Type arr) {
    std::is_array_v<Type> == true;
  };

  template <class T>
  concept c_array =
      std::is_array_v<T> && std::extent_v<std::remove_cvref_t<T>> >
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
  concept unique_ptr = requires(Type ptr) {
    ptr.operator*();
    typename std::remove_cvref_t<Type>::element_type;
  }
  &&!requires(Type ptr, Type ptr2) { ptr = ptr2; };

  template <typename Type>
  concept optional = !expected<Type> && requires(Type optional) {
    optional.value();
    optional.has_value();
    optional.operator*();
    typename std::remove_cvref_t<Type>::value_type;
  };


  template <typename Type>
  constexpr inline bool is_compatible_v = false;

  template <typename Type, uint64_t version>
  constexpr inline bool is_compatible_v<compatible<Type,version>> = true;

  template <typename Type>
  constexpr inline bool is_variant_v = false;

  template <typename... args>
  constexpr inline bool is_variant_v<std::variant<args...>> = true;

  template <typename T>
  concept variant = is_variant_v<T>;

  template <typename T>
  concept is_compatible = is_compatible_v<T>;

  struct UniversalVectorType {
    template <typename T>
    operator std::vector<T>();
  };

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

  struct UniversalNullptrType {
    operator std::nullptr_t();
  };

  struct UniversalOptionalType {
    template <optional U>
    operator U();
  };

  struct UniversalCompatibleType {
    template <is_compatible U>
    operator U();
  };

  template <typename T, typename... Args>
  consteval std::size_t member_count_impl() {
    if constexpr (requires { T{{Args{}}..., {UniversalVectorType{}}}; } == true) {
      return member_count_impl<T, Args..., UniversalVectorType>();
    }
    else if constexpr (requires { T{{Args{}}..., {UniversalType{}}}; } == true) {
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
    else if constexpr (requires {
                         T{{Args{}}..., {UniversalCompatibleType{}}};
                       } == true) {
      return member_count_impl<T, Args..., UniversalCompatibleType>();
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
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14);
    }
    else if constexpr (Count == 15) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
              a15] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15);
    }
    else if constexpr (Count == 16) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16);
    }
    else if constexpr (Count == 17) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17);
    }
    else if constexpr (Count == 18) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18);
    }
    else if constexpr (Count == 19) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19);
    }
    else if constexpr (Count == 20) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20);
    }
    else if constexpr (Count == 21) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21);
    }
    else if constexpr (Count == 22) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22);
    }
    else if constexpr (Count == 23) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23);
    }
    else if constexpr (Count == 24) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24);
    }
    else if constexpr (Count == 25) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
                     a25);
    }
    else if constexpr (Count == 26) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26);
    }
    else if constexpr (Count == 27) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27);
    }
    else if constexpr (Count == 28) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28);
    }
    else if constexpr (Count == 29) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29);
    }
    else if constexpr (Count == 30) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30);
    }
    else if constexpr (Count == 31) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31);
    }
    else if constexpr (Count == 32) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32);
    }
    else if constexpr (Count == 33) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33);
    }
    else if constexpr (Count == 34) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34);
    }
    else if constexpr (Count == 35) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35);
    }
    else if constexpr (Count == 36) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36);
    }
    else if constexpr (Count == 37) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36,
                     a37);
    }
    else if constexpr (Count == 38) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38);
    }
    else if constexpr (Count == 39) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39);
    }
    else if constexpr (Count == 40) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40);
    }
    else if constexpr (Count == 41) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41);
    }
    else if constexpr (Count == 42) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42);
    }
    else if constexpr (Count == 43) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43);
    }
    else if constexpr (Count == 44) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44);
    }
    else if constexpr (Count == 45) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45);
    }
    else if constexpr (Count == 46) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46);
    }
    else if constexpr (Count == 47) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47);
    }
    else if constexpr (Count == 48) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48);
    }
    else if constexpr (Count == 49) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48,
                     a49);
    }
    else if constexpr (Count == 50) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50);
    }
    else if constexpr (Count == 51) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51);
    }
    else if constexpr (Count == 52) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52);
    }
    else if constexpr (Count == 53) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53);
    }
    else if constexpr (Count == 54) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54);
    }
    else if constexpr (Count == 55) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55);
    }
    else if constexpr (Count == 56) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56);
    }
    else if constexpr (Count == 57) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57);
    }
    else if constexpr (Count == 58) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58);
    }
    else if constexpr (Count == 59) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59);
    }
    else if constexpr (Count == 60) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60);
    }
    else if constexpr (Count == 61) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60, a61] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60,
                     a61);
    }
    else if constexpr (Count == 62) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60, a61, a62] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61,
                     a62);
    }
    else if constexpr (Count == 63) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60, a61, a62, a63] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61,
                     a62, a63);
    }
    else if constexpr (Count == 64) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60, a61, a62, a63, a64] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61,
                     a62, a63, a64);
    }
  }

  template <template <size_t index, typename...> typename Func,
            typename... Args>
  constexpr decltype(auto) STRUCT_PACK_INLINE template_switch(std::size_t index,
                                                              auto &...args) {
    switch (index) {
      case 0:
        return Func<0, Args...>::run(args...);
      case 1:
        return Func<1, Args...>::run(args...);
      case 2:
        return Func<2, Args...>::run(args...);
      case 3:
        return Func<3, Args...>::run(args...);
      case 4:
        return Func<4, Args...>::run(args...);
      case 5:
        return Func<5, Args...>::run(args...);
      case 6:
        return Func<6, Args...>::run(args...);
      case 7:
        return Func<7, Args...>::run(args...);
      case 8:
        return Func<8, Args...>::run(args...);
      case 9:
        return Func<9, Args...>::run(args...);
      case 10:
        return Func<10, Args...>::run(args...);
      case 11:
        return Func<11, Args...>::run(args...);
      case 12:
        return Func<12, Args...>::run(args...);
      case 13:
        return Func<13, Args...>::run(args...);
      case 14:
        return Func<14, Args...>::run(args...);
      case 15:
        return Func<15, Args...>::run(args...);
      case 16:
        return Func<16, Args...>::run(args...);
      case 17:
        return Func<17, Args...>::run(args...);
      case 18:
        return Func<18, Args...>::run(args...);
      case 19:
        return Func<19, Args...>::run(args...);
      case 20:
        return Func<20, Args...>::run(args...);
      case 21:
        return Func<21, Args...>::run(args...);
      case 22:
        return Func<22, Args...>::run(args...);
      case 23:
        return Func<23, Args...>::run(args...);
      case 24:
        return Func<24, Args...>::run(args...);
      case 25:
        return Func<25, Args...>::run(args...);
      case 26:
        return Func<26, Args...>::run(args...);
      case 27:
        return Func<27, Args...>::run(args...);
      case 28:
        return Func<28, Args...>::run(args...);
      case 29:
        return Func<29, Args...>::run(args...);
      case 30:
        return Func<30, Args...>::run(args...);
      case 31:
        return Func<31, Args...>::run(args...);
      case 32:
        return Func<32, Args...>::run(args...);
      case 33:
        return Func<33, Args...>::run(args...);
      case 34:
        return Func<34, Args...>::run(args...);
      case 35:
        return Func<35, Args...>::run(args...);
      case 36:
        return Func<36, Args...>::run(args...);
      case 37:
        return Func<37, Args...>::run(args...);
      case 38:
        return Func<38, Args...>::run(args...);
      case 39:
        return Func<39, Args...>::run(args...);
      case 40:
        return Func<40, Args...>::run(args...);
      case 41:
        return Func<41, Args...>::run(args...);
      case 42:
        return Func<42, Args...>::run(args...);
      case 43:
        return Func<43, Args...>::run(args...);
      case 44:
        return Func<44, Args...>::run(args...);
      case 45:
        return Func<45, Args...>::run(args...);
      case 46:
        return Func<46, Args...>::run(args...);
      case 47:
        return Func<47, Args...>::run(args...);
      case 48:
        return Func<48, Args...>::run(args...);
      case 49:
        return Func<49, Args...>::run(args...);
      case 50:
        return Func<50, Args...>::run(args...);
      case 51:
        return Func<51, Args...>::run(args...);
      case 52:
        return Func<52, Args...>::run(args...);
      case 53:
        return Func<53, Args...>::run(args...);
      case 54:
        return Func<54, Args...>::run(args...);
      case 55:
        return Func<55, Args...>::run(args...);
      case 56:
        return Func<56, Args...>::run(args...);
      case 57:
        return Func<57, Args...>::run(args...);
      case 58:
        return Func<58, Args...>::run(args...);
      case 59:
        return Func<59, Args...>::run(args...);
      case 60:
        return Func<60, Args...>::run(args...);
      case 61:
        return Func<61, Args...>::run(args...);
      case 62:
        return Func<62, Args...>::run(args...);
      case 63:
        return Func<63, Args...>::run(args...);
      case 64:
        return Func<64, Args...>::run(args...);
      case 65:
        return Func<65, Args...>::run(args...);
      case 66:
        return Func<66, Args...>::run(args...);
      case 67:
        return Func<67, Args...>::run(args...);
      case 68:
        return Func<68, Args...>::run(args...);
      case 69:
        return Func<69, Args...>::run(args...);
      case 70:
        return Func<70, Args...>::run(args...);
      case 71:
        return Func<71, Args...>::run(args...);
      case 72:
        return Func<72, Args...>::run(args...);
      case 73:
        return Func<73, Args...>::run(args...);
      case 74:
        return Func<74, Args...>::run(args...);
      case 75:
        return Func<75, Args...>::run(args...);
      case 76:
        return Func<76, Args...>::run(args...);
      case 77:
        return Func<77, Args...>::run(args...);
      case 78:
        return Func<78, Args...>::run(args...);
      case 79:
        return Func<79, Args...>::run(args...);
      case 80:
        return Func<80, Args...>::run(args...);
      case 81:
        return Func<81, Args...>::run(args...);
      case 82:
        return Func<82, Args...>::run(args...);
      case 83:
        return Func<83, Args...>::run(args...);
      case 84:
        return Func<84, Args...>::run(args...);
      case 85:
        return Func<85, Args...>::run(args...);
      case 86:
        return Func<86, Args...>::run(args...);
      case 87:
        return Func<87, Args...>::run(args...);
      case 88:
        return Func<88, Args...>::run(args...);
      case 89:
        return Func<89, Args...>::run(args...);
      case 90:
        return Func<90, Args...>::run(args...);
      case 91:
        return Func<91, Args...>::run(args...);
      case 92:
        return Func<92, Args...>::run(args...);
      case 93:
        return Func<93, Args...>::run(args...);
      case 94:
        return Func<94, Args...>::run(args...);
      case 95:
        return Func<95, Args...>::run(args...);
      case 96:
        return Func<96, Args...>::run(args...);
      case 97:
        return Func<97, Args...>::run(args...);
      case 98:
        return Func<98, Args...>::run(args...);
      case 99:
        return Func<99, Args...>::run(args...);
      case 100:
        return Func<100, Args...>::run(args...);
      case 101:
        return Func<101, Args...>::run(args...);
      case 102:
        return Func<102, Args...>::run(args...);
      case 103:
        return Func<103, Args...>::run(args...);
      case 104:
        return Func<104, Args...>::run(args...);
      case 105:
        return Func<105, Args...>::run(args...);
      case 106:
        return Func<106, Args...>::run(args...);
      case 107:
        return Func<107, Args...>::run(args...);
      case 108:
        return Func<108, Args...>::run(args...);
      case 109:
        return Func<109, Args...>::run(args...);
      case 110:
        return Func<110, Args...>::run(args...);
      case 111:
        return Func<111, Args...>::run(args...);
      case 112:
        return Func<112, Args...>::run(args...);
      case 113:
        return Func<113, Args...>::run(args...);
      case 114:
        return Func<114, Args...>::run(args...);
      case 115:
        return Func<115, Args...>::run(args...);
      case 116:
        return Func<116, Args...>::run(args...);
      case 117:
        return Func<117, Args...>::run(args...);
      case 118:
        return Func<118, Args...>::run(args...);
      case 119:
        return Func<119, Args...>::run(args...);
      case 120:
        return Func<120, Args...>::run(args...);
      case 121:
        return Func<121, Args...>::run(args...);
      case 122:
        return Func<122, Args...>::run(args...);
      case 123:
        return Func<123, Args...>::run(args...);
      case 124:
        return Func<124, Args...>::run(args...);
      case 125:
        return Func<125, Args...>::run(args...);
      case 126:
        return Func<126, Args...>::run(args...);
      case 127:
        return Func<127, Args...>::run(args...);
      case 128:
        return Func<128, Args...>::run(args...);
      case 129:
        return Func<129, Args...>::run(args...);
      case 130:
        return Func<130, Args...>::run(args...);
      case 131:
        return Func<131, Args...>::run(args...);
      case 132:
        return Func<132, Args...>::run(args...);
      case 133:
        return Func<133, Args...>::run(args...);
      case 134:
        return Func<134, Args...>::run(args...);
      case 135:
        return Func<135, Args...>::run(args...);
      case 136:
        return Func<136, Args...>::run(args...);
      case 137:
        return Func<137, Args...>::run(args...);
      case 138:
        return Func<138, Args...>::run(args...);
      case 139:
        return Func<139, Args...>::run(args...);
      case 140:
        return Func<140, Args...>::run(args...);
      case 141:
        return Func<141, Args...>::run(args...);
      case 142:
        return Func<142, Args...>::run(args...);
      case 143:
        return Func<143, Args...>::run(args...);
      case 144:
        return Func<144, Args...>::run(args...);
      case 145:
        return Func<145, Args...>::run(args...);
      case 146:
        return Func<146, Args...>::run(args...);
      case 147:
        return Func<147, Args...>::run(args...);
      case 148:
        return Func<148, Args...>::run(args...);
      case 149:
        return Func<149, Args...>::run(args...);
      case 150:
        return Func<150, Args...>::run(args...);
      case 151:
        return Func<151, Args...>::run(args...);
      case 152:
        return Func<152, Args...>::run(args...);
      case 153:
        return Func<153, Args...>::run(args...);
      case 154:
        return Func<154, Args...>::run(args...);
      case 155:
        return Func<155, Args...>::run(args...);
      case 156:
        return Func<156, Args...>::run(args...);
      case 157:
        return Func<157, Args...>::run(args...);
      case 158:
        return Func<158, Args...>::run(args...);
      case 159:
        return Func<159, Args...>::run(args...);
      case 160:
        return Func<160, Args...>::run(args...);
      case 161:
        return Func<161, Args...>::run(args...);
      case 162:
        return Func<162, Args...>::run(args...);
      case 163:
        return Func<163, Args...>::run(args...);
      case 164:
        return Func<164, Args...>::run(args...);
      case 165:
        return Func<165, Args...>::run(args...);
      case 166:
        return Func<166, Args...>::run(args...);
      case 167:
        return Func<167, Args...>::run(args...);
      case 168:
        return Func<168, Args...>::run(args...);
      case 169:
        return Func<169, Args...>::run(args...);
      case 170:
        return Func<170, Args...>::run(args...);
      case 171:
        return Func<171, Args...>::run(args...);
      case 172:
        return Func<172, Args...>::run(args...);
      case 173:
        return Func<173, Args...>::run(args...);
      case 174:
        return Func<174, Args...>::run(args...);
      case 175:
        return Func<175, Args...>::run(args...);
      case 176:
        return Func<176, Args...>::run(args...);
      case 177:
        return Func<177, Args...>::run(args...);
      case 178:
        return Func<178, Args...>::run(args...);
      case 179:
        return Func<179, Args...>::run(args...);
      case 180:
        return Func<180, Args...>::run(args...);
      case 181:
        return Func<181, Args...>::run(args...);
      case 182:
        return Func<182, Args...>::run(args...);
      case 183:
        return Func<183, Args...>::run(args...);
      case 184:
        return Func<184, Args...>::run(args...);
      case 185:
        return Func<185, Args...>::run(args...);
      case 186:
        return Func<186, Args...>::run(args...);
      case 187:
        return Func<187, Args...>::run(args...);
      case 188:
        return Func<188, Args...>::run(args...);
      case 189:
        return Func<189, Args...>::run(args...);
      case 190:
        return Func<190, Args...>::run(args...);
      case 191:
        return Func<191, Args...>::run(args...);
      case 192:
        return Func<192, Args...>::run(args...);
      case 193:
        return Func<193, Args...>::run(args...);
      case 194:
        return Func<194, Args...>::run(args...);
      case 195:
        return Func<195, Args...>::run(args...);
      case 196:
        return Func<196, Args...>::run(args...);
      case 197:
        return Func<197, Args...>::run(args...);
      case 198:
        return Func<198, Args...>::run(args...);
      case 199:
        return Func<199, Args...>::run(args...);
      case 200:
        return Func<200, Args...>::run(args...);
      case 201:
        return Func<201, Args...>::run(args...);
      case 202:
        return Func<202, Args...>::run(args...);
      case 203:
        return Func<203, Args...>::run(args...);
      case 204:
        return Func<204, Args...>::run(args...);
      case 205:
        return Func<205, Args...>::run(args...);
      case 206:
        return Func<206, Args...>::run(args...);
      case 207:
        return Func<207, Args...>::run(args...);
      case 208:
        return Func<208, Args...>::run(args...);
      case 209:
        return Func<209, Args...>::run(args...);
      case 210:
        return Func<210, Args...>::run(args...);
      case 211:
        return Func<211, Args...>::run(args...);
      case 212:
        return Func<212, Args...>::run(args...);
      case 213:
        return Func<213, Args...>::run(args...);
      case 214:
        return Func<214, Args...>::run(args...);
      case 215:
        return Func<215, Args...>::run(args...);
      case 216:
        return Func<216, Args...>::run(args...);
      case 217:
        return Func<217, Args...>::run(args...);
      case 218:
        return Func<218, Args...>::run(args...);
      case 219:
        return Func<219, Args...>::run(args...);
      case 220:
        return Func<220, Args...>::run(args...);
      case 221:
        return Func<221, Args...>::run(args...);
      case 222:
        return Func<222, Args...>::run(args...);
      case 223:
        return Func<223, Args...>::run(args...);
      case 224:
        return Func<224, Args...>::run(args...);
      case 225:
        return Func<225, Args...>::run(args...);
      case 226:
        return Func<226, Args...>::run(args...);
      case 227:
        return Func<227, Args...>::run(args...);
      case 228:
        return Func<228, Args...>::run(args...);
      case 229:
        return Func<229, Args...>::run(args...);
      case 230:
        return Func<230, Args...>::run(args...);
      case 231:
        return Func<231, Args...>::run(args...);
      case 232:
        return Func<232, Args...>::run(args...);
      case 233:
        return Func<233, Args...>::run(args...);
      case 234:
        return Func<234, Args...>::run(args...);
      case 235:
        return Func<235, Args...>::run(args...);
      case 236:
        return Func<236, Args...>::run(args...);
      case 237:
        return Func<237, Args...>::run(args...);
      case 238:
        return Func<238, Args...>::run(args...);
      case 239:
        return Func<239, Args...>::run(args...);
      case 240:
        return Func<240, Args...>::run(args...);
      case 241:
        return Func<241, Args...>::run(args...);
      case 242:
        return Func<242, Args...>::run(args...);
      case 243:
        return Func<243, Args...>::run(args...);
      case 244:
        return Func<244, Args...>::run(args...);
      case 245:
        return Func<245, Args...>::run(args...);
      case 246:
        return Func<246, Args...>::run(args...);
      case 247:
        return Func<247, Args...>::run(args...);
      case 248:
        return Func<248, Args...>::run(args...);
      case 249:
        return Func<249, Args...>::run(args...);
      case 250:
        return Func<250, Args...>::run(args...);
      case 251:
        return Func<251, Args...>::run(args...);
      case 252:
        return Func<252, Args...>::run(args...);
      case 253:
        return Func<253, Args...>::run(args...);
      case 254:
        return Func<254, Args...>::run(args...);
      case 255:
        return Func<255, Args...>::run(args...);
      default:
        return Func<256, Args...>::run(args...);
        // index shouldn't bigger than 256
    }
  }  // namespace detail
}  // namespace detail
}  // namespace struct_pack