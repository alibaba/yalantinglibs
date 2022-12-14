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
#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "error_code.h"
#include "md5_constexpr.hpp"
#include "struct_pack/struct_pack/varint.hpp"
#include "tuple.hpp"

static_assert(std::endian::native == std::endian::little,
              "only support little endian now");

#include "reflection.h"

namespace struct_pack {

/*!
 * \ingroup struct_pack
 * \brief similar to std::optional<T>.
 * However, its semantics are forward compatible field.
 *
 * For example,
 *
 * ```cpp
 * struct person_v1 {
 *   int age;
 *   std::string name;
 * };
 *
 * struct person_v2 {
 *   int age;
 *   std::string name;
 *   struct_pack::compatible<bool> id;
 * };
 * ```
 *
 * Note that extension fields can only be added at the end!
 * Otherwise, compile-time error reported.
 *
 * For example,
 *
 * ```cpp
 * struct person2 {
 *   int age;
 *   struct_pack::compatible<int32_t> id;
 *   std::string name;
 *   struct_pack::compatible<bool> maybe;
 * };
 * person2 p = {.age=1,.id=1,.name="Hello",.maybe=false};
 * auto data = struct_pack::serialize(p);
 * ```
 *
 * compile error
 *
 * ```
 * error: static_assert failed, "The position of compatible<T> in struct is not
 * allowed!"
 * ```
 *
 * The value of compatible can be empty.
 *
 * TODO: add doc
 *
 * @tparam T field type
 */
template <typename T>
struct compatible : public std::optional<T> {
  constexpr compatible() = default;
  constexpr compatible(const compatible &other) = default;
  constexpr compatible(compatible &&other) = default;
  constexpr compatible(std::optional<T> &&other)
      : std::optional<T>(std::move(other)){};
  constexpr compatible(const std::optional<T> &other)
      : std::optional<T>(other){};
  constexpr compatible &operator=(const compatible &other) = default;
  constexpr compatible &operator=(compatible &&other) = default;
  using base = std::optional<T>;
  using base::base;
};

using var_int32_t = detail::sint<int32_t>;
using var_int64_t = detail::sint<int64_t>;
using var_uint32_t = detail::varint<uint32_t>;
using var_uint64_t = detail::varint<uint64_t>;

enum class type_info_config { automatic, disable, enable };

struct serialize_config {
  type_info_config add_type_info = type_info_config::automatic;
};

template <typename T>
constexpr inline type_info_config enable_type_info =
    type_info_config::automatic;

template <typename... Args>
STRUCT_PACK_INLINE consteval decltype(auto) get_type_literal();

namespace detail {

[[noreturn]] inline void unreachable() {
  // Uses compiler specific extensions if possible.
  // Even if no extension is used, undefined behavior is still raised by
  // an empty function body and the noreturn attribute.
#ifdef __GNUC__  // GCC, Clang, ICC
  __builtin_unreachable();
#elif defined(_MSC_VER)  // msvc
  __assume(false);
#endif
}

template <typename T>
constexpr inline bool is_trivial_tuple = false;

template <typename... T>
constexpr inline bool is_trivial_tuple<tuplet::tuple<T...>> = true;

template <class U>
constexpr auto get_types(U &&t) {
  using T = std::remove_cvref_t<U>;
  if constexpr (std::is_fundamental_v<T> || std::is_enum_v<T> || varint_t<T> ||
                std::is_same_v<std::string, T> || container<T> || optional<T> ||
                unique_ptr<T> || variant<T> || expected<T> || array<T> ||
                c_array<T> || std::is_same_v<std::monostate, T>) {
    return std::tuple<T>{};
  }
  else if constexpr (tuple<T>) {
    return T{};
  }
  else if constexpr (is_trivial_tuple<T>) {
    return T{};
  }
  else if constexpr (pair<T>) {
    return std::tuple<typename T::first_type, typename T::second_type>{};
  }
  else if constexpr (std::is_aggregate_v<T>) {
    // clang-format off
    return visit_members(
        std::forward<U>(t), [&]<typename... Args>(Args &&
                                                  ...) CONSTEXPR_INLINE_LAMBDA {
          return std::tuple<std::remove_cvref_t<Args>...>{};
        });
    // clang-format on
  }
  else {
    static_assert(!sizeof(T), "the type is not supported!");
  }
}

template <typename T>
struct is_trivial_serializable {
 private:
  static constexpr bool solve() {
    if constexpr (std::is_enum_v<T> || std::is_fundamental_v<T>) {
      return true;
    }
    else if constexpr (array<T>) {
      return is_trivial_serializable<typename T::value_type>::value;
    }
    else if constexpr (c_array<T>) {
      return is_trivial_serializable<
          typename std::remove_all_extents<T>::type>::value;
    }
    else if constexpr (!pair<T> && tuple<T> && !is_trivial_tuple<T>) {
      return false;
    }
    else if constexpr (container<T> || optional<T> || variant<T> ||
                       unique_ptr<T> || expected<T> || container_adapter<T> ||
                       varint_t<T>) {
      return false;
    }
    else if constexpr (pair<T>) {
      return is_trivial_serializable<typename T::first_type>::value &&
             is_trivial_serializable<typename T::second_type>::value;
    }
    else if constexpr (is_trivial_tuple<T>) {
      return []<std::size_t... I>(std::index_sequence<I...>)
          CONSTEXPR_INLINE_LAMBDA {
        return (is_trivial_serializable<std::tuple_element_t<I, T>>::value &&
                ...);
      }
      (std::make_index_sequence<std::tuple_size_v<T>>{});
    }
    else if constexpr (std::is_class_v<T>) {
      using T_ = decltype(get_types(T{}));
      return []<std::size_t... I>(std::index_sequence<I...>)
          CONSTEXPR_INLINE_LAMBDA {
        return (is_trivial_serializable<std::tuple_element_t<I, T_>>::value &&
                ...);
      }
      (std::make_index_sequence<std::tuple_size_v<T_>>{});
    }
    else
      return false;
  }

 public:
  static inline constexpr bool value = is_trivial_serializable::solve();
};

template <typename Type>
concept trivially_copyable_container = continuous_container<Type> &&
    requires(Type container) {
  requires is_trivial_serializable<typename Type::value_type>::value;
};

// clang-format off
template <typename T>
concept struct_pack_byte = std::is_same_v<char, T>
                           || std::is_same_v<unsigned char, T>
                           || std::is_same_v<std::byte, T>;

template <typename T>
concept struct_pack_buffer = trivially_copyable_container<T>
                             && struct_pack_byte<typename T::value_type>;
// clang-format on

enum class type_id {
  // compatible template type
  compatible_t = 0,
  // fundamental integral type
  int32_t = 1,
  uint32_t,
  int64_t,
  uint64_t,
  int8_t,
  uint8_t,
  int16_t,
  uint16_t,
  int128_t,  // TODO: support int128/uint128
  uint128_t,
  bool_t,
  char_8_t,
  char_16_t,
  char_32_t,
  w_char_t,  // Note: this type is not portable!Enable it with marco
             // STRUCT_PACK_ENABLE_UNPORTABLE_TYPE
  // fundamental float type
  float16_t,  // TODO: wait for C++23 standard float type
  float32_t,
  float64_t,
  float128_t,
  v_int32_t,   // variable size int
  v_int64_t,   // variable size int
  v_uint32_t,  // variable size unsigned int
  v_uint64_t,  // variable size unsigned int
  // template type
  string_t = 128,
  array_t,
  map_container_t,
  set_container_t,
  container_t,
  optional_t,
  variant_t,
  expected_t,
  // monostate, or void
  monostate_t = 250,
  // circle_flag
  circle_flag = 251,
  trivial_class_t = 253,
  // struct type
  non_trivial_class_t = 254,
  // end helper
  type_end_flag = 255,
};

template <typename T>
consteval type_id get_varint_type() {
  if constexpr (std::is_same_v<var_int32_t, T>) {
    return type_id::v_int32_t;
  }
  else if constexpr (std::is_same_v<var_int64_t, T>) {
    return type_id::v_int64_t;
  }
  else if constexpr (std::is_same_v<var_uint32_t, T>) {
    return type_id::v_uint32_t;
  }
  else if constexpr (std::is_same_v<var_uint64_t, T>) {
    return type_id::v_uint64_t;
  }
  else {
    static_assert(!std::is_same_v<wchar_t, T>, "unsupported varint type!");
  }
}

template <typename T>
consteval type_id get_integral_type() {
  if constexpr (std::is_same_v<int32_t, T>) {
    return type_id::int32_t;
  }
  else if constexpr (std::is_same_v<uint32_t, T>) {
    return type_id::uint32_t;
  }
  else if constexpr (std::is_same_v<int64_t, T> ||
                     (sizeof(long long) == 8 && std::is_same_v<T, long long>)) {
    return type_id::int64_t;
  }
  else if constexpr (std::is_same_v<uint64_t, T> ||
                     (sizeof(unsigned long long) == 8 &&
                      std::is_same_v<T, unsigned long long>)) {
    return type_id::uint64_t;
  }
  else if constexpr (std::is_same_v<int8_t, T> ||
                     std::is_same_v<signed char, T>) {
    return type_id::int8_t;
  }
  else if constexpr (std::is_same_v<uint8_t, T> ||
                     std::is_same_v<unsigned char, T>) {
    return type_id::uint8_t;
  }
  else if constexpr (std::is_same_v<int16_t, T>) {
    return type_id::int16_t;
  }
  else if constexpr (std::is_same_v<uint16_t, T>) {
    return type_id::uint16_t;
  }
  // In struct_pack, the char will be saved as unsigned!
  else if constexpr (std::is_same_v<char, T> || std::is_same_v<char8_t, T>) {
    return type_id::char_8_t;
  }
#ifdef STRUCT_PACK_ENABLE_UNPORTABLE_TYPE
  else if constexpr (std::is_same_v<wchar_t, T>) {
    return type_id::w_char_t;
  }
#endif
  // char16_t's size maybe bigger than 16 bits, which is not supported.
  else if constexpr (std::is_same_v<char16_t, T>) {
    static_assert(sizeof(char16_t) == 2,
                  "sizeof(char16_t)!=2, which is not supported.");
    return type_id::char_16_t;
  }
  // char32_t's size maybe bigger than 32bits, which is not supported.
  else if constexpr (std::is_same_v<char32_t, T> && sizeof(char32_t) == 4) {
    static_assert(sizeof(char32_t) == 4,
                  "sizeof(char16_t)!=4, which is not supported.");
    return type_id::char_32_t;
  }
  else if constexpr (std::is_same_v<bool, T> && sizeof(bool)) {
    static_assert(sizeof(bool) == 1,
                  "sizeof(bool)!=1, which is not supported.");
    return type_id::bool_t;
  }
  else {
    /*
     * Due to different data model,
     * the following types are not allowed on macOS
     * but work on Linux
     * For example,
     * on macOS, `typedef unsigned long long uint64_t;`
     * on Linux, `typedef unsigned long int  uint64_t;`
     *
     * - long
     * - long int
     * - signed long
     * - signed long int
     * - unsigned long
     * - unsigned long int
     *
     * We add this static_assert to give more information about not supported
     * type.
     */
    static_assert(
        !std::is_same_v<wchar_t, T>,
        "Tips: Add macro STRUCT_PACK_ENABLE_UNPORTABLE_TYPE to support "
        "wchar_t");
    static_assert(!std::is_same_v<long, T> && !std::is_same_v<unsigned long, T>,
                  "The long types have different width in "
                  "different data model. "
                  "see "
                  "https://en.cppreference.com/w/cpp/language/"
                  "types. "
                  "Please use fixed width integer types. e.g. "
                  "int32_t, int64_t. "
                  "see "
                  "https://en.cppreference.com/w/cpp/types/"
                  "integer.");
    static_assert(!sizeof(T), "not supported type");
    // This branch will always compiled error.
  }
}

template <typename T>
consteval type_id get_floating_point_type() {
  if constexpr (std::is_same_v<float, T>) {
    if constexpr (!std::numeric_limits<float>::is_iec559 ||
                  sizeof(float) != 4) {
      static_assert(
          !sizeof(T),
          "The float type in this machine is not standard IEEE 754 32bits "
          "float point!");
    }
    return type_id::float32_t;
  }
  else if constexpr (std::is_same_v<double, T>) {
    if constexpr (!std::numeric_limits<double>::is_iec559 ||
                  sizeof(double) != 8) {
      static_assert(
          !sizeof(T),
          "The double type in this machine is not standard IEEE 754 64bits "
          "float point!");
    }
    return type_id::float64_t;
  }
  else if constexpr (std::is_same_v<long double, T>) {
    if constexpr (sizeof(long double) != 16 ||
                  std::numeric_limits<long double>::is_iec559) {
      static_assert(!sizeof(T),
                    "The long double type in this machine is not standard IEEE "
                    "754 128bits "
                    "float point!");
    }
    return type_id::float128_t;
  }
  else {
    static_assert(!sizeof(T), "not supported type");
  }
}

template <typename T>
consteval type_id get_type_id() {
  static_assert(CHAR_BIT == 8);
  // compatible member, which should be ignored in MD5 calculated.
  if constexpr (optional<T> && requires {
                  requires std::is_same_v<
                      struct_pack::compatible<typename T::value_type>, T>;
                }) {
    return type_id::compatible_t;
  }
  else if constexpr (std::is_enum_v<T>) {
    return get_integral_type<std::underlying_type_t<T>>();
  }
  else if constexpr (std::is_integral_v<T>) {
    return get_integral_type<T>();
  }
  else if constexpr (std::is_floating_point_v<T>) {
    return get_floating_point_type<T>();
  }
  else if constexpr (detail::varint_t<T>) {
    return get_varint_type<T>();
  }
  else if constexpr (std::is_same_v<T, std::monostate> ||
                     std::is_same_v<T, void>) {
    return type_id::monostate_t;
  }
  else if constexpr (string<T>) {
    return type_id::string_t;
  }
  else if constexpr (array<T> || c_array<T>) {
    return type_id::array_t;
  }
  else if constexpr (map_container<T>) {
    return type_id::map_container_t;
  }
  else if constexpr (set_container<T>) {
    return type_id::set_container_t;
  }
  else if constexpr (container<T>) {
    return type_id::container_t;
  }
  else if constexpr (optional<T> || unique_ptr<T>) {
    return type_id::optional_t;
  }
  else if constexpr (variant<T>) {
    static_assert(
        std::variant_size_v<T> > 1 ||
            (std::variant_size_v<T> == 1 &&
             !std::is_same_v<std::variant_alternative_t<0, T>, std::monostate>),
        "The variant should contain's at least one type!");
    static_assert(std::variant_size_v<T> < 256, "The variant is too complex!");
    return type_id::variant_t;
  }
  else if constexpr (expected<T>) {
    return type_id::expected_t;
  }
  else if constexpr (is_trivial_tuple<T> || pair<T>) {
    return type_id::trivial_class_t;
  }
  else if constexpr (tuple<T>) {
    return type_id::non_trivial_class_t;
  }
  else if constexpr (std::is_class_v<T>) {
    static_assert(std::is_aggregate_v<std::remove_cvref_t<T>>);
    return type_id::trivial_class_t;
  }
  else {
    static_assert(!sizeof(T), "not supported type");
  }
}

template <size_t size>
consteval decltype(auto) get_size_literal() {
  static_assert(sizeof(size_t) <= 8);
  if constexpr (size < 1ull * 127) {
    return string_literal<char, 1>{{static_cast<char>(size + 129)}};
  }
  else if constexpr (size < 1ull * 127 * 127) {
    return string_literal<char, 2>{{static_cast<char>(size % 127 + 1),
                                    static_cast<char>(size / 127 + 129)}};
  }
  else if constexpr (size < 1ull * 127 * 127 * 127) {
    return string_literal<char, 3>{
        {static_cast<char>(size % 127 + 1),
         static_cast<char>(size / 127 % 127 + 1),
         static_cast<char>(size / (127 * 127) + 129)}};
  }
  else if constexpr (size < 1ull * 127 * 127 * 127 * 127) {
    return string_literal<char, 4>{
        {static_cast<char>(size % 127 + 1),
         static_cast<char>(size / 127 % 127 + 1),
         static_cast<char>(size / (127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127) + 129)}};
  }
  else if constexpr (size < 1ull * 127 * 127 * 127 * 127 * 127) {
    return string_literal<char, 5>{
        {static_cast<char>(size % 127 + 1),
         static_cast<char>(size / 127 % 127 + 1),
         static_cast<char>(size / (127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127 * 127) + 129)}};
  }
  else if constexpr (size < 1ull * 127 * 127 * 127 * 127 * 127 * 127) {
    return string_literal<char, 6>{
        {static_cast<char>(size % 127 + 1),
         static_cast<char>(size / 127 % 127 + 1),
         static_cast<char>(size / (127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127 * 127 * 127) + 129)}};
  }
  else if constexpr (size < 1ull * 127 * 127 * 127 * 127 * 127 * 127 * 127) {
    return string_literal<char, 7>{
        {static_cast<char>(size % 127 + 1),
         static_cast<char>(size / 127 % 127 + 1),
         static_cast<char>(size / (127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127 * 127 * 127) % 127 + 1),
         static_cast<char>(size / (127 * 127 * 127 * 127 * 127 * 127) + 129)}};
  }
  else if constexpr (size <
                     1ull * 127 * 127 * 127 * 127 * 127 * 127 * 127 * 127) {
    return string_literal<char, 8>{{
        static_cast<char>(size % 127 + 1),
        static_cast<char>(size / 127 % 127 + 1),
        static_cast<char>(size / (127 * 127) % 127 + 1),
        static_cast<char>(size / (127 * 127 * 127) % 127 + 1),
        static_cast<char>(size / (127 * 127 * 127 * 127) % 127 + 1),
        static_cast<char>(size / (127 * 127 * 127 * 127 * 127) % 127 + 1),
        static_cast<char>(size / (127 * 127 * 127 * 127 * 127 * 127) % 127 + 1),
        static_cast<char>(size / (127 * 127 * 127 * 127 * 127 * 127 * 127) +
                          129),
    }};
  }
  else {
    static_assert(
        size >= 1ull * 127 * 127 * 127 * 127 * 127 * 127 * 127 * 127 * 127,
        "The size is too large.");
  }
}
template <typename arg, typename... ParentArgs>
consteval std::size_t check_cycle() {
  using types_tuple = std::tuple<ParentArgs...>;
  if constexpr (sizeof...(ParentArgs)) {
    return []<std::size_t... I>(std::index_sequence<I...>) {
      std::size_t ret = std::max(
          {(std::is_same_v<std::tuple_element_t<I, types_tuple>, arg> ? I + 1
                                                                      : 0)...});
      return ret;
    }
    (std::make_index_sequence<sizeof...(ParentArgs)>());
  }
  else {
    return 0;
  }
}

// This help function is just to improve unit test coverage. :)
// The original lambada in `get_type_literal` is a compile-time expression.
// Currently, the unit test coverage tools like
// [Coverage](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html)
// can not detect code that is run at compile time.
template <typename Args, typename... ParentArgs, std::size_t... I>
consteval decltype(auto) get_type_literal(std::index_sequence<I...>);

template <typename Args, typename... ParentArgs, std::size_t... I>
consteval decltype(auto) get_variant_literal(std::index_sequence<I...>);

template <typename Arg, typename... ParentArgs>
consteval decltype(auto) get_type_literal() {
  constexpr auto id = get_type_id<Arg>();
  constexpr auto ret = string_literal<char, 1>{{static_cast<char>(id)}};
  if constexpr (id == type_id::non_trivial_class_t ||
                id == type_id::trivial_class_t) {
    using Args = decltype(get_types(Arg{}));
    constexpr auto body = get_type_literal<Args, Arg, ParentArgs...>(
        std::make_index_sequence<std::tuple_size_v<Args>>());
    if constexpr (is_trivial_serializable<Arg>::value) {
      static_assert(
          min_align<Arg>() == '0' || min_align<Arg>() <= max_align<Arg>(),
          "#pragma pack may decrease the alignment of a class, however, "
          "it cannot make a class over aligned.");
      constexpr auto end =
          string_literal<char, 3>{{static_cast<char>(min_align<Arg>()),
                                   static_cast<char>(max_align<Arg>()),
                                   static_cast<char>(type_id::type_end_flag)}};
      return ret + body + end;
    }
    else {
      constexpr auto end =
          string_literal<char, 1>{{static_cast<char>(type_id::type_end_flag)}};
      return ret + body + end;
    }
  }
  else if constexpr (id == type_id::variant_t) {
    constexpr auto sz = std::variant_size_v<Arg>;
    static_assert(sz > 0, "empty param of std::variant is not allowed!");
    static_assert(sz < 256, "too many alternative type in variant!");
    constexpr auto body = get_variant_literal<Arg>(
        std::make_index_sequence<std::variant_size_v<Arg>>());
    constexpr auto end =
        string_literal<char, 1>{{static_cast<char>(type_id::type_end_flag)}};
    return ret + body + end;
  }
  else if constexpr (id == type_id::array_t) {
    constexpr auto sz =
        sizeof(Arg) /
        sizeof(decltype(std::declval<
                        Arg>()[0]));  // std::size(std::declval<Arg>());
    return ret +
           get_type_literal<
               std::remove_cvref_t<decltype(std::declval<Arg>()[0])>, Arg,
               ParentArgs...>() +
           get_size_literal<sz>();
  }
  else if constexpr (unique_ptr<Arg>) {
    // detect cycle in graph
    constexpr std::size_t has_cycle =
        check_cycle<typename Arg::element_type, ParentArgs...>();
    if constexpr (has_cycle) {
      return ret +
             string_literal<char, 1>{
                 {static_cast<char>(type_id::circle_flag)}} +
             get_size_literal<has_cycle - 1>();
    }
    else {
      return ret +
             get_type_literal<std::remove_cvref_t<typename Arg::element_type>,
                              Arg, ParentArgs...>();
    }
  }
  else if constexpr (id == type_id::container_t || id == type_id::optional_t ||
                     id == type_id::string_t) {
    return ret + get_type_literal<std::remove_cvref_t<typename Arg::value_type>,
                                  Arg, ParentArgs...>();
  }
  else if constexpr (id == type_id::set_container_t) {
    return ret + get_type_literal<std::remove_cvref_t<typename Arg::key_type>,
                                  Arg, ParentArgs...>();
  }
  else if constexpr (id == type_id::map_container_t) {
    return ret +
           get_type_literal<std::remove_cvref_t<typename Arg::key_type>, Arg,
                            ParentArgs...>() +
           get_type_literal<std::remove_cvref_t<typename Arg::mapped_type>, Arg,
                            ParentArgs...>();
  }
  else if constexpr (id == type_id::expected_t) {
    return ret +
           get_type_literal<std::remove_cvref_t<typename Arg::value_type>, Arg,
                            ParentArgs...>() +
           get_type_literal<std::remove_cvref_t<typename Arg::error_type>, Arg,
                            ParentArgs...>();
  }
  else if constexpr (id != type_id::compatible_t) {
    return ret;
  }
  else {
    return string_literal<char, 0>{};
  }
}

template <typename Args, typename... ParentArgs, std::size_t... I>
consteval decltype(auto) get_type_literal(std::index_sequence<I...>) {
  return ((get_type_literal<std::remove_cvref_t<std::tuple_element_t<I, Args>>,
                            ParentArgs...>()) +
          ...);
}

template <typename Args, typename... ParentArgs, std::size_t... I>
consteval decltype(auto) get_variant_literal(std::index_sequence<I...>) {
  return ((get_type_literal<
              std::remove_cvref_t<std::variant_alternative_t<I, Args>>, Args,
              ParentArgs...>()) +
          ...);
}

template <typename Parent, typename... Args>
consteval decltype(auto) get_types_literal_impl() {
  if constexpr (std::is_same_v<Parent, void>)
    return (get_type_literal<Args>() + ...);
  else
    return (get_type_literal<Args, Parent>() + ...);
}

template <typename T, typename... Args>
consteval decltype(auto) get_types_literal() {
  constexpr auto root_id = get_type_id<std::remove_cvref_t<T>>();
  if constexpr (root_id == type_id::non_trivial_class_t ||
                root_id == type_id::trivial_class_t) {
    constexpr auto begin =
        string_literal<char, 1>{{static_cast<char>(root_id)}};
    constexpr auto body = get_types_literal_impl<T, Args...>();
    if constexpr (is_trivial_serializable<T>::value) {
      static_assert(min_align<T>() == '0' || min_align<T>() <= max_align<T>(),
                    "#pragma pack may decrease the alignment of a class, "
                    "however, it cannot make a class over aligned.");
      constexpr auto end = string_literal<char, 3>{
          {static_cast<char>(min_align<T>()), static_cast<char>(max_align<T>()),
           static_cast<char>(type_id::type_end_flag)}};
      return begin + body + end;
    }
    else {
      constexpr auto end =
          string_literal<char, 1>{{static_cast<char>(type_id::type_end_flag)}};
      return begin + body + end;
    }
  }
  else {
    return get_types_literal_impl<void, Args...>();
  }
}

template <typename T, typename Tuple, std::size_t... I>
consteval decltype(auto) get_types_literal(std::index_sequence<I...>) {
  return get_types_literal<
      T, std::remove_cvref_t<std::tuple_element_t<I, Tuple>>...>();
}

// the compatible<T> should in the end of the struct, and it shouldn't in the
// nested struct.
template <size_t depth, typename... Args>
consteval int check_if_compatible_element_exist();

// This help function is just to improve unit test coverage. :)
// Same as `get_type_literal_help_coverage`
template <std::size_t depth, typename Args, std::size_t... I>
consteval decltype(auto) check_if_compatible_element_exist_help_coverage(
    std::index_sequence<I...>) {
  return check_if_compatible_element_exist<
      depth + 1, std::remove_cvref_t<std::tuple_element_t<I, Args>>...>();
}

template <size_t depth, typename Arg>
constexpr int check_if_compatible_element_exist_helper(bool &flag) {
  constexpr auto id = get_type_id<Arg>();
  if constexpr (id == type_id::compatible_t) {
    flag = false;
    if (depth)
      return -1;
    return 1;
  }
  else {
    if (!flag)
      return -1;
    if constexpr (id == type_id::non_trivial_class_t ||
                  id == type_id::trivial_class_t) {
      using subArgs = decltype(get_types(Arg{}));
      return check_if_compatible_element_exist_help_coverage<depth, subArgs>(
          std::make_index_sequence<std::tuple_size_v<subArgs>>());
    }
    else {
      return 0;
    }
  }
}

template <size_t depth, typename... Args>
consteval int check_if_compatible_element_exist() {
  bool flag = true;
  int ret = 0;
  (
      [&]() {
        auto tmp = check_if_compatible_element_exist_helper<depth, Args>(flag);
        if (tmp == -1) {
          ret = -1;
        }
        else if (tmp == 1 && ret != -1) {
          ret = 1;
        }
        return ret;
      }(),
      ...);
  return ret;
}

template <typename T, typename... Args>
consteval uint32_t get_types_code_impl() {
  constexpr auto str = get_types_literal<T, std::remove_cvref_t<Args>...>();
  return MD5::MD5Hash32Constexpr(str.data(), str.size());
}

template <typename T, size_t... I>
consteval int check_if_compatible_element_exist(std::index_sequence<I...>) {
  constexpr auto ret =
      check_if_compatible_element_exist<0, std::tuple_element_t<I, T>...>();
  // ret == 0 -> unexist_compatible_element
  // ret == 1 -> legal_compatible_element
  // ret == -1 -> illegal_compatible_element
  static_assert(
      ret == 0 || ret == 1,
      "The relative position of compatible<T> in struct is not allowed!");
  return ret;
}

template <typename T, typename Tuple, size_t... I>
consteval uint32_t get_types_code(std::index_sequence<I...>) {
  return get_types_code_impl<T, std::tuple_element_t<I, Tuple>...>();
}

struct size_info {
  std::size_t total;
  std::size_t size_cnt;
  std::size_t max_size;
  constexpr size_info &operator+=(const size_info &other) {
    this->total += other.total;
    this->size_cnt += other.size_cnt;
    this->max_size = std::max(this->max_size, other.max_size);
    return *this;
  }
  constexpr size_info operator+(const size_info &other) {
    return {this->total + other.total, this->size_cnt += other.size_cnt,
            std::max(this->max_size, other.max_size)};
  }
};

template <typename T, typename... Args>
constexpr size_info STRUCT_PACK_INLINE
calculate_payload_size(const T &item, const Args &...items);

template <typename T>
constexpr size_info STRUCT_PACK_INLINE calculate_one_size(const T &item) {
  constexpr auto id = get_type_id<std::remove_cvref_t<T>>();
  static_assert(id != detail::type_id::type_end_flag);
  using type = std::remove_cvref_t<decltype(item)>;
  static_assert(!std::is_pointer_v<type>);
  size_info ret{.total = 0, .size_cnt = 0, .max_size = 0};
  if constexpr (std::is_same_v<type, std::monostate>) {
  }
  else if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type>) {
    ret.total = sizeof(type);
  }
  else if constexpr (detail::varint_t<type>) {
    ret.total = detail::calculate_varint_size(item);
  }
  else if constexpr (c_array<type> || array<type>) {
    if constexpr (is_trivial_serializable<type>::value) {
      ret.total = sizeof(type);
    }
    else {
      for (auto &i : item) {
        ret += calculate_one_size(i);
      }
    }
  }
  else if constexpr (container<type>) {
    ret.size_cnt += 1;
    ret.max_size = std::max(ret.max_size, item.size());
    if constexpr (trivially_copyable_container<type>) {
      using value_type = typename type::value_type;
      ret.total = item.size() * sizeof(value_type);
    }
    else {
      for (auto &&i : item) {
        ret += calculate_one_size(i);
      }
    }
  }
  else if constexpr (container_adapter<type>) {
    static_assert(!sizeof(type), "the container adapter type is not supported");
  }
  else if constexpr (!pair<type> && tuple<type> && !is_trivial_tuple<type>) {
    std::apply(
        [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
          ret += calculate_payload_size(items...);
        },
        item);
  }
  else if constexpr (optional<type>) {
    ret.total = sizeof(char);
    if (item.has_value()) {
      ret += calculate_one_size(*item);
    }
  }
  else if constexpr (variant<type>) {
    ret.total = sizeof(uint8_t);
    ret += std::visit(
        [](const auto &e) {
          return calculate_one_size(e);
        },
        item);
  }
  else if constexpr (expected<type>) {
    ret.total = sizeof(bool);
    if (item.has_value()) {
      if constexpr (!std::is_same_v<typename type::value_type, void>)
        ret += calculate_one_size(item.value());
    }
    else {
      ret += calculate_one_size(item.error());
    }
  }
  else if constexpr (std::is_class_v<type>) {
    if constexpr (!pair<type> && !is_trivial_tuple<type>)
      static_assert(std::is_aggregate_v<std::remove_cvref_t<type>>);
    if constexpr (is_trivial_serializable<type>::value) {
      ret.total = sizeof(type);
    }
    else {
      visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
        ret += calculate_payload_size(items...);
      });
    }
  }
  else {
    static_assert(!sizeof(type), "the type is not supported yet");
  }
  return ret;
}

template <unique_ptr T>
constexpr size_info inline calculate_one_size(const T &item) {
  size_info ret{.total = sizeof(char), .size_cnt = 0, .max_size = 0};
  if (item != nullptr) {
    ret += calculate_one_size(*item);
  }
  return ret;
}

template <typename T, typename... Args>
constexpr size_info STRUCT_PACK_INLINE
calculate_payload_size(const T &item, const Args &...items) {
  if constexpr (sizeof...(items))
    return calculate_one_size(item) + calculate_payload_size(items...);
  else
    return calculate_one_size(item);
}

template <typename T, typename Tuple>
consteval uint32_t get_types_code() {
  return detail::get_types_code<T, Tuple>(
      std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename T>
consteval int check_if_compatible_element_exist() {
  return detail::check_if_compatible_element_exist<T>(
      std::make_index_sequence<std::tuple_size_v<T>>{});
}

template <typename T>
concept exist_compatible_member = check_if_compatible_element_exist<
    decltype(get_types(std::remove_cvref_t<T>{}))>()
== 1;

template <typename T>
concept unexist_compatible_member = check_if_compatible_element_exist<
    decltype(get_types(std::remove_cvref_t<T>{}))>()
== 0;

template <typename T>
struct serialize_static_config {
  static constexpr bool has_compatible = exist_compatible_member<T>;
#ifdef NDEBUG
  static constexpr bool has_type_literal = false;
#else
  static constexpr bool has_type_literal = true;
#endif
};

struct serialize_runtime_info {
  std::size_t len;
  unsigned char metainfo;
};

template <typename... Args>
using get_args_type =
    typename std::conditional<sizeof...(Args) == 1,
                              std::tuple_element_t<0, std::tuple<Args...>>,
                              std::tuple<Args...>>::type;
template <serialize_config conf, typename T>
constexpr bool check_if_add_type_literal() {
  if constexpr (conf.add_type_info == type_info_config::automatic) {
    if constexpr (enable_type_info<T> == type_info_config::automatic) {
      return serialize_static_config<T>::has_type_literal;
    }
    else {
      return enable_type_info<T> == type_info_config::enable;
    }
  }
  else {
    return conf.add_type_info == type_info_config::enable;
  }
}

template <serialize_config conf, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE constexpr serialize_runtime_info
get_serialize_runtime_info(const Args &...args) {
  using Type = get_args_type<Args...>;
  constexpr bool has_compatible = serialize_static_config<Type>::has_compatible;
  constexpr bool has_type_literal = check_if_add_type_literal<conf, Type>();
  serialize_runtime_info ret{.len = sizeof(uint32_t)};
  auto sz_info = calculate_payload_size(args...);

  if (sz_info.max_size < (1ull << 8)) [[likely]] {
    ret.len += sz_info.total + sz_info.size_cnt;
    ret.metainfo = 0b00000;
    constexpr bool has_compile_time_determined_meta_info =
        has_compatible || has_type_literal;
    if constexpr (has_compile_time_determined_meta_info) {
      ret.len += sizeof(unsigned char);
    }
  }
  else {
    if (sz_info.max_size < (1ull << 16)) {
      ret.len += sz_info.total + sz_info.size_cnt * 2;
      ret.metainfo = 0b01000;
    }
    else if (sz_info.max_size < (1ull << 32)) {
      ret.len += sz_info.total + sz_info.size_cnt * 4;
      ret.metainfo = 0b10000;
    }
    else {
      ret.len += sz_info.total + sz_info.size_cnt * 8;
      ret.metainfo = 0b11000;
    }
    // size_type >= 1 , has metainfo
    ret.len += sizeof(unsigned char);
  }
  if constexpr (has_type_literal) {
    constexpr auto type_literal = struct_pack::get_type_literal<Args...>();
    // struct_pack::get_type_literal<Args...>().size() crash in clang13. Bug?
    ret.len += type_literal.size() + 1;
    ret.metainfo |= 0b100;
  }
  if constexpr (has_compatible) {  // calculate bytes count of serialize
                                   // length
    if (ret.len + 2 < (1ull << 16)) [[likely]] {
      ret.len += 2;
      ret.metainfo |= 0b01;
    }
    else if (ret.len + 4 < (1ull << 32)) {
      ret.len += 4;
      ret.metainfo |= 0b10;
    }
    else {
      ret.len += 8;
      ret.metainfo |= 0b11;
    }
  }
  return ret;
}

template <struct_pack_byte Byte, typename serialize_type>
class packer {
 public:
  packer(Byte *data, const serialize_runtime_info &info)
      : data_(data), pos_(0), info(info) {}
  packer(const packer &) = delete;
  packer &operator=(const packer &) = delete;

  template <serialize_config conf, std::size_t size_type, typename T,
            typename... Args>
  STRUCT_PACK_INLINE void serialize(const T &t, const Args &...args) {
    serialize_metainfo<conf, size_type == 1, T, Args...>();
    serialize_many<size_type>(t, args...);
  }

  STRUCT_PACK_INLINE const Byte *data() { return data_; }

  STRUCT_PACK_INLINE size_t size() { return pos_; }

 private:
  template <typename T, typename... Args>
  static consteval uint32_t STRUCT_PACK_INLINE calculate_raw_hash() {
    if constexpr (sizeof...(Args) == 0) {
      return get_types_code<
          std::remove_cvref_t<T>,
          std::remove_cvref_t<decltype(get_types(std::declval<T>()))>>();
    }
    else {
      return get_types_code<
          std::tuple<std::remove_cvref_t<T>, std::remove_cvref_t<Args>...>,
          std::tuple<std::remove_cvref_t<T>, std::remove_cvref_t<Args>...>>();
    }
  }
  template <serialize_config conf, typename T, typename... Args>
  static consteval uint32_t STRUCT_PACK_INLINE calculate_hash_head() {
    constexpr uint32_t raw_types_code = calculate_raw_hash<T, Args...>();
    if constexpr (serialize_static_config<serialize_type>::has_compatible ||
                  check_if_add_type_literal<conf, serialize_type>()) {
      return raw_types_code - raw_types_code % 2 + 1;
    }
    else {  // default case, only has hash_code
      return raw_types_code - raw_types_code % 2;
    }
  }
  template <serialize_config conf, bool is_default_size_type, typename T,
            typename... Args>
  constexpr void STRUCT_PACK_INLINE serialize_metainfo() {
    constexpr auto hash_head = calculate_hash_head<conf, T, Args...>() |
                               (is_default_size_type ? 0 : 1);
    std::memcpy(data_ + pos_, &hash_head, sizeof(uint32_t));
    pos_ += sizeof(uint32_t);
    if constexpr (hash_head % 2) {  // has more metainfo
      std::memcpy(data_ + pos_, &info.metainfo, sizeof(char));
      pos_ += sizeof(char);
      if constexpr (serialize_static_config<serialize_type>::has_compatible) {
        constexpr std::size_t sz[] = {0, 2, 4, 8};
        auto len_size = sz[info.metainfo & 0b11];
        std::memcpy(data_ + pos_, &info.len, len_size);
        pos_ += len_size;
      }
      if constexpr (check_if_add_type_literal<conf, serialize_type>()) {
        constexpr auto type_literal =
            struct_pack::get_type_literal<T, Args...>();
        std::memcpy(data_ + pos_, type_literal.data(), type_literal.size() + 1);
        pos_ += type_literal.size() + 1;
      }
    }
  }

 private:
  template <std::size_t size_type>
  constexpr void STRUCT_PACK_INLINE serialize_many(const auto &first_item,
                                                   const auto &...items) {
    serialize_one<size_type>(first_item);
    if constexpr (sizeof...(items) > 0) {
      serialize_many<size_type>(items...);
    }
  }

  template <std::size_t size_type>
  constexpr void STRUCT_PACK_INLINE serialize_one(const auto &item) {
    using type = std::remove_cvref_t<decltype(item)>;
    static_assert(!std::is_pointer_v<type>);
    if constexpr (std::is_same_v<type, std::monostate>) {
    }
    else if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type>) {
      std::memcpy(data_ + pos_, &item, sizeof(type));
      pos_ += sizeof(type);
    }
    else if constexpr (detail::varint_t<type>) {
      detail::serialize_varint(data_, pos_, item);
    }
    else if constexpr (c_array<type> || array<type>) {
      if constexpr (is_trivial_serializable<type>::value) {
        std::memcpy(data_ + pos_, &item, sizeof(type));
        pos_ += sizeof(type);
      }
      else {
        for (const auto &i : item) {
          serialize_one<size_type>(i);
        }
      }
    }
    else if constexpr (map_container<type> || container<type>) {
      uint64_t size = item.size();
#ifdef STRUCT_PACK_OPTIMIZE
      std::memcpy(data_ + pos_, &size, size_type);
      pos_ += size_type;
#else
      if constexpr (size_type == 1) {
        std::memcpy(data_ + pos_, &size, size_type);
        pos_ += size_type;
      }
      else {
        switch ((info.metainfo & 0b11000) >> 3) {
          case 1:
            std::memcpy(data_ + pos_, &size, 2);
            pos_ += 2;
            break;
          case 2:
            std::memcpy(data_ + pos_, &size, 4);
            pos_ += 4;
            break;
          case 3:
            std::memcpy(data_ + pos_, &size, 8);
            pos_ += 8;
            break;
          default:
            unreachable();
        }
      }
#endif
      if constexpr (trivially_copyable_container<type>) {
        using value_type = typename type::value_type;
        auto container_size = 1ull * size * sizeof(value_type);
        if (container_size >= PTRDIFF_MAX) [[unlikely]]
          unreachable();
        else
          std::memcpy(data_ + pos_, item.data(), container_size);
        pos_ += container_size;
        return;
      }
      else {
        for (const auto &i : item) {
          serialize_one<size_type>(i);
        }
      }
    }
    else if constexpr (container_adapter<type>) {
      static_assert(!sizeof(type),
                    "the container adapter type is not supported");
    }
    else if constexpr (!pair<type> && tuple<type> && !is_trivial_tuple<type>) {
      std::apply(
          [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
            serialize_many<size_type>(items...);
          },
          item);
    }
    else if constexpr (optional<type>) {
      bool has_value = item.has_value();
      std::memcpy(data_ + pos_, &has_value, sizeof(char));
      pos_ += sizeof(char);
      if (has_value) {
        serialize_one<size_type>(*item);
      }
    }
    else if constexpr (variant<type>) {
      static_assert(std::variant_size_v<type> < 256,
                    "variant's size is too large");
      uint8_t index = item.index();
      std::memcpy(data_ + pos_, &index, sizeof(index));
      pos_ += sizeof(index);
      std::visit(
          [this](auto &&e) {
            this->serialize_one<size_type>(e);
          },
          item);
    }
    else if constexpr (expected<type>) {
      bool has_value = item.has_value();
      std::memcpy(data_ + pos_, &has_value, sizeof(has_value));
      pos_ += sizeof(has_value);
      if (has_value) {
        if constexpr (!std::is_same_v<typename type::value_type, void>)
          serialize_one<size_type>(item.value());
      }
      else {
        serialize_one<size_type>(item.error());
      }
    }
    else if constexpr (std::is_class_v<type>) {
      if constexpr (!pair<type> && !is_trivial_tuple<type>)
        static_assert(std::is_aggregate_v<std::remove_cvref_t<type>>);
      if constexpr (is_trivial_serializable<type>::value) {
        std::memcpy(data_ + pos_, &item, sizeof(type));
        pos_ += sizeof(type);
      }
      else {
        visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
          serialize_many<size_type>(items...);
        });
      }
    }
    else {
      static_assert(!sizeof(type), "the type is not supported yet");
    }
    return;
  }

  template <std::size_t size_type, unique_ptr T>
  constexpr void inline serialize_one(const T &item) {
    bool has_value = (item != nullptr);
    std::memcpy(data_ + pos_, &has_value, sizeof(char));
    pos_ += sizeof(char);
    if (has_value) {
      serialize_one<size_type>(*item);
    }
  }

  template <typename T>
  friend constexpr size_t get_needed_size(const T &t);
  Byte *data_;
  std::size_t pos_;
  const serialize_runtime_info &info;
};

template <struct_pack_byte Byte>
class unpacker {
 public:
  unpacker() = delete;
  unpacker(const unpacker &) = delete;
  unpacker &operator=(const unpacker &) = delete;

  STRUCT_PACK_INLINE unpacker(const Byte *data, std::size_t size)
      : data_{data}, size_(size) {}

  template <typename T, typename... Args>
  STRUCT_PACK_INLINE struct_pack::errc deserialize(T &t, Args &...args) {
    auto &&[err_code, data_len] =
        deserialize_metainfo<get_args_type<T, Args...>>();
    if (err_code != struct_pack::errc{}) [[unlikely]] {
      return err_code;
    }
    switch (size_type_) {
      case 0:
        return deserialize_many<1>(t, args...);
#ifdef STRUCT_PACK_OPTIMIZE
      case 1:
        return deserialize_many<2>(t, args...);
      case 2:
        return deserialize_many<4>(t, args...);
      case 3:
        return deserialize_many<8>(t, args...);
#else
      case 1:
      case 2:
      case 3:
        return deserialize_many<2>(t, args...);
#endif
      default:
        unreachable();
    }
  }

  template <typename T, typename... Args>
  STRUCT_PACK_INLINE struct_pack::errc deserialize(std::size_t &len, T &t,
                                                   Args &...args) {
    auto &&[err_code, data_len] =
        deserialize_metainfo<get_args_type<T, Args...>>();
    if (err_code != struct_pack::errc{}) [[unlikely]] {
      return err_code;
    }
    struct_pack::errc ret;
    switch (size_type_) {
      case 0:
        ret = deserialize_many<1>(t, args...);
        break;
#ifdef STRUCT_PACK_OPTIMIZE
      case 1:
        ret = deserialize_many<2>(t, args...);
        break;
      case 2:
        ret = deserialize_many<4>(t, args...);
        break;
      case 3:
        ret = deserialize_many<8>(t, args...);
        break;
#else
      case 1:
      case 2:
      case 3:
        ret = deserialize_many<2>(t, args...);
        break;
#endif
      default:
        unreachable();
    }
    len = (ret == struct_pack::errc{} ? std::max(pos_, data_len) : 0);
    return ret;
  }

  template <typename U, size_t I>
  STRUCT_PACK_INLINE struct_pack::errc get_field(
      std::tuple_element_t<
          I, decltype(get_types(std::declval<std::remove_cvref_t<U>>()))>
          &field) {
    using T = std::remove_cvref_t<U>;

    if (auto [code, _] = deserialize_metainfo<T>(); code != struct_pack::errc{})
        [[unlikely]] {
      return code;
    }
    switch (size_type_) {
      case 0:
        return get_field_impl<1, U, I>(field);
#ifdef STRUCT_PACK_OPTIMIZE
      case 1:
        return get_field_impl<2, U, I>(field);
      case 2:
        return get_field_impl<4, U, I>(field);
      case 3:
        return get_field_impl<8, U, I>(field);
#else
      case 1:
      case 2:
      case 3:
        return get_field_impl<2, U, I>(field);
#endif
      default:
        unreachable();
    }
  }

  template <std::size_t size_type, typename U, size_t I>
  STRUCT_PACK_INLINE struct_pack::errc get_field_impl(
      std::tuple_element_t<
          I, decltype(get_types(std::declval<std::remove_cvref_t<U>>()))>
          &field) {
    using T = std::remove_cvref_t<U>;

    static T t;
    struct_pack::errc err_code;
    if constexpr (tuple<T>) {
      err_code = std::apply(
          [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
            static_assert(I < sizeof...(items), "out of range");
            return for_each<size_type, I>(field, items...);
          },
          t);
    }
    else if constexpr (std::is_class_v<T>) {
      static_assert(std::is_aggregate_v<T>);
      err_code = visit_members(t, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
        static_assert(I < sizeof...(items), "out of range");
        return for_each<size_type, I>(field, items...);
      });
    }
    else {
      static_assert(!sizeof(T), "illegal type");
    }
    pos_ = 0;
    return err_code;
  }

 private:
  template <size_t index, typename size_type, typename NotSkip>
  struct variant_construct_helper {
    template <typename unpack, typename variant_t>
    static STRUCT_PACK_INLINE constexpr void run(unpack &unpacker,
                                                 variant_t &v) {
      if constexpr (index >= std::variant_size_v<variant_t>) {
        return;
      }
      else {
        v = variant_t{std::in_place_index_t<index>{}};
        unpacker.template deserialize_one<size_type::value, NotSkip::value>(
            std::get<index>(v));
      }
    }
  };

  STRUCT_PACK_INLINE std::pair<struct_pack::errc, std::size_t>
  deserialize_compatible(unsigned compatible_sz_len) {
    constexpr std::size_t sz[] = {0, 2, 4, 8};
    auto len_sz = sz[compatible_sz_len];
    uint64_t data_len = 0;
    if (size_ < sizeof(uint32_t) + sizeof(unsigned char) + len_sz)
        [[unlikely]] {
      return {errc::no_buffer_space, 0};
    }
    std::memcpy(&data_len, data_ + pos_, len_sz);
    pos_ += len_sz;
    return {errc{}, data_len};
  }

  template <typename T>
  STRUCT_PACK_INLINE struct_pack::errc deserialize_type_literal() {
    constexpr auto literal = struct_pack::get_type_literal<T>();
    if (std::string_view{(char *)(data_ + pos_), literal.size() + 1} !=
        std::string_view{literal.data(), literal.size() + 1}) [[unlikely]] {
      return errc::hash_conflict;
    }
    pos_ += literal.size() + 1;
    return errc{};
  }

  template <class T>
  STRUCT_PACK_INLINE std::pair<struct_pack::errc, std::size_t>
  deserialize_metainfo() {
    if (size_ < sizeof(uint32_t)) [[unlikely]] {
      return {struct_pack::errc::no_buffer_space, 0};
    }
    constexpr uint32_t types_code =
        get_types_code<T, decltype(get_types(std::declval<T>()))>();
    uint32_t current_types_code;
    std::memcpy(&current_types_code, data_ + pos_, sizeof(uint32_t));
    if ((current_types_code / 2) != (types_code / 2)) [[unlikely]] {
      return {struct_pack::errc::invalid_argument, 0};
    }
    pos_ += sizeof(uint32_t);
    if (current_types_code % 2 == 0) [[likely]]  // unexist extended metainfo
    {
      size_type_ = 0;
      return {};
    }
    if (size_ < sizeof(unsigned char) + sizeof(uint32_t)) [[unlikely]] {
      return {struct_pack::errc::no_buffer_space, 0};
    }
    unsigned char metainfo;
    std::memcpy(&metainfo, data_ + pos_, sizeof(unsigned char));
    pos_ += sizeof(unsigned char);
    std::pair<errc, std::size_t> ret;
    auto compatible_sz_len = metainfo & 0b11;
    if (compatible_sz_len) {
      if (ret = deserialize_compatible(compatible_sz_len); ret.first != errc{})
          [[unlikely]] {
        return ret;
      }
    }
    auto has_type_literal = metainfo & 0b100;
    if (has_type_literal) {
      if (auto ec = deserialize_type_literal<T>(); ec != errc{}) [[unlikely]] {
        return {ec, 0};
      }
    }
    size_type_ = (metainfo & 0b11000) >> 3;
    return ret;
  }

  template <size_t size_type, bool NotSkip = true>
  constexpr struct_pack::errc STRUCT_PACK_INLINE deserialize_many() {
    return {};
  }

  template <size_t size_type, bool NotSkip = true>
  constexpr struct_pack::errc STRUCT_PACK_INLINE
  deserialize_many(auto &&first_item, auto &&...items) {
    auto code = deserialize_one<size_type, NotSkip>(first_item);
    if (code != struct_pack::errc{}) [[unlikely]] {
      return code;
    }
    return deserialize_many<size_type, NotSkip>(items...);
  }

  template <size_t size_type, bool NotSkip>
  constexpr struct_pack::errc STRUCT_PACK_INLINE deserialize_one(auto &item) {
    struct_pack::errc code{};
    using type = std::remove_cvref_t<decltype(item)>;
    static_assert(!std::is_pointer_v<type>);
    if constexpr (std::is_same_v<type, std::monostate>) {
    }
    else if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type>) {
      if (pos_ + sizeof(type) > size_) [[unlikely]] {
        return struct_pack::errc::no_buffer_space;
      }
      if constexpr (NotSkip) {
        std::memcpy(&item, data_ + pos_, sizeof(type));
      }
      pos_ += sizeof(type);
    }
    else if constexpr (detail::varint_t<type>) {
      code = detail::deserialize_varint(data_, pos_, size_, item);
    }
    else if constexpr (array<type> || c_array<type>) {
      if constexpr (is_trivial_serializable<type>::value) {
        if constexpr (NotSkip) {
          std::memcpy(&item, data_ + pos_, sizeof(type));
        }
        pos_ += sizeof(type);
      }
      else {
        for (auto &i : item) {
          code = deserialize_one<size_type, NotSkip>(i);
          if (code != struct_pack::errc{}) [[unlikely]] {
            return code;
          }
        }
      }
    }
    else if constexpr (container<type>) {
      size_t size = 0;
#ifdef STRUCT_PACK_OPTIMIZE
      if (pos_ + size_type > size_) [[unlikely]] {
        return struct_pack::errc::no_buffer_space;
      }
      std::memcpy(&size, data_ + pos_, size_type);
      pos_ += size_type;
#else
      if constexpr (size_type == 1) {
        if (pos_ + size_type > size_) [[unlikely]] {
          return struct_pack::errc::no_buffer_space;
        }
        std::memcpy(&size, data_ + pos_, size_type);
        pos_ += size_type;
      }
      else {
        switch (size_type_) {
          case 1:
            if (pos_ + 2 > size_) [[unlikely]] {
              return struct_pack::errc::no_buffer_space;
            }
            std::memcpy(&size, data_ + pos_, 2);
            pos_ += 2;
            break;
          case 2:
            if (pos_ + 4 > size_) [[unlikely]] {
              return struct_pack::errc::no_buffer_space;
            }
            std::memcpy(&size, data_ + pos_, 4);
            pos_ += 4;
            break;
          case 3:
            if (pos_ + 8 > size_) [[unlikely]] {
              return struct_pack::errc::no_buffer_space;
            }
            std::memcpy(&size, data_ + pos_, 8);
            pos_ += 8;
            break;
          default:
            unreachable();
        }
      }
#endif
      if (size == 0) [[unlikely]] {
        return {};
      }
      if constexpr (map_container<type> || set_container<type>) {
        // value is the element of map/set container.
        // if the type is set, then value is set::value_type;
        // if the type is map, then value is pair<key_type,mapped_type>
        decltype([]<typename T>(const T &) {
          if constexpr (map_container<T>) {
            return std::pair<typename T::key_type, typename T::mapped_type>{};
          }
          else {
            return typename T::value_type{};
          }
        }(item)) value;
        for (size_t i = 0; i < size; ++i) {
          code = deserialize_one<size_type, NotSkip>(value);
          if (code != struct_pack::errc{}) [[unlikely]] {
            return code;
          }
          if constexpr (NotSkip) {
            item.emplace(std::move(value));
          }
        }
      }
      else {
        using value_type = typename type::value_type;
        if constexpr (trivially_copyable_container<type>) {
          size_t mem_sz = size * sizeof(value_type);
          if constexpr (NotSkip) {
            if (pos_ + mem_sz > size_) [[unlikely]] {
              return struct_pack::errc::no_buffer_space;
            }
            if constexpr (string_view<type>) {
              item = {(value_type *)(data_ + pos_), size};
            }
            else {
              item.resize(size);
              if (mem_sz >= PTRDIFF_MAX) [[unlikely]]
                unreachable();
              else
                std::memcpy(item.data(), data_ + pos_, mem_sz);
            }
          }
          pos_ += mem_sz;
        }
        else {
          if constexpr (NotSkip) {
            item.resize(size);
            for (auto &i : item) {
              code = deserialize_one<size_type, NotSkip>(i);
              if (code != struct_pack::errc{}) [[unlikely]] {
                return code;
              }
            }
          }
          else {
            value_type useless;
            for (size_t i = 0; i < size; ++i) {
              code = deserialize_one<size_type, NotSkip>(useless);
              if (code != struct_pack::errc{}) [[unlikely]] {
                return code;
              }
            }
          }
        }
      }
    }
    else if constexpr (container_adapter<type>) {
      static_assert(!sizeof(type),
                    "the container adapter type is not supported");
    }
    else if constexpr (!pair<type> && tuple<type> && !is_trivial_tuple<type>) {
      std::apply(
          [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
            code = deserialize_many<size_type>(items...);
          },
          item);
    }
    else if constexpr (optional<type>) {
      if (pos_ + sizeof(bool) > size_) [[unlikely]] {
        return struct_pack::errc::no_buffer_space;
      }
      bool has_value{};
      std::memcpy(&has_value, data_ + pos_, sizeof(bool));
      pos_ += sizeof(bool);
      if (!has_value) [[unlikely]] {
        return {};
      }
      item = type{std::in_place_t{}};
      deserialize_one<size_type, NotSkip>(*item);
    }
    else if constexpr (variant<type>) {
      if (pos_ + sizeof(uint8_t) > size_) [[unlikely]] {
        return struct_pack::errc::no_buffer_space;
      }
      uint8_t index;
      std::memcpy(&index, data_ + pos_, sizeof(index));
      pos_ += sizeof(index);
      if (index >= std::variant_size_v<type>) [[unlikely]] {
        return struct_pack::errc::invalid_argument;
      }
      else {
        template_switch<variant_construct_helper,
                        std::integral_constant<std::size_t, size_type>,
                        std::integral_constant<bool, NotSkip>>(index, *this,
                                                               item);
      }
    }
    else if constexpr (expected<type>) {
      if (pos_ + sizeof(bool) > size_) [[unlikely]] {
        return struct_pack::errc::no_buffer_space;
      }
      bool has_value{};
      std::memcpy(&has_value, data_ + pos_, sizeof(bool));
      pos_ += sizeof(bool);
      if (has_value) {
        if constexpr (!std::is_same_v<typename type::value_type, void>)
          deserialize_one<size_type, NotSkip>(item.value());
      }
      else {
        typename type::error_type value;
        deserialize_one<size_type, NotSkip>(value);
        if constexpr (NotSkip) {
          item = typename type::unexpected_type{std::move(value)};
        }
      }
    }
    else if constexpr (std::is_class_v<type>) {
      if constexpr (!pair<type> && !is_trivial_tuple<type>)
        static_assert(std::is_aggregate_v<std::remove_cvref_t<type>>);
      if constexpr (is_trivial_serializable<type>::value) {
        if (pos_ + sizeof(type) > size_) [[unlikely]] {
          return struct_pack::errc::no_buffer_space;
        }
        if constexpr (NotSkip) {
          std::memcpy(&item, data_ + pos_, sizeof(type));
        }

        pos_ += sizeof(type);
      }
      else {
        visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
          code = deserialize_many<size_type>(items...);
        });
      }
    }
    else {
      static_assert(!sizeof(type), "the type is not supported yet");
    }
    return code;
  }

  template <size_t size_type, bool NotSkip, unique_ptr T>
  constexpr struct_pack::errc inline deserialize_one(T &item) {
    if (pos_ + sizeof(bool) > size_) [[unlikely]] {
      return struct_pack::errc::no_buffer_space;
    }
    bool has_value{};
    std::memcpy(&has_value, data_ + pos_, sizeof(bool));
    pos_ += sizeof(bool);
    if (!has_value) [[unlikely]] {
      return {};
    }
    item = std::make_unique<typename T::element_type>();
    deserialize_one<size_type, NotSkip>(*item);
    return struct_pack::errc{};
  }

  // partial deserialize_to
  template <size_t size_type, size_t I, size_t FieldIndex, typename FiledType,
            typename T>
  STRUCT_PACK_INLINE bool set_value(struct_pack::errc &err_code,
                                    FiledType &field, T &&t) {
    if constexpr (FieldIndex == I) {
      static_assert(std::is_same_v<std::remove_cvref_t<FiledType>,
                                   std::remove_cvref_t<T>>);
      err_code = deserialize_one<size_type, true>(field);
      return /*don't skip=*/true;
    }
    err_code = deserialize_one<size_type, false>(t);
    return /*don't skip=*/false;
  }

  template <int I, class... Ts>
  decltype(auto) get_nth(Ts &&...ts) {
    return std::get<I>(std::forward_as_tuple(ts...));
  }

  template <size_t size_type, size_t FieldIndex, typename FieldType,
            typename... Args>
  STRUCT_PACK_INLINE constexpr decltype(auto) for_each(FieldType &field,
                                                       Args &&...items) {
    bool stop = false;
    struct_pack::errc code{};
    [&]<std::size_t... I>(std::index_sequence<I...>) CONSTEXPR_INLINE_LAMBDA {
      ((!stop && (stop = set_value<size_type, I, FieldIndex>(
                      code, field, get_nth<I>(items...)))),
       ...);
    }
    (std::make_index_sequence<sizeof...(Args)>{});
    return code;
  }

  const Byte *data_;
  std::size_t size_;
  std::size_t pos_{};
  unsigned char size_type_;
};

}  // namespace detail
}  // namespace struct_pack
