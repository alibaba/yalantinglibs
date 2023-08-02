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

#include "error_code.hpp"
#include "md5_constexpr.hpp"
#include "reflection.hpp"
#include "trivial_view.hpp"
#include "varint.hpp"

#if __cplusplus >= 202002L
#include "tuple.hpp"
#endif

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
 *   struct_pack::compatible<bool, 20210101> id; // version number is 20210101
 *   struct_pack::compatible<double, 20210101> salary;
 *   std::string name;
 * };
 *
 * struct person_v3 {
 *   int age;
 *   struct_pack::compatible<std::string, 20210302> nickname; // new version
 * number is bigger than 20210101 struct_pack::compatible<bool, 20210101> id;
 *   struct_pack::compatible<double, 20210101> salary;
 *   std::string name;
 * };
 * ```
 *
 * `struct_pack::compatible<T, version_number>` can be null, thus ensuring
 * forward and backward compatibility.
 *
 * For example, serialize person_v2, and then deserialize it according to
 * person_v1, the extra fields will be directly ignored during deserialization.
 * If person_v1 is serialized and then deserialized according to person_v2, the
 * compatibale fields in person_v2 are all null values.
 *
 * The default value of version_number in `struct_pack::compatible<T,
 * version_number>` is 0.
 *
 * ```cpp
 * person_v2 p2{20, "tom", 114.1, "tom"};
 * auto buf = struct_pack::serialize(p2);
 *
 * person_v1 p1;
 * // deserialize person_v2 as person_v1
 * auto ec = struct_pack::deserialize_to(p1, buf.data(), buf.size());
 * CHECK(ec == std::errc{});
 *
 * auto buf1 = struct_pack::serialize(p1);
 * person_v2 p3;
 * // deserialize person_v1 as person_v2
 * auto ec = struct_pack::deserialize_to(p3, buf1.data(), buf1.size());
 * CHECK(ec == std::errc{});
 * ```
 *
 * When you update your struct:
 * 1. The new compatible field's version number should bigger than last changed.
 * 2. Don't remove or change any old field.
 *
 * For example,
 *
 * ```cpp
 * struct loginRequest_V1
 * {
 *   string user_name;
 *   string pass_word;
 *   struct_pack::compatible<string> verification_code;
 * };
 *
 * struct loginRequest_V2
 * {
 *   string user_name;
 *   string pass_word;
 *   struct_pack::compatible<network::ip> ip_address;
 * };
 *
 * auto data=struct_pack::serialize(loginRequest_V1{});
 * loginRequest_V2 req;
 * struct_pack::deserialize_to(req, data); // undefined behavior!
 *
 * ```
 *
 * ```cpp
 * struct loginRequest_V1
 * {
 *   string user_name;
 *   string pass_word;
 *   struct_pack::compatible<string, 20210101> verification_code;
 * };
 *
 * struct loginRequest_V2
 * {
 *   string user_name;
 *   string pass_word;
 *   struct_pack::compatible<string, 20210101> verification_code;
 *   struct_pack::compatible<network::ip, 20000101> ip_address;
 * };
 *
 * auto data=struct_pack::serialize(loginRequest_V1{});
 * loginRequest_V2 req;
 * struct_pack::deserialize_to(req, data);  // undefined behavior!
 *
 * The value of compatible can be empty.
 *
 * TODO: add doc
 *
 * @tparam T field type
 */

template <typename T, uint64_t version>
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
  friend bool operator==(const compatible<T, version> &self,
                         const compatible<T, version> &other) {
    return static_cast<bool>(self) == static_cast<bool>(other) &&
           (!self || *self == *other);
  }
  static constexpr uint64_t version_number = version;
};

using var_int32_t = detail::sint<int32_t>;
using var_int64_t = detail::sint<int64_t>;
using var_uint32_t = detail::varint<uint32_t>;
using var_uint64_t = detail::varint<uint64_t>;

enum type_info_config { automatic = 0, disable = 1, enable = 2 };

template <typename T>
constexpr inline type_info_config enable_type_info =
    type_info_config::automatic;

template <typename... Args>
STRUCT_PACK_INLINE constexpr decltype(auto) get_type_literal();

struct serialize_buffer_size;

namespace detail {
template <uint64_t conf, typename... Args>
STRUCT_PACK_INLINE constexpr serialize_buffer_size get_serialize_runtime_info(
    const Args &...args);
}

struct serialize_buffer_size {
 private:
  std::size_t len_;
  unsigned char metainfo_;

 public:
  constexpr serialize_buffer_size() : len_(sizeof(uint32_t)), metainfo_(0) {}
  constexpr std::size_t size() const { return len_; }
  constexpr unsigned char metainfo() const { return metainfo_; }
  constexpr operator std::size_t() const { return len_; }

  template <uint64_t conf, typename... Args>
  friend STRUCT_PACK_INLINE constexpr serialize_buffer_size
  struct_pack::detail::get_serialize_runtime_info(const Args &...args);
};

namespace detail {

#if __cplusplus >= 202002L
template <typename... T>
constexpr inline bool is_trivial_tuple<tuplet::tuple<T...>> = true;
#endif

template <typename T>
[[noreturn]] constexpr T declval() {
  unreachable();
}

template <typename U>
constexpr auto get_types() {
  using T = remove_cvref_t<U>;
  if constexpr (std::is_fundamental_v<T> || std::is_enum_v<T> || varint_t<T> ||
                string<T> || container<T> || optional<T> || unique_ptr<T> ||
                is_variant_v<T> || expected<T> || array<T> || c_array<T> ||
                std::is_same_v<std::monostate, T>
#if __GNUC__ || __clang__
                || std::is_same_v<__int128, T> ||
                std::is_same_v<unsigned __int128, T>
#endif
  ) {
    return declval<std::tuple<T>>();
  }
  else if constexpr (tuple<T>) {
    return declval<T>();
  }
  else if constexpr (is_trivial_tuple<T>) {
    return declval<T>();
  }
  else if constexpr (pair<T>) {
    return declval<
        std::tuple<typename T::first_type, typename T::second_type>>();
  }
  else if constexpr (std::is_class_v<T>) {
    // clang-format off
    return visit_members(
        declval<T>(), [](auto &&... args) constexpr {
          return declval<std::tuple<remove_cvref_t<decltype(args)>...>>();
        });
    // clang-format on
  }
  else {
    static_assert(!sizeof(T), "the type is not supported!");
  }
}

// clang-format off

#if __cpp_concepts < 201907L

template <typename T, typename = void>
struct trivially_copyable_container_impl : std::false_type {};

template <typename T>
struct trivially_copyable_container_impl<T, std::void_t<std::enable_if_t<is_trivial_serializable<typename T::value_type>::value>>>
    : std::true_type {};

template <typename Type>
constexpr bool trivially_copyable_container =
    continuous_container<Type> && trivially_copyable_container_impl<Type>::value;

#else

template <typename Type>
constexpr bool trivially_copyable_container =
    continuous_container<Type> &&
    requires(Type container) {
      requires is_trivial_serializable<typename Type::value_type>::value;
    };

#endif
// clang-format on
template <typename T>
constexpr bool struct_pack_byte =
    std::is_same_v<char, T> || std::is_same_v<unsigned char, T> ||
    std::is_same_v<std::byte, T>;

template <typename T>
#if __cpp_concepts < 201907L
constexpr bool
#else
concept
#endif
    struct_pack_buffer = trivially_copyable_container<T> &&
    struct_pack_byte<typename T::value_type>;

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
  int128_t,  // Tips: We only support 128-bit integer on gcc clang
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
constexpr type_id get_varint_type() {
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
constexpr type_id get_integral_type() {
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
  else if constexpr (std::is_same_v<char, T>
#ifdef __cpp_lib_char8_t
                     || std::is_same_v<char8_t, T>
#endif
  ) {
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
                  "sizeof(char32_t)!=4, which is not supported.");
    return type_id::char_32_t;
  }
  else if constexpr (std::is_same_v<bool, T> && sizeof(bool)) {
    static_assert(sizeof(bool) == 1,
                  "sizeof(bool)!=1, which is not supported.");
    return type_id::bool_t;
  }
#if __GNUC__ || __clang__
  //-std=gnu++20
  else if constexpr (std::is_same_v<__int128, T>) {
    return type_id::int128_t;
  }
  else if constexpr (std::is_same_v<unsigned __int128, T>) {
    return type_id::uint128_t;
  }
#endif
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
constexpr type_id get_floating_point_type() {
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
constexpr type_id get_type_id() {
  static_assert(CHAR_BIT == 8);
  // compatible member, which should be ignored in MD5 calculated.
  if constexpr (optional<T> && is_compatible_v<T>) {
    return type_id::compatible_t;
  }
  else if constexpr (std::is_enum_v<T>) {
    return get_integral_type<std::underlying_type_t<T>>();
  }
  else if constexpr (std::is_integral_v<T>
#if __GNUC__ || __CLANG__
                     || std::is_same_v<__int128, T> ||
                     std::is_same_v<unsigned __int128, T>
#endif
  ) {
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
  else if constexpr (array<T> || c_array<T> || static_span<T>) {
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
  else if constexpr (is_variant_v<T>) {
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
    return type_id::trivial_class_t;
  }
  else {
    static_assert(!sizeof(T), "not supported type");
  }
}  // namespace detail

template <size_t size>
constexpr decltype(auto) get_size_literal() {
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

template <typename Arg, typename... ParentArgs, std::size_t... I>
constexpr std::size_t check_circle_impl(std::index_sequence<I...>) {
  using types_tuple = std::tuple<ParentArgs...>;
  return (std::max)(
      {(std::is_same_v<std::tuple_element_t<I, types_tuple>, Arg> ? I + 1
                                                                  : 0)...});
}

template <typename Arg, typename... ParentArgs>
constexpr std::size_t check_circle() {
  if constexpr (sizeof...(ParentArgs) != 0) {
    return check_circle_impl<Arg, ParentArgs...>(
        std::make_index_sequence<sizeof...(ParentArgs)>());
  }
  else {
    return 0;
  }
}

template <typename T>
struct get_array_element {
  using type = typename T::value_type;
};

template <typename T, std::size_t sz>
struct get_array_element<T[sz]> {
  using type = T;
};

template <typename T>
std::size_t constexpr get_array_size() {
  if constexpr (array<T> || c_array<T>) {
    return sizeof(T) / sizeof(typename get_array_element<T>::type);
  }
  else {
    return T::extent;
  }
}

struct size_info {
  std::size_t total;
  std::size_t size_cnt;
  std::size_t max_size;
  constexpr size_info &operator+=(const size_info &other) {
    this->total += other.total;
    this->size_cnt += other.size_cnt;
    this->max_size = (std::max)(this->max_size, other.max_size);
    return *this;
  }
  constexpr size_info operator+(const size_info &other) {
    return {this->total + other.total, this->size_cnt += other.size_cnt,
            (std::max)(this->max_size, other.max_size)};
  }
};
template <typename T>
constexpr size_info inline calculate_one_size(const T &item);

namespace align {

template <typename T>
constexpr std::size_t alignment_impl();

template <typename T>
constexpr std::size_t alignment_v = alignment_impl<T>();

template <typename type, std::size_t... I>
constexpr std::size_t default_alignment_helper(std::index_sequence<I...>) {
  return (std::max)(
      {(is_compatible_v<remove_cvref_t<std::tuple_element_t<I, type>>>
            ? std::size_t{0}
            : align::alignment_v<
                  remove_cvref_t<std::tuple_element_t<I, type>>>)...});
}

template <typename T>
constexpr std::size_t default_alignment() {
  if constexpr (!is_trivial_serializable<T>::value && !is_trivial_view_v<T>) {
    using type = decltype(get_types<T>());
    return default_alignment_helper<type>(
        std::make_index_sequence<std::tuple_size_v<type>>());
  }
  else if constexpr (is_trivial_view_v<T>) {
    return std::alignment_of_v<typename T::value_type>;
  }
  else {
    return std::alignment_of_v<T>;
  }
}
template <typename T>
constexpr std::size_t default_alignment_v = default_alignment<T>();

template <typename T>
constexpr std::size_t alignment_impl();

template <typename type, std::size_t... I>
constexpr std::size_t pack_alignment_impl_helper(std::index_sequence<I...>) {
  return (std::max)(
      {(is_compatible_v<remove_cvref_t<std::tuple_element_t<I, type>>>
            ? std::size_t{0}
            : align::alignment_v<
                  remove_cvref_t<std::tuple_element_t<I, type>>>)...});
}

template <typename T>
constexpr std::size_t pack_alignment_impl() {
  static_assert(std::is_class_v<T>);
  static_assert(!is_trivial_view_v<T>);
  constexpr auto ret = struct_pack::pack_alignment_v<T>;
  static_assert(ret == 0 || ret == 1 || ret == 2 || ret == 4 || ret == 8 ||
                ret == 16);
  if constexpr (ret == 0) {
    using type = decltype(get_types<T>());
    return pack_alignment_impl_helper<type>(
        std::make_index_sequence<std::tuple_size_v<type>>());
  }
  else {
    return ret;
  }
}

template <typename T>
constexpr std::size_t pack_alignment_v = pack_alignment_impl<T>();

template <typename T>
constexpr std::size_t alignment_impl() {
  if constexpr (struct_pack::alignment_v<T> == 0 &&
                struct_pack::pack_alignment_v<T> == 0) {
    return default_alignment_v<T>;
  }
  else if constexpr (struct_pack::alignment_v<T> != 0) {
    if constexpr (is_trivial_serializable<T>::value) {
      static_assert(default_alignment_v<T> == alignment_v<T>);
    }
    constexpr auto ret = struct_pack::alignment_v<T>;
    static_assert(
        [](std::size_t align) constexpr {
          while (align % 2 == 0) {
            align /= 2;
          }
          return align == 1;
        }(ret),
        "alignment should be power of 2");
    return ret;
  }
  else {
    if constexpr (is_trivial_serializable<T>::value) {
      return default_alignment_v<T>;
    }
    else {
      return pack_alignment_v<T>;
    }
  }
}

template <typename P, typename T, std::size_t I>
struct calculate_trival_obj_size {
  constexpr void operator()(std::size_t &total);
};

template <typename P, typename T, std::size_t I>
struct calculate_padding_size_impl {
  constexpr void operator()(
      std::size_t &offset,
      std::array<std::size_t, struct_pack::members_count<P> + 1>
          &padding_size) {
    if constexpr (is_compatible_v<T>) {
      padding_size[I] = 0;
    }
    else if constexpr (is_trivial_view_v<T>) {
      calculate_padding_size_impl<P, typename T::value_type, I>{}(offset,
                                                                  padding_size);
    }
    else {
      if (offset % align::alignment_v<T>) {
        padding_size[I] =
            (std::min)(align::pack_alignment_v<P> - 1,
                       align::alignment_v<T> - offset % align::alignment_v<T>);
      }
      else {
        padding_size[I] = 0;
      }
      offset += padding_size[I];
      if constexpr (is_trivial_serializable<T>::value)
        offset += sizeof(T);
      else {
        for_each<T, calculate_trival_obj_size>(offset);
        static_assert(is_trivial_serializable<T, true>::value);
      }
    }
  }
};

template <typename T>
constexpr auto calculate_padding_size() {
  std::array<std::size_t, struct_pack::members_count<T> + 1> padding_size{};
  std::size_t offset = 0;
  for_each<T, calculate_padding_size_impl>(offset, padding_size);
  if (offset % align::alignment_v<T>) {
    padding_size[struct_pack::members_count<T>] =
        align::alignment_v<T> - offset % align::alignment_v<T>;
  }
  else {
    padding_size[struct_pack::members_count<T>] = 0;
  }
  return padding_size;
}
template <typename T>
constexpr std::array<std::size_t, struct_pack::members_count<T> + 1>
    padding_size = calculate_padding_size<T>();
template <typename T>
constexpr std::size_t get_total_padding_size() {
  std::size_t sum = 0;
  for (auto &e : padding_size<T>) {
    sum += e;
  }
  return sum;
};
template <typename T>
constexpr std::size_t total_padding_size = get_total_padding_size<T>();

template <typename P, typename T, std::size_t I>
using calculate_trival_obj_size_wrapper = calculate_trival_obj_size<P, T, I>;

template <typename P, typename T, std::size_t I>
constexpr void calculate_trival_obj_size<P, T, I>::operator()(
    std::size_t &total) {
  if constexpr (I == 0) {
    total += total_padding_size<P>;
  }
  if constexpr (!is_compatible_v<T>) {
    if constexpr (is_trivial_serializable<T>::value) {
      total += sizeof(T);
    }
    else if constexpr (is_trivial_view_v<T>) {
      total += sizeof(typename T::value_type);
    }
    else {
      static_assert(is_trivial_serializable<T, true>::value);
      std::size_t offset = 0;
      for_each<T, calculate_trival_obj_size_wrapper>(offset);
      total += offset;
    }
  }
}

}  // namespace align

// This help function is just to improve unit test coverage. :)
// The original lambada in `get_type_literal` is a compile-time expression.
// Currently, the unit test coverage tools like
// [Coverage](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html)
// can not detect code that is run at compile time.
template <typename Args, typename... ParentArgs, std::size_t... I>
constexpr decltype(auto) get_type_literal(std::index_sequence<I...>);

template <typename Args, typename... ParentArgs, std::size_t... I>
constexpr decltype(auto) get_variant_literal(std::index_sequence<I...>);

template <typename Arg, typename... ParentArgs>
constexpr decltype(auto) get_type_literal() {
  if constexpr (is_trivial_view_v<Arg>) {
    return get_type_literal<typename Arg::value_type, ParentArgs...>();
  }
  else {
    constexpr std::size_t has_cycle = check_circle<Arg, ParentArgs...>();
    if constexpr (has_cycle != 0) {
      static_assert(has_cycle >= 2);
      return string_literal<char, 1>{
                 {static_cast<char>(type_id::circle_flag)}} +
             get_size_literal<has_cycle - 2>();
    }
    else {
      constexpr auto id = get_type_id<Arg>();
      constexpr auto ret = string_literal<char, 1>{{static_cast<char>(id)}};
      if constexpr (id == type_id::non_trivial_class_t ||
                    id == type_id::trivial_class_t) {
        using Args = decltype(get_types<Arg>());
        constexpr auto body = get_type_literal<Args, Arg, ParentArgs...>(
            std::make_index_sequence<std::tuple_size_v<Args>>());
        if constexpr (is_trivial_serializable<Arg, true>::value) {
          static_assert(
              align::pack_alignment_v<Arg> <= align::alignment_v<Arg>,
              "If you add #pragma_pack to a struct, please specify the "
              "struct_pack::pack_alignment_v<T>.");
          constexpr auto end = string_literal<char, 1>{
              {static_cast<char>(type_id::type_end_flag)}};
          return ret + body + get_size_literal<align::pack_alignment_v<Arg>>() +
                 get_size_literal<align::alignment_v<Arg>>() + end;
        }
        else {
          constexpr auto end = string_literal<char, 1>{
              {static_cast<char>(type_id::type_end_flag)}};
          return ret + body + end;
        }
      }
      else if constexpr (id == type_id::variant_t) {
        constexpr auto sz = std::variant_size_v<Arg>;
        static_assert(sz > 0, "empty param of std::variant is not allowed!");
        static_assert(sz < 256, "too many alternative type in variant!");
        constexpr auto body = get_variant_literal<Arg, ParentArgs...>(
            std::make_index_sequence<std::variant_size_v<Arg>>());
        constexpr auto end = string_literal<char, 1>{
            {static_cast<char>(type_id::type_end_flag)}};
        return ret + body + end;
      }
      else if constexpr (id == type_id::array_t) {
        constexpr auto sz = get_array_size<Arg>();
        static_assert(sz > 0, "The array's size must greater than zero!");
        return ret +
               get_type_literal<
                   remove_cvref_t<decltype(std::declval<Arg>()[0])>, Arg,
                   ParentArgs...>() +
               get_size_literal<sz>();
      }
      else if constexpr (unique_ptr<Arg>) {
        return ret +
               get_type_literal<remove_cvref_t<typename Arg::element_type>, Arg,
                                ParentArgs...>();
      }
      else if constexpr (id == type_id::container_t ||
                         id == type_id::optional_t || id == type_id::string_t) {
        return ret + get_type_literal<remove_cvref_t<typename Arg::value_type>,
                                      Arg, ParentArgs...>();
      }
      else if constexpr (id == type_id::set_container_t) {
        return ret + get_type_literal<remove_cvref_t<typename Arg::key_type>,
                                      Arg, ParentArgs...>();
      }
      else if constexpr (id == type_id::map_container_t) {
        return ret +
               get_type_literal<remove_cvref_t<typename Arg::key_type>, Arg,
                                ParentArgs...>() +
               get_type_literal<remove_cvref_t<typename Arg::mapped_type>, Arg,
                                ParentArgs...>();
      }
      else if constexpr (id == type_id::expected_t) {
        return ret +
               get_type_literal<remove_cvref_t<typename Arg::value_type>, Arg,
                                ParentArgs...>() +
               get_type_literal<remove_cvref_t<typename Arg::error_type>, Arg,
                                ParentArgs...>();
      }
      else if constexpr (id != type_id::compatible_t) {
        return ret;
      }
      else {
        return string_literal<char, 0>{};
      }
    }
  }
}

template <typename Args, typename... ParentArgs, std::size_t... I>
constexpr decltype(auto) get_type_literal(std::index_sequence<I...>) {
  return ((get_type_literal<remove_cvref_t<std::tuple_element_t<I, Args>>,
                            ParentArgs...>()) +
          ...);
}

template <typename Args, typename... ParentArgs, std::size_t... I>
constexpr decltype(auto) get_variant_literal(std::index_sequence<I...>) {
  return ((get_type_literal<remove_cvref_t<std::variant_alternative_t<I, Args>>,
                            Args, ParentArgs...>()) +
          ...);
}

template <typename Parent, typename... Args>
constexpr decltype(auto) get_types_literal_impl() {
  if constexpr (std::is_same_v<Parent, void>)
    return (get_type_literal<Args>() + ...);
  else
    return (get_type_literal<Args, Parent>() + ...);
}

template <typename T, typename... Args>
constexpr decltype(auto) get_types_literal() {
  if constexpr (is_trivial_view_v<T>) {
    return get_types_literal<T::value_type, Args...>();
  }
  else {
    constexpr auto root_id = get_type_id<remove_cvref_t<T>>();
    constexpr auto end =
        string_literal<char, 1>{{static_cast<char>(type_id::type_end_flag)}};
    if constexpr (root_id == type_id::non_trivial_class_t ||
                  root_id == type_id::trivial_class_t) {
      constexpr auto begin =
          string_literal<char, 1>{{static_cast<char>(root_id)}};
      constexpr auto body = get_types_literal_impl<T, Args...>();
      if constexpr (is_trivial_serializable<T, true>::value) {
        static_assert(align::pack_alignment_v<T> <= align::alignment_v<T>,
                      "If you add #pragma_pack to a struct, please specify the "
                      "struct_pack::pack_alignment_v<T>.");
        return begin + body + get_size_literal<align::pack_alignment_v<T>>() +
               get_size_literal<align::alignment_v<T>>() + end;
      }
      else {
        return begin + body + end;
      }
    }
    else {
      return get_types_literal_impl<void, Args...>();
    }
  }
}

template <typename T, typename Tuple, std::size_t... I>
constexpr decltype(auto) get_types_literal(std::index_sequence<I...>) {
  return get_types_literal<T,
                           remove_cvref_t<std::tuple_element_t<I, Tuple>>...>();
}

template <uint64_t version, typename Args, typename... ParentArgs>
constexpr bool check_if_compatible_element_exist_impl_helper();

template <uint64_t version, typename Args, typename... ParentArgs,
          std::size_t... I>
constexpr bool check_if_compatible_element_exist_impl(
    std::index_sequence<I...>) {
  return (check_if_compatible_element_exist_impl_helper<
              version, remove_cvref_t<std::tuple_element_t<I, Args>>,
              ParentArgs...>() ||
          ...);
}

template <uint64_t version, typename Arg, typename... ParentArgs,
          std::size_t... I>
constexpr bool check_if_compatible_element_exist_impl_variant(
    std::index_sequence<I...>) {
  return (check_if_compatible_element_exist_impl_helper<
              version, remove_cvref_t<std::variant_alternative_t<I, Arg>>,
              ParentArgs...>() ||
          ...);
}

template <uint64_t version, typename Arg, typename... ParentArgs>
constexpr bool check_if_compatible_element_exist_impl_helper() {
  using T = remove_cvref_t<Arg>;
  constexpr auto id = get_type_id<T>();
  if constexpr (is_trivial_view_v<Arg>) {
    return check_if_compatible_element_exist_impl_helper<
        version, typename Arg::value_type, ParentArgs...>();
  }
  else if constexpr (check_circle<Arg, ParentArgs...>() != 0) {
    return false;
  }
  else if constexpr (id == type_id::compatible_t) {
    if constexpr (version != UINT64_MAX)
      return T::version_number == version;
    else
      return true;
  }
  else {
    if constexpr (id == type_id::non_trivial_class_t ||
                  id == type_id::trivial_class_t) {
      using subArgs = decltype(get_types<T>());
      return check_if_compatible_element_exist_impl<version, subArgs, T,
                                                    ParentArgs...>(
          std::make_index_sequence<std::tuple_size_v<subArgs>>());
    }
    else if constexpr (id == type_id::optional_t) {
      if constexpr (unique_ptr<T>) {
        return check_if_compatible_element_exist_impl_helper<
            version, typename T::element_type, T, ParentArgs...>();
      }
      else {
        return check_if_compatible_element_exist_impl_helper<
            version, typename T::value_type, T, ParentArgs...>();
      }
    }
    else if constexpr (id == type_id::array_t) {
      return check_if_compatible_element_exist_impl_helper<
          version, remove_cvref_t<typename get_array_element<T>::type>, T,
          ParentArgs...>();
    }
    else if constexpr (id == type_id::map_container_t) {
      return check_if_compatible_element_exist_impl_helper<
                 version, typename T::key_type, T, ParentArgs...>() ||
             check_if_compatible_element_exist_impl_helper<
                 version, typename T::mapped_type, T, ParentArgs...>();
    }
    else if constexpr (id == type_id::set_container_t ||
                       id == type_id::container_t) {
      return check_if_compatible_element_exist_impl_helper<
          version, typename T::value_type, T, ParentArgs...>();
    }
    else if constexpr (id == type_id::expected_t) {
      return check_if_compatible_element_exist_impl_helper<
                 version, typename T::value_type, T, ParentArgs...>() ||
             check_if_compatible_element_exist_impl_helper<
                 version, typename T::error_type, T, ParentArgs...>();
    }
    else if constexpr (id == type_id::variant_t) {
      return check_if_compatible_element_exist_impl_variant<version, T, T,
                                                            ParentArgs...>(
          std::make_index_sequence<std::variant_size_v<T>>{});
    }
    else {
      return false;
    }
  }
}

template <typename T, typename... Args>
constexpr uint32_t get_types_code_impl() {
  constexpr auto str = get_types_literal<T, remove_cvref_t<Args>...>();
  return MD5::MD5Hash32Constexpr(str.data(), str.size());
}

template <typename T, typename Tuple, size_t... I>
constexpr uint32_t get_types_code(std::index_sequence<I...>) {
  return get_types_code_impl<T, std::tuple_element_t<I, Tuple>...>();
}

template <typename T, typename... Args>
constexpr size_info STRUCT_PACK_INLINE
calculate_payload_size(const T &item, const Args &...items);

template <typename T>
constexpr size_info inline calculate_one_size(const T &item) {
  constexpr auto id = get_type_id<remove_cvref_t<T>>();
  static_assert(id != detail::type_id::type_end_flag);
  using type = remove_cvref_t<decltype(item)>;
  static_assert(!std::is_pointer_v<type>);
  size_info ret{};
  if constexpr (id == type_id::monostate_t) {
  }
  else if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type> ||
                     id == type_id::int128_t || id == type_id::uint128_t) {
    ret.total = sizeof(type);
  }
  else if constexpr (is_trivial_view_v<type>) {
    return calculate_one_size(item.get());
  }
  else if constexpr (detail::varint_t<type>) {
    ret.total = detail::calculate_varint_size(item);
  }
  else if constexpr (id == type_id::array_t) {
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
    ret.max_size = (std::max)(ret.max_size, item.size());
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
  else if constexpr (optional<type> || unique_ptr<type>) {
    ret.total = sizeof(char);
    if (item) {
      ret += calculate_one_size(*item);
    }
  }
  else if constexpr (is_variant_v<type>) {
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
    if constexpr (!pair<type> && !is_trivial_tuple<type>) {
      if constexpr (!user_defined_refl<type>)
        static_assert(std::is_aggregate_v<remove_cvref_t<type>>,
                      "struct_pack only support aggregated type, or you should "
                      "add macro STRUCT_PACK_REFL(Type,field1,field2...)");
    }
    if constexpr (is_trivial_serializable<type>::value) {
      ret.total = sizeof(type);
    }
    else if constexpr (is_trivial_serializable<type, true>::value) {
      visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
        ret += calculate_payload_size(items...);
        ret.total += align::total_padding_size<type>;
      });
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

template <typename T, typename... Args>
constexpr size_info STRUCT_PACK_INLINE
calculate_payload_size(const T &item, const Args &...items) {
  if constexpr (sizeof...(items) != 0)
    return calculate_one_size(item) + calculate_payload_size(items...);
  else
    return calculate_one_size(item);
}

template <typename T, typename Tuple>
constexpr uint32_t get_types_code() {
  return detail::get_types_code<T, Tuple>(
      std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename T, uint64_t version = UINT64_MAX>
constexpr bool check_if_compatible_element_exist() {
  using U = remove_cvref_t<T>;
  return detail::check_if_compatible_element_exist_impl<version, U>(
      std::make_index_sequence<std::tuple_size_v<U>>{});
}

template <typename T, uint64_t version = UINT64_MAX>
constexpr bool exist_compatible_member =
    check_if_compatible_element_exist<decltype(get_types<T>()), version>();
// clang-format off
template <typename T, uint64_t version = UINT64_MAX>
constexpr bool unexist_compatible_member = !
exist_compatible_member<decltype(get_types<T>()), version>;
// clang-format on
template <typename Args, typename... ParentArgs>
constexpr std::size_t calculate_compatible_version_size();

template <typename Args, typename... ParentArgs, std::size_t... I>
constexpr std::size_t calculate_compatible_version_size(
    std::index_sequence<I...>) {
  return (calculate_compatible_version_size<
              remove_cvref_t<std::tuple_element_t<I, Args>>, ParentArgs...>() +
          ...);
}

template <typename Arg, typename... ParentArgs, std::size_t... I>
constexpr std::size_t calculate_variant_compatible_version_size(
    std::index_sequence<I...>) {
  return (
      calculate_compatible_version_size<
          remove_cvref_t<std::variant_alternative_t<I, Arg>>, ParentArgs...>() +
      ...);
}

template <typename Arg, typename... ParentArgs>
constexpr std::size_t calculate_compatible_version_size() {
  using T = remove_cvref_t<Arg>;
  constexpr auto id = get_type_id<T>();
  std::size_t sz = 0;
  if constexpr (is_trivial_view_v<T>) {
    return 0;
  }
  if constexpr (check_circle<T, ParentArgs...>())
    sz = 0;
  else if constexpr (id == type_id::compatible_t) {
    sz = 1;
  }
  else {
    if constexpr (id == type_id::non_trivial_class_t ||
                  id == type_id::trivial_class_t) {
      using subArgs = decltype(get_types<T>());
      return calculate_compatible_version_size<subArgs, T, ParentArgs...>(
          std::make_index_sequence<std::tuple_size_v<subArgs>>());
    }
    else if constexpr (id == type_id::optional_t) {
      if constexpr (unique_ptr<T>) {
        sz = calculate_compatible_version_size<typename T::element_type, T,
                                               ParentArgs...>();
      }
      else {
        sz = calculate_compatible_version_size<typename T::value_type, T,
                                               ParentArgs...>();
      }
    }
    else if constexpr (id == type_id::array_t) {
      return calculate_compatible_version_size<
          remove_cvref_t<typename get_array_element<T>::type>, T,
          ParentArgs...>();
    }
    else if constexpr (id == type_id::map_container_t) {
      return calculate_compatible_version_size<typename T::key_type, T,
                                               ParentArgs...>() +
             calculate_compatible_version_size<typename T::mapped_type, T,
                                               ParentArgs...>();
    }
    else if constexpr (id == type_id::set_container_t ||
                       id == type_id::container_t) {
      return calculate_compatible_version_size<typename T::value_type, T,
                                               ParentArgs...>();
    }
    else if constexpr (id == type_id::expected_t) {
      return calculate_compatible_version_size<typename T::value_type, T,
                                               ParentArgs...>() +
             calculate_compatible_version_size<typename T::error_type, T,
                                               ParentArgs...>();
    }
    else if constexpr (id == type_id::variant_t) {
      return calculate_variant_compatible_version_size<T, T, ParentArgs...>(
          std::make_index_sequence<std::variant_size_v<T>>{});
    }
  }
  return sz;
}

template <typename Buffer, typename Args, typename... ParentArgs>
constexpr void get_compatible_version_numbers(Buffer &buffer, std::size_t &sz);

template <typename Buffer, typename Args, typename... ParentArgs,
          std::size_t... I>
constexpr void get_compatible_version_numbers(Buffer &buffer, std::size_t &sz,
                                              std::index_sequence<I...>) {
  return (
      get_compatible_version_numbers<
          Buffer, remove_cvref_t<std::tuple_element_t<I, Args>>, ParentArgs...>(
          buffer, sz),
      ...);
}

template <typename Buffer, typename Arg, typename... ParentArgs,
          std::size_t... I>
constexpr void get_variant_compatible_version_numbers(
    Buffer &buffer, std::size_t &sz, std::index_sequence<I...>) {
  return (get_compatible_version_numbers<
              Buffer, remove_cvref_t<std::variant_alternative_t<I, Arg>>,
              ParentArgs...>(buffer, sz),
          ...);
}

template <typename Buffer, typename Arg, typename... ParentArgs>
constexpr void get_compatible_version_numbers(Buffer &buffer, std::size_t &sz) {
  using T = remove_cvref_t<Arg>;
  constexpr auto id = get_type_id<T>();
  if constexpr (is_trivial_view_v<T>) {
    return;
  }
  else if constexpr (check_circle<T, ParentArgs...>()) {
    return;
  }
  else if constexpr (id == type_id::compatible_t) {
    buffer[sz++] = T::version_number;
    return;
  }
  else {
    if constexpr (id == type_id::non_trivial_class_t ||
                  id == type_id::trivial_class_t) {
      using subArgs = decltype(get_types<T>());
      get_compatible_version_numbers<Buffer, subArgs, T, ParentArgs...>(
          buffer, sz, std::make_index_sequence<std::tuple_size_v<subArgs>>());
    }
    else if constexpr (id == type_id::optional_t) {
      if constexpr (unique_ptr<T>) {
        get_compatible_version_numbers<Buffer, typename T::element_type, T,
                                       ParentArgs...>(buffer, sz);
      }
      else {
        get_compatible_version_numbers<Buffer, typename T::value_type, T,
                                       ParentArgs...>(buffer, sz);
      }
    }
    else if constexpr (id == type_id::array_t) {
      get_compatible_version_numbers<
          Buffer, remove_cvref_t<typename get_array_element<T>::type>, T,
          ParentArgs...>(buffer, sz);
    }
    else if constexpr (id == type_id::map_container_t) {
      get_compatible_version_numbers<Buffer, typename T::key_type, T,
                                     ParentArgs...>(buffer, sz);
      get_compatible_version_numbers<Buffer, typename T::mapped_type, T,
                                     ParentArgs...>(buffer, sz);
    }
    else if constexpr (id == type_id::set_container_t ||
                       id == type_id::container_t) {
      get_compatible_version_numbers<Buffer, typename T::value_type, T,
                                     ParentArgs...>(buffer, sz);
    }
    else if constexpr (id == type_id::expected_t) {
      get_compatible_version_numbers<Buffer, typename T::value_type, T,
                                     ParentArgs...>(buffer, sz);
      get_compatible_version_numbers<Buffer, typename T::error_type, T,
                                     ParentArgs...>(buffer, sz);
    }
    else if constexpr (id == type_id::variant_t) {
      get_variant_compatible_version_numbers<Buffer, T, T, ParentArgs...>(
          buffer, sz, std::make_index_sequence<std::variant_size_v<T>>{});
    }
  }
}

template <std::size_t sz>
constexpr void STRUCT_PACK_INLINE
compile_time_sort(std::array<uint64_t, sz> &array) {
  // FIXME: use faster compile-time sort
  for (std::size_t i = 0; i < array.size(); ++i) {
    for (std::size_t j = i + 1; j < array.size(); ++j) {
      if (array[i] > array[j]) {
        auto tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
      }
    }
  }
  return;
}

template <std::size_t sz>
constexpr std::size_t STRUCT_PACK_INLINE
calculate_uniqued_size(const std::array<uint64_t, sz> &input) {
  std::size_t unique_cnt = sz;
  for (std::size_t i = 1; i < input.size(); ++i) {
    if (input[i] == input[i - 1]) {
      --unique_cnt;
    }
  }
  return unique_cnt;
}

template <std::size_t sz1, std::size_t sz2>
constexpr void STRUCT_PACK_INLINE compile_time_unique(
    const std::array<uint64_t, sz1> &input, std::array<uint64_t, sz2> &output) {
  std::size_t j = 0;
  static_assert(sz1 != 0, "not allow empty input!");
  output[0] = input[0];
  for (std::size_t i = 1; i < input.size(); ++i) {
    if (input[i] != input[i - 1]) {
      output[++j] = input[i];
    }
  }
}

template <typename T>
constexpr auto STRUCT_PACK_INLINE get_sorted_compatible_version_numbers() {
  std::array<uint64_t, calculate_compatible_version_size<T>()> buffer{};
  std::size_t sz = 0;
  get_compatible_version_numbers<decltype(buffer), T>(buffer, sz);
  compile_time_sort(buffer);
  return buffer;
}

template <typename T>
constexpr auto STRUCT_PACK_INLINE
get_sorted_and_uniqued_compatible_version_numbers() {
  constexpr auto buffer = get_sorted_compatible_version_numbers<T>();
  std::array<uint64_t, calculate_uniqued_size(buffer)> uniqued_buffer{};
  compile_time_unique(buffer, uniqued_buffer);
  return uniqued_buffer;
}

template <typename T>
constexpr auto compatible_version_number =
    get_sorted_and_uniqued_compatible_version_numbers<T>();

template <typename T>
struct serialize_static_config {
  static constexpr bool has_compatible = exist_compatible_member<T>;
#ifdef NDEBUG
  static constexpr bool has_type_literal = false;
#else
  static constexpr bool has_type_literal = true;
#endif
};

template <typename... Args>
using get_args_type = remove_cvref_t<typename std::conditional<
    sizeof...(Args) == 1, std::tuple_element_t<0, std::tuple<Args...>>,
    std::tuple<Args...>>::type>;
template <uint64_t conf, typename T>
constexpr bool check_if_add_type_literal() {
  if constexpr (conf == type_info_config::automatic) {
    if constexpr (enable_type_info<T> == type_info_config::automatic) {
      return serialize_static_config<T>::has_type_literal;
    }
    else {
      return enable_type_info<T> == type_info_config::enable;
    }
  }
  else {
    return conf == type_info_config::enable;
  }
}

template <uint64_t conf, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE constexpr serialize_buffer_size
get_serialize_runtime_info(const Args &...args) {
  using Type = get_args_type<Args...>;
  constexpr bool has_compatible = serialize_static_config<Type>::has_compatible;
  constexpr bool has_type_literal = check_if_add_type_literal<conf, Type>();
  serialize_buffer_size ret;
  auto sz_info = calculate_payload_size(args...);

  if SP_LIKELY (sz_info.max_size < (int64_t{1} << 8)) {
    ret.len_ += sz_info.total + sz_info.size_cnt;
    ret.metainfo_ = 0b00000;
    constexpr bool has_compile_time_determined_meta_info =
        has_compatible || has_type_literal;
    if constexpr (has_compile_time_determined_meta_info) {
      ret.len_ += sizeof(unsigned char);
    }
  }
  else {
    if (sz_info.max_size < (int64_t{1} << 16)) {
      ret.len_ += sz_info.total + sz_info.size_cnt * 2;
      ret.metainfo_ = 0b01000;
    }
    else if (sz_info.max_size < (int64_t{1} << 32)) {
      ret.len_ += sz_info.total + sz_info.size_cnt * 4;
      ret.metainfo_ = 0b10000;
    }
    else {
      ret.len_ += sz_info.total + sz_info.size_cnt * 8;
      ret.metainfo_ = 0b11000;
    }
    // size_type >= 1 , has metainfo
    ret.len_ += sizeof(unsigned char);
  }
  if constexpr (has_type_literal) {
    constexpr auto type_literal = struct_pack::get_type_literal<Args...>();
    // struct_pack::get_type_literal<Args...>().size() crash in clang13. Bug?
    ret.len_ += type_literal.size() + 1;
    ret.metainfo_ |= 0b100;
  }
  if constexpr (has_compatible) {  // calculate bytes count of serialize
                                   // length
    if SP_LIKELY (ret.len_ + 2 < (int64_t{1} << 16)) {
      ret.len_ += 2;
      ret.metainfo_ |= 0b01;
    }
    else if (ret.len_ + 4 < (int64_t{1} << 32)) {
      ret.len_ += 4;
      ret.metainfo_ |= 0b10;
    }
    else {
      ret.len_ += 8;
      ret.metainfo_ |= 0b11;
    }
  }
  return ret;
}

template <
#if __cpp_concepts >= 201907L
    writer_t writer,
#else
    typename writer,
#endif
    typename serialize_type>
class packer {

 public:
  packer(writer &writer_, const serialize_buffer_size &info)
      : writer_(writer_), info(info) {
#if __cpp_concepts < 201907L
    static_assert(writer_t<writer>,
                  "The writer type must satisfy requirements!");
#endif
  }
  packer(const packer &) = delete;
  packer &operator=(const packer &) = delete;

  template <std::size_t size_type, typename T, std::size_t... I,
            typename... Args>
  void serialize_expand_compatible_helper(const T &t, std::index_sequence<I...>,
                                          const Args &...args) {
    using Type = get_args_type<T, Args...>;
    (serialize_many<size_type, compatible_version_number<Type>[I]>(t, args...),
     ...);
  }

  template <uint64_t conf, std::size_t size_type, typename T, typename... Args>
  STRUCT_PACK_INLINE void serialize(const T &t, const Args &...args) {
    serialize_metainfo<conf, size_type == 1, T, Args...>();
    serialize_many<size_type, UINT64_MAX>(t, args...);
    using Type = get_args_type<T, Args...>;
    if constexpr (serialize_static_config<Type>::has_compatible) {
      constexpr std::size_t sz = compatible_version_number<Type>.size();
      return serialize_expand_compatible_helper<size_type, T, Args...>(
          t, std::make_index_sequence<sz>{}, args...);
    }
  }

 private:
  template <typename T, typename... Args>
  static constexpr uint32_t STRUCT_PACK_INLINE calculate_raw_hash() {
    if constexpr (sizeof...(Args) == 0) {
      return get_types_code<remove_cvref_t<T>,
                            remove_cvref_t<decltype(get_types<T>())>>();
    }
    else {
      return get_types_code<
          std::tuple<remove_cvref_t<T>, remove_cvref_t<Args>...>,
          std::tuple<remove_cvref_t<T>, remove_cvref_t<Args>...>>();
    }
  }
  template <uint64_t conf, typename T, typename... Args>
  static constexpr uint32_t STRUCT_PACK_INLINE calculate_hash_head() {
    constexpr uint32_t raw_types_code = calculate_raw_hash<T, Args...>();
    if constexpr (serialize_static_config<serialize_type>::has_compatible ||
                  check_if_add_type_literal<conf, serialize_type>()) {
      return raw_types_code - raw_types_code % 2 + 1;
    }
    else {  // default case, only has hash_code
      return raw_types_code - raw_types_code % 2;
    }
  }
  template <uint64_t conf, bool is_default_size_type, typename T,
            typename... Args>
  constexpr void STRUCT_PACK_INLINE serialize_metainfo() {
    constexpr auto hash_head = calculate_hash_head<conf, T, Args...>() |
                               (is_default_size_type ? 0 : 1);
    writer_.write((char *)&hash_head, sizeof(uint32_t));
    if constexpr (hash_head % 2) {  // has more metainfo
      auto metainfo = info.metainfo();
      writer_.write((char *)&metainfo, sizeof(char));
      if constexpr (serialize_static_config<serialize_type>::has_compatible) {
        constexpr std::size_t sz[] = {0, 2, 4, 8};
        auto len_size = sz[metainfo & 0b11];
        auto len = info.size();
        writer_.write((char *)&len, len_size);
      }
      if constexpr (check_if_add_type_literal<conf, serialize_type>()) {
        constexpr auto type_literal =
            struct_pack::get_type_literal<T, Args...>();
        writer_.write(type_literal.data(), type_literal.size() + 1);
      }
    }
  }

 private:
  template <std::size_t size_type, uint64_t version, typename First,
            typename... Args>
  constexpr void STRUCT_PACK_INLINE serialize_many(const First &first_item,
                                                   const Args &...items) {
    serialize_one<size_type, version>(first_item);
    if constexpr (sizeof...(items) > 0) {
      serialize_many<size_type, version>(items...);
    }
  }
  constexpr void STRUCT_PACK_INLINE write_padding(std::size_t sz) {
    if (sz > 0) {
      constexpr char buf = 0;
      for (std::size_t i = 0; i < sz; ++i) writer_.write(&buf, 1);
    }
  }

  template <std::size_t size_type, uint64_t version, typename T>
  constexpr void inline serialize_one(const T &item) {
    using type = remove_cvref_t<decltype(item)>;
    static_assert(!std::is_pointer_v<type>);
    constexpr auto id = get_type_id<type>();
    if constexpr (is_trivial_view_v<T>) {
      return serialize_one<size_type, version>(item.get());
    }
    else if constexpr (version == UINT64_MAX) {
      if constexpr (id == type_id::compatible_t) {
        // do nothing
      }
      else if constexpr (std::is_same_v<type, std::monostate>) {
        // do nothing
      }
      else if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type> ||
                         id == type_id::int128_t || id == type_id::uint128_t) {
        writer_.write((char *)&item, sizeof(type));
      }
      else if constexpr (unique_ptr<type>) {
        bool has_value = (item != nullptr);
        writer_.write((char *)&has_value, sizeof(char));
        if (has_value) {
          serialize_one<size_type, version>(*item);
        }
      }
      else if constexpr (detail::varint_t<type>) {
        detail::serialize_varint(writer_, item);
      }
      else if constexpr (id == type_id::array_t) {
        if constexpr (is_trivial_serializable<type>::value) {
          writer_.write((char *)&item, sizeof(type));
        }
        else {
          for (const auto &i : item) {
            serialize_one<size_type, version>(i);
          }
        }
      }
      else if constexpr (map_container<type> || container<type>) {
        uint64_t size = item.size();
#ifdef STRUCT_PACK_OPTIMIZE
        writer_.write((char *)&size, size_type);
#else
        if constexpr (size_type == 1) {
          writer_.write((char *)&size, size_type);
        }
        else {
          switch ((info.metainfo() & 0b11000) >> 3) {
            case 1:
              writer_.write((char *)&size, 2);
              break;
            case 2:
              writer_.write((char *)&size, 4);
              break;
            case 3:
              writer_.write((char *)&size, 8);
              break;
            default:
              unreachable();
          }
        }
#endif
        if constexpr (trivially_copyable_container<type>) {
          using value_type = typename type::value_type;
          auto container_size = 1ull * size * sizeof(value_type);
          if SP_UNLIKELY (container_size >= PTRDIFF_MAX)
            unreachable();
          else {
            writer_.write((char *)item.data(), container_size);
          }
          return;
        }
        else {
          for (const auto &i : item) {
            serialize_one<size_type, version>(i);
          }
        }
      }
      else if constexpr (container_adapter<type>) {
        static_assert(!sizeof(type),
                      "the container adapter type is not supported");
      }
      else if constexpr (!pair<type> && tuple<type> &&
                         !is_trivial_tuple<type>) {
        std::apply(
            [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
              serialize_many<size_type, version>(items...);
            },
            item);
      }
      else if constexpr (optional<type>) {
        bool has_value = item.has_value();
        writer_.write((char *)&has_value, sizeof(bool));
        if (has_value) {
          serialize_one<size_type, version>(*item);
        }
      }
      else if constexpr (is_variant_v<type>) {
        static_assert(std::variant_size_v<type> < 256,
                      "variant's size is too large");
        uint8_t index = item.index();
        writer_.write((char *)&index, sizeof(index));
        std::visit(
            [this](auto &&e) {
              this->serialize_one<size_type, version>(e);
            },
            item);
      }
      else if constexpr (expected<type>) {
        bool has_value = item.has_value();
        writer_.write((char *)&has_value, sizeof(has_value));
        if (has_value) {
          if constexpr (!std::is_same_v<typename type::value_type, void>)
            serialize_one<size_type, version>(item.value());
        }
        else {
          serialize_one<size_type, version>(item.error());
        }
      }
      else if constexpr (std::is_class_v<type>) {
        if constexpr (!pair<type> && !is_trivial_tuple<type>)
          if constexpr (!user_defined_refl<type>)
            static_assert(
                std::is_aggregate_v<remove_cvref_t<type>>,
                "struct_pack only support aggregated type, or you should "
                "add macro STRUCT_PACK_REFL(Type,field1,field2...)");
        if constexpr (is_trivial_serializable<type>::value) {
          writer_.write((char *)&item, sizeof(type));
        }
        else if constexpr (is_trivial_serializable<type, true>::value) {
          visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
            int i = 1;
            ((serialize_one<size_type, version>(items),
              write_padding(align::padding_size<type>[i++])),
             ...);
          });
        }
        else {
          visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
            serialize_many<size_type, version>(items...);
          });
        }
      }
      else {
        static_assert(!sizeof(type), "the type is not supported yet");
      }
    }
    else if constexpr (exist_compatible_member<type, version>) {
      if constexpr (id == type_id::compatible_t) {
        if constexpr (version == type::version_number) {
          bool has_value = item.has_value();
          writer_.write((char *)&has_value, sizeof(bool));
          if (has_value) {
            serialize_one<size_type, UINT64_MAX>(*item);
          }
        }
      }
      else if constexpr (unique_ptr<type>) {
        if (item != nullptr) {
          serialize_one<size_type, version>(*item);
        }
      }
      else if constexpr (id == type_id::array_t) {
        for (const auto &i : item) {
          serialize_one<size_type, version>(i);
        }
      }
      else if constexpr (map_container<type> || container<type>) {
        for (const auto &i : item) {
          serialize_one<size_type, version>(i);
        }
      }
      else if constexpr (!pair<type> && tuple<type> &&
                         !is_trivial_tuple<type>) {
        std::apply(
            [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
              serialize_many<size_type, version>(items...);
            },
            item);
      }
      else if constexpr (optional<type>) {
        if (item.has_value()) {
          serialize_one<size_type, version>(*item);
        }
      }
      else if constexpr (is_variant_v<type>) {
        std::visit(
            [this](const auto &e) {
              this->serialize_one<size_type, version>(e);
            },
            item);
      }
      else if constexpr (expected<type>) {
        if (item.has_value()) {
          if constexpr (!std::is_same_v<typename type::value_type, void>)
            serialize_one<size_type, version>(item.value());
        }
        else {
          serialize_one<size_type, version>(item.error());
        }
      }
      else if constexpr (std::is_class_v<type>) {
        visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
          serialize_many<size_type, version>(items...);
        });
      }
    }
    return;
  }

  template <typename T>
  friend constexpr serialize_buffer_size get_needed_size(const T &t);
  writer &writer_;
  const serialize_buffer_size &info;
};  // namespace detail

template <uint64_t conf = type_info_config::automatic,
#if __cpp_concepts >= 201907L
          struct_pack::writer_t Writer,
#else
          typename Writer,
#endif
          typename... Args>
STRUCT_PACK_MAY_INLINE void serialize_to(Writer &writer,
                                         const serialize_buffer_size &info,
                                         const Args &...args) {
#if __cpp_concepts < 201907L
  static_assert(writer_t<Writer>, "The writer type must satisfy requirements!");
#endif
  static_assert(sizeof...(args) > 0);
  detail::packer<Writer, detail::get_args_type<Args...>> o(writer, info);
  switch ((info.metainfo() & 0b11000) >> 3) {
    case 0:
      o.template serialize<conf, 1>(args...);
      break;
#ifdef STRUCT_PACK_OPTIMIZE
    case 1:
      o.template serialize<conf, 2>(args...);
      break;
    case 2:
      o.template serialize<conf, 4>(args...);
      break;
    case 3:
      o.template serialize<conf, 8>(args...);
      break;
#else
    case 1:
    case 2:
    case 3:
      o.template serialize<conf, 2>(args...);
      break;
#endif
    default:
      detail::unreachable();
      break;
  };
}

struct memory_reader {
  const char *now;
  const char *end;
  constexpr memory_reader(const char *beg, const char *end) noexcept
      : now(beg), end(end) {}
  bool read(char *target, size_t len) {
    if SP_UNLIKELY (now + len > end) {
      return false;
    }
    memcpy(target, now, len);
    now += len;
    return true;
  }
  const char *read_view(size_t len) {
    if SP_UNLIKELY (now + len > end) {
      return nullptr;
    }
    auto ret = now;
    now += len;
    return ret;
  }
  bool ignore(size_t len) {
    if SP_UNLIKELY (now + len > end) {
      return false;
    }
    now += len;
    return true;
  }
  std::size_t tellg() { return (std::size_t)now; }
};
#if __cpp_concepts >= 201907L
template <reader_t Reader>
#else
template <typename Reader>
#endif
class unpacker {
 public:
  unpacker() = delete;
  unpacker(const unpacker &) = delete;
  unpacker &operator=(const unpacker &) = delete;

  STRUCT_PACK_INLINE unpacker(Reader &reader) : reader_(reader) {
#if __cpp_concepts < 201907L
    static_assert(reader_t<Reader>,
                  "The writer type must satisfy requirements!");
#endif
  }

  template <typename T, typename... Args>
  STRUCT_PACK_MAY_INLINE struct_pack::errc deserialize(T &t, Args &...args) {
    using Type = get_args_type<T, Args...>;
    constexpr bool has_compatible =
        check_if_compatible_element_exist<decltype(get_types<Type>())>();
    if constexpr (has_compatible) {
      data_len_ = reader_.tellg();
    }
    auto &&[err_code, buffer_len] = deserialize_metainfo<Type>();
    if SP_UNLIKELY (err_code != struct_pack::errc{}) {
      return err_code;
    }
    if constexpr (has_compatible) {
      data_len_ += buffer_len;
    }
    switch (size_type_) {
      case 0:
        err_code = deserialize_many<1, UINT64_MAX, true>(t, args...);
        break;
#ifdef STRUCT_PACK_OPTIMIZE
      case 1:
        err_code = deserialize_many<2, UINT64_MAX, true>(t, args...);
        break;
      case 2:
        err_code = deserialize_many<4, UINT64_MAX, true>(t, args...);
        break;
      case 3:
        err_code = deserialize_many<8, UINT64_MAX, true>(t, args...);
        break;
#else
      case 1:
      case 2:
      case 3:
        err_code = deserialize_many<2, UINT64_MAX, true>(t, args...);
        break;
#endif
      default:
        unreachable();
    }
    if constexpr (has_compatible) {
      if SP_UNLIKELY (err_code != errc::ok) {
        return err_code;
      }
      constexpr std::size_t sz = compatible_version_number<Type>.size();
      err_code = deserialize_compatibles<T, Args...>(
          t, std::make_index_sequence<sz>{}, args...);
    }
    return err_code;
  }

  template <typename T, typename... Args>
  STRUCT_PACK_MAY_INLINE struct_pack::errc deserialize_with_len(
      std::size_t &len, T &t, Args &...args) {
    using Type = get_args_type<T, Args...>;
    constexpr bool has_compatible =
        check_if_compatible_element_exist<decltype(get_types<Type>())>();
    if constexpr (has_compatible) {
      data_len_ = reader_.tellg();
    }
    auto &&[err_code, buffer_len] = deserialize_metainfo<Type>();
    len = buffer_len;
    if SP_UNLIKELY (err_code != struct_pack::errc{}) {
      return err_code;
    }
    if constexpr (has_compatible) {
      data_len_ += buffer_len;
    }
    switch (size_type_) {
      case 0:
        err_code = deserialize_many<1, UINT64_MAX, true>(t, args...);
        break;
#ifdef STRUCT_PACK_OPTIMIZE
      case 1:
        err_code = deserialize_many<2, UINT64_MAX, true>(t, args...);
        break;
      case 2:
        err_code = deserialize_many<4, UINT64_MAX, true>(t, args...);
        break;
      case 3:
        err_code = deserialize_many<8, UINT64_MAX, true>(t, args...);
        break;
#else
      case 1:
      case 2:
      case 3:
        err_code = deserialize_many<2, UINT64_MAX, true>(t, args...);
        break;
#endif
      default:
        unreachable();
    }
    if constexpr (has_compatible) {
      if SP_UNLIKELY (err_code != errc::ok) {
        return err_code;
      }
      constexpr std::size_t sz = compatible_version_number<Type>.size();
      err_code = deserialize_compatibles<T, Args...>(
          t, std::make_index_sequence<sz>{}, args...);
    }
    return err_code;
  }

  template <typename U, size_t I>
  STRUCT_PACK_MAY_INLINE struct_pack::errc get_field(
      std::tuple_element_t<I, decltype(get_types<U>())> &field) {
    using T = remove_cvref_t<U>;
    using Type = get_args_type<T>;

    constexpr bool has_compatible =
        check_if_compatible_element_exist<decltype(get_types<Type>())>();
    if constexpr (has_compatible) {
      data_len_ = reader_.tellg();
    }

    auto &&[err_code, buffer_len] = deserialize_metainfo<T>();
    if SP_UNLIKELY (err_code != struct_pack::errc{}) {
      return err_code;
    }
    if constexpr (has_compatible) {
      data_len_ += buffer_len;
    }
    switch (size_type_) {
      case 0:
        err_code = get_field_impl<1, UINT64_MAX, U, I>(field);
        break;
#ifdef STRUCT_PACK_OPTIMIZE
      case 1:
        err_code = get_field_impl<2, UINT64_MAX, U, I>(field);
        break;
      case 2:
        err_code = get_field_impl<4, UINT64_MAX, U, I>(field);
        break;
      case 3:
        err_code = get_field_impl<8, UINT64_MAX, U, I>(field);
        break;
#else
      case 1:
      case 2:
      case 3:
        err_code = get_field_impl<2, UINT64_MAX, U, I>(field);
        break;
#endif
      default:
        unreachable();
    }
    if constexpr (has_compatible) {
      if SP_UNLIKELY (err_code != errc::ok) {
        return err_code;
      }
      constexpr std::size_t sz = compatible_version_number<Type>.size();
      err_code = deserialize_compatible_fields<U, I>(
          field, std::make_index_sequence<sz>{});
    }
    return err_code;
  }

 private:
  template <typename T, typename... Args, size_t... I>
  STRUCT_PACK_INLINE struct_pack::errc deserialize_compatibles(
      T &t, std::index_sequence<I...>, Args &...args) {
    using Type = get_args_type<T, Args...>;
    struct_pack::errc err_code;
    switch (size_type_) {
      case 0:
        ([&] {
          err_code =
              deserialize_many<1, compatible_version_number<Type>[I], true>(
                  t, args...);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
#ifdef STRUCT_PACK_OPTIMIZE
      case 1:
        ([&] {
          err_code =
              deserialize_many<2, compatible_version_number<Type>[I], true>(
                  t, args...);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
      case 2:
        ([&] {
          err_code =
              deserialize_many<4, compatible_version_number<Type>[I], true>(
                  t, args...);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
      case 3:
        ([&] {
          err_code =
              deserialize_many<8, compatible_version_number<Type>[I], true>(
                  t, args...);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
#else
      case 1:
      case 2:
      case 3:
        ([&] {
          err_code =
              deserialize_many<2, compatible_version_number<Type>[I], true>(
                  t, args...);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
#endif
      default:
        unreachable();
    }
    if (size_type_ ==
        UCHAR_MAX) {  // reuse size_type_ as a tag that the buffer miss some
                      // compatible field, whic is legal.
      err_code = {};
    }
    return err_code;
  }

  template <typename U, size_t I, size_t... Is>
  STRUCT_PACK_INLINE struct_pack::errc deserialize_compatible_fields(
      std::tuple_element_t<I, decltype(get_types<U>())> &field,
      std::index_sequence<Is...>) {
    using T = remove_cvref_t<U>;
    using Type = get_args_type<T>;
    struct_pack::errc err_code;
    switch (size_type_) {
      case 0:
        ([&] {
          err_code =
              get_field_impl<1, compatible_version_number<Type>[Is], U, I>(
                  field);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
#ifdef STRUCT_PACK_OPTIMIZE
      case 1:
        ([&] {
          err_code =
              get_field_impl<2, compatible_version_number<Type>[Is], U, I>(
                  field);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
      case 2:
        ([&] {
          err_code =
              get_field_impl<4, compatible_version_number<Type>[Is], U, I>(
                  field);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
      case 3:
        ([&] {
          err_code =
              get_field_impl<8, compatible_version_number<Type>[Is], U, I>(
                  field);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
#else
      case 1:
      case 2:
      case 3:
        ([&] {
          err_code =
              get_field_impl<2, compatible_version_number<Type>[Is], U, I>(
                  field);
          return err_code == errc::ok;
        }() &&
         ...);
        break;
#endif
      default:
        unreachable();
    }
    if (size_type_ ==
        UCHAR_MAX) {  // reuse size_type_ as a tag that the buffer miss some
                      // compatible field, whic is legal.
      err_code = {};
    }
    return err_code;
  }

  template <std::size_t size_type, uint64_t version, typename U, size_t I>
  STRUCT_PACK_INLINE struct_pack::errc get_field_impl(
      std::tuple_element_t<I, decltype(get_types<U>())> &field) {
    using T = remove_cvref_t<U>;

    T t;
    struct_pack::errc err_code;
    if constexpr (tuple<T>) {
      err_code = std::apply(
          [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
            static_assert(I < sizeof...(items), "out of range");
            return for_each<size_type, version, I>(field, items...);
          },
          t);
    }
    else if constexpr (std::is_class_v<T>) {
      if constexpr (!user_defined_refl<T>)
        static_assert(std::is_aggregate_v<remove_cvref_t<T>>,
                      "struct_pack only support aggregated type, or you should "
                      "add macro STRUCT_PACK_REFL(Type,field1,field2...)");
      err_code = visit_members(t, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
        static_assert(I < sizeof...(items), "out of range");
        return for_each<size_type, version, I>(field, items...);
      });
    }
    else {
      static_assert(!sizeof(T), "illegal type");
    }
    return err_code;
  }

  template <typename size_type, typename version, typename NotSkip>
  struct variant_construct_helper {
    template <size_t index, typename unpack, typename variant_t>
    static STRUCT_PACK_INLINE constexpr void run(unpack &unpacker,
                                                 variant_t &v) {
      if constexpr (index >= std::variant_size_v<variant_t>) {
        return;
      }
      else {
        v = variant_t{std::in_place_index_t<index>{}};
        unpacker.template deserialize_one<size_type::value, version::value,
                                          NotSkip::value>(std::get<index>(v));
      }
    }
  };

  STRUCT_PACK_INLINE std::pair<struct_pack::errc, std::uint64_t>
  deserialize_compatible(unsigned compatible_sz_len) {
    constexpr std::size_t sz[] = {0, 2, 4, 8};
    auto len_sz = sz[compatible_sz_len];
    uint64_t data_len = 0;
    if SP_UNLIKELY (!reader_.read((char *)&data_len, len_sz)) {
      return {errc::no_buffer_space, 0};
    }
    return {errc{}, data_len};
  }

  template <typename T>
  STRUCT_PACK_INLINE struct_pack::errc deserialize_type_literal() {
    constexpr auto literal = struct_pack::get_type_literal<T>();
    if constexpr (view_reader_t<Reader>) {
      const char *buffer = reader_.read_view(literal.size() + 1);
      if SP_UNLIKELY (!buffer) {
        return errc::no_buffer_space;
      }
      if SP_UNLIKELY (memcmp(buffer, literal.data(), literal.size() + 1)) {
        return errc::hash_conflict;
      }
    }
    else {
      char buffer[literal.size() + 1];
      if SP_UNLIKELY (!reader_.read(buffer, literal.size() + 1)) {
        return errc::no_buffer_space;
      }
      if SP_UNLIKELY (memcmp(buffer, literal.data(), literal.size() + 1)) {
        return errc::hash_conflict;
      }
    }

    return errc{};
  }

  template <class T>
  STRUCT_PACK_INLINE std::pair<struct_pack::errc, std::uint64_t>
  deserialize_metainfo() {
    uint32_t current_types_code;
    if SP_UNLIKELY (!reader_.read((char *)&current_types_code,
                                  sizeof(uint32_t))) {
      return {struct_pack::errc::no_buffer_space, 0};
    }
    constexpr uint32_t types_code =
        get_types_code<T, decltype(get_types<T>())>();
    if SP_UNLIKELY ((current_types_code / 2) != (types_code / 2)) {
      return {struct_pack::errc::invalid_buffer, 0};
    }
    if SP_LIKELY (current_types_code % 2 == 0)  // unexist extended metainfo
    {
      size_type_ = 0;
      return {};
    }
    unsigned char metainfo;
    if SP_UNLIKELY (!reader_.read((char *)&metainfo, sizeof(unsigned char))) {
      return {struct_pack::errc::no_buffer_space, 0};
    }
    std::pair<errc, std::uint64_t> ret;
    auto compatible_sz_len = metainfo & 0b11;
    if (compatible_sz_len) {
      ret = deserialize_compatible(compatible_sz_len);
      if SP_UNLIKELY (ret.first != errc{}) {
        return ret;
      }
    }
    auto has_type_literal = metainfo & 0b100;
    if (has_type_literal) {
      auto ec = deserialize_type_literal<T>();
      if SP_UNLIKELY (ec != errc{}) {
        return {ec, 0};
      }
    }
    size_type_ = (metainfo & 0b11000) >> 3;
    return ret;
  }

  template <size_t size_type, uint64_t version, bool NotSkip>
  constexpr struct_pack::errc STRUCT_PACK_INLINE deserialize_many() {
    return {};
  }
  template <size_t size_type, uint64_t version, bool NotSkip, typename First,
            typename... Args>
  constexpr struct_pack::errc STRUCT_PACK_INLINE
  deserialize_many(First &&first_item, Args &&...items) {
    auto code = deserialize_one<size_type, version, NotSkip>(first_item);
    if SP_UNLIKELY (code != struct_pack::errc{}) {
      return code;
    }
    return deserialize_many<size_type, version, NotSkip>(items...);
  }

  constexpr struct_pack::errc STRUCT_PACK_INLINE
  ignore_padding(std::size_t sz) {
    if (sz > 0) {
      return reader_.ignore(sz) ? errc{} : errc::no_buffer_space;
    }
    else {
      return errc{};
    }
  }

  template <size_t size_type, uint64_t version, bool NotSkip, typename T>
  constexpr struct_pack::errc inline deserialize_one(T &item) {
    struct_pack::errc code{};
    using type = remove_cvref_t<decltype(item)>;
    static_assert(!std::is_pointer_v<type>);
    constexpr auto id = get_type_id<type>();
    if constexpr (is_trivial_view_v<type>) {
      static_assert(view_reader_t<Reader>,
                    "The Reader isn't a view_reader, can't deserialize "
                    "a trivial_view<T>");
      const char *view = reader_.read_view(sizeof(typename T::value_type));
      if SP_LIKELY (view != nullptr) {
        item = *reinterpret_cast<const typename T::value_type *>(view);
        code = errc::ok;
      }
      else {
        code = errc::no_buffer_space;
      }
    }
    else if constexpr (version == UINT64_MAX) {
      if constexpr (id == type_id::compatible_t) {
        // do nothing
      }
      else if constexpr (std::is_same_v<type, std::monostate>) {
        // do nothing
      }
      else if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type> ||
                         id == type_id::int128_t || id == type_id::uint128_t) {
        if constexpr (NotSkip) {
          if SP_UNLIKELY (!reader_.read((char *)&item, sizeof(type))) {
            return struct_pack::errc::no_buffer_space;
          }
        }
        else {
          return reader_.ignore(sizeof(type)) ? errc{} : errc::no_buffer_space;
        }
      }
      else if constexpr (unique_ptr<type>) {
        bool has_value{};
        if SP_UNLIKELY (!reader_.read((char *)&has_value, sizeof(bool))) {
          return struct_pack::errc::no_buffer_space;
        }
        if (!has_value) {
          return {};
        }
        item = std::make_unique<typename T::element_type>();
        deserialize_one<size_type, version, NotSkip>(*item);
      }
      else if constexpr (detail::varint_t<type>) {
        code = detail::deserialize_varint<NotSkip>(reader_, item);
      }
      else if constexpr (id == type_id::array_t) {
        if constexpr (is_trivial_serializable<type>::value) {
          if constexpr (NotSkip) {
            if SP_UNLIKELY (!reader_.read((char *)&item, sizeof(type))) {
              return struct_pack::errc::no_buffer_space;
            }
          }
          else {
            return reader_.ignore(sizeof(type)) ? errc{}
                                                : errc::no_buffer_space;
          }
        }
        else {
          for (auto &i : item) {
            code = deserialize_one<size_type, version, NotSkip>(i);
            if SP_UNLIKELY (code != struct_pack::errc{}) {
              return code;
            }
          }
        }
      }
      else if constexpr (container<type>) {
        size_t size = 0;
#ifdef STRUCT_PACK_OPTIMIZE
        if SP_UNLIKELY (!reader_.read((char *)&size, size_type)) {
          return struct_pack::errc::no_buffer_space;
        }
#else
        if constexpr (size_type == 1) {
          if SP_UNLIKELY (!reader_.read((char *)&size, size_type)) {
            return struct_pack::errc::no_buffer_space;
          }
        }
        else {
          switch (size_type_) {
            case 1:
              if SP_UNLIKELY (!reader_.read((char *)&size, 2)) {
                return struct_pack::errc::no_buffer_space;
              }
              break;
            case 2:
              if SP_UNLIKELY (!reader_.read((char *)&size, 4)) {
                return struct_pack::errc::no_buffer_space;
              }
              break;
            case 3:
              if SP_UNLIKELY (!reader_.read((char *)&size, 8)) {
                return struct_pack::errc::no_buffer_space;
              }
              break;
            default:
              unreachable();
          }
        }
#endif
        if SP_UNLIKELY (size == 0) {
          return {};
        }
        if constexpr (map_container<type>) {
          std::pair<typename type::key_type, typename type::mapped_type>
              value{};
          if constexpr (is_trivial_serializable<decltype(value)>::value &&
                        !NotSkip) {
            return reader_.ignore(size * sizeof(value)) ? errc{}
                                                        : errc::no_buffer_space;
          }
          else {
            for (size_t i = 0; i < size; ++i) {
              code = deserialize_one<size_type, version, NotSkip>(value);
              if SP_UNLIKELY (code != struct_pack::errc{}) {
                return code;
              }
              if constexpr (NotSkip) {
                item.emplace(std::move(value));
                // TODO: mapped_type can deserialize without be moved
              }
            }
          }
        }
        else if constexpr (set_container<type>) {
          typename type::value_type value{};
          if constexpr (is_trivial_serializable<decltype(value)>::value &&
                        !NotSkip) {
            return reader_.ignore(size * sizeof(value)) ? errc{}
                                                        : errc::no_buffer_space;
          }
          else {
            for (size_t i = 0; i < size; ++i) {
              code = deserialize_one<size_type, version, NotSkip>(value);
              if SP_UNLIKELY (code != struct_pack::errc{}) {
                return code;
              }
              if constexpr (NotSkip) {
                item.emplace(std::move(value));
                // TODO: mapped_type can deserialize without be moved
              }
            }
          }
        }
        else {
          using value_type = typename type::value_type;
          if constexpr (trivially_copyable_container<type>) {
            size_t mem_sz = size * sizeof(value_type);
            if constexpr (NotSkip) {
              if constexpr (string_view<type> || dynamic_span<type>) {
                static_assert(
                    view_reader_t<Reader>,
                    "The Reader isn't a view_reader, can't deserialize "
                    "a string_view/span");
                const char *view = reader_.read_view(mem_sz);
                if SP_UNLIKELY (view == nullptr) {
                  return struct_pack::errc::no_buffer_space;
                }
                item = {(value_type *)(view), size};
              }
              else {
                if SP_UNLIKELY (mem_sz >= PTRDIFF_MAX)
                  unreachable();
                else {
                  item.resize(size);
                  if SP_UNLIKELY (!reader_.read((char *)item.data(), mem_sz)) {
                    return struct_pack::errc::no_buffer_space;
                  }
                }
              }
            }
            else {
              return reader_.ignore(mem_sz) ? errc{} : errc::no_buffer_space;
            }
          }
          else {
            if constexpr (NotSkip) {
              if constexpr (dynamic_span<type>) {
                static_assert(!dynamic_span<type>,
                              "It's illegal to deserialize a span<T> which T "
                              "is a non-trival-serializable type.");
              }
              else {
                item.resize(size);
                for (auto &i : item) {
                  code = deserialize_one<size_type, version, NotSkip>(i);
                  if SP_UNLIKELY (code != struct_pack::errc{}) {
                    return code;
                  }
                }
              }
            }
            else {
              value_type useless;
              for (size_t i = 0; i < size; ++i) {
                code = deserialize_one<size_type, version, NotSkip>(useless);
                if SP_UNLIKELY (code != struct_pack::errc{}) {
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
      else if constexpr (!pair<type> && tuple<type> &&
                         !is_trivial_tuple<type>) {
        std::apply(
            [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
              code = deserialize_many<size_type, version, NotSkip>(items...);
            },
            item);
      }
      else if constexpr (optional<type> || expected<type>) {
        bool has_value{};
        if SP_UNLIKELY (!reader_.read((char *)&has_value, sizeof(bool))) {
          return struct_pack::errc::no_buffer_space;
        }
        if SP_UNLIKELY (!has_value) {
          if constexpr (expected<type>) {
            item = typename type::unexpected_type{typename type::error_type{}};
            deserialize_one<size_type, version, NotSkip>(item.error());
          }
          else {
            return {};
          }
        }
        else {
          if constexpr (expected<type>) {
            if constexpr (!std::is_same_v<typename type::value_type, void>)
              deserialize_one<size_type, version, NotSkip>(item.value());
          }
          else {
            item = type{std::in_place_t{}};
            deserialize_one<size_type, version, NotSkip>(*item);
          }
        }
      }
      else if constexpr (is_variant_v<type>) {
        uint8_t index{};
        if SP_UNLIKELY (!reader_.read((char *)&index, sizeof(index))) {
          return struct_pack::errc::no_buffer_space;
        }
        if SP_UNLIKELY (index >= std::variant_size_v<type>) {
          return struct_pack::errc::invalid_buffer;
        }
        else {
          template_switch<variant_construct_helper<
              std::integral_constant<std::size_t, size_type>,
              std::integral_constant<std::uint64_t, version>,
              std::integral_constant<bool, NotSkip>>>(index, *this, item);
        }
      }
      else if constexpr (std::is_class_v<type>) {
        if constexpr (!pair<type> && !is_trivial_tuple<type>)
          if constexpr (!user_defined_refl<type>)
            static_assert(
                std::is_aggregate_v<remove_cvref_t<type>>,
                "struct_pack only support aggregated type, or you should "
                "add macro STRUCT_PACK_REFL(Type,field1,field2...)");
        if constexpr (is_trivial_serializable<type>::value) {
          if constexpr (NotSkip) {
            if SP_UNLIKELY (!reader_.read((char *)&item, sizeof(type))) {
              return struct_pack::errc::no_buffer_space;
            }
          }
          else {
            return reader_.ignore(sizeof(type)) ? errc{}
                                                : errc::no_buffer_space;
          }
        }
        else if constexpr (is_trivial_serializable<type, true>::value) {
          visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
            int i = 1;
            auto f = [&](auto &&item) {
              code = deserialize_one<size_type, version, NotSkip>(item);
              if SP_LIKELY (code == errc::ok) {
                code = ignore_padding(align::padding_size<type>[i++]);
              }
              return code == errc::ok;
            };
            [[maybe_unused]] bool op = (f(items) && ...);
          });
        }
        else {
          visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
            code = deserialize_many<size_type, version, NotSkip>(items...);
          });
        }
      }
      else {
        static_assert(!sizeof(type), "the type is not supported yet");
      }
    }
    else if constexpr (exist_compatible_member<type, version>) {
      if constexpr (id == type_id::compatible_t) {
        if constexpr (version == type::version_number) {
          auto pos = reader_.tellg();
          if ((std::size_t)pos >= data_len_) {
            if (std::is_unsigned_v<decltype(pos)> || pos >= 0) {
              size_type_ = UCHAR_MAX;  // Just notice that this is not a real
                                       // error, this is a flag for exit.
            }
            return struct_pack::errc::no_buffer_space;
          }
          bool has_value{};
          if SP_UNLIKELY (!reader_.read((char *)&has_value, sizeof(bool))) {
            return struct_pack::errc::no_buffer_space;
          }
          if (!has_value) {
            return code;
          }
          else {
            item = type{std::in_place_t{}};
            deserialize_one<size_type, UINT64_MAX, NotSkip>(*item);
          }
        }
      }
      else if constexpr (unique_ptr<type>) {
        if (item == nullptr) {
          return {};
        }
        deserialize_one<size_type, version, NotSkip>(*item);
      }
      else if constexpr (id == type_id::array_t) {
        for (auto &i : item) {
          code = deserialize_one<size_type, version, NotSkip>(i);
          if SP_UNLIKELY (code != struct_pack::errc{}) {
            return code;
          }
        }
      }
      else if constexpr (container<type>) {
        if constexpr (id == type_id::set_container_t) {
          // TODO: support it.
          static_assert(!sizeof(type),
                        "we don't support compatible field in set now.");
        }
        else if constexpr (id == type_id::map_container_t) {
          static_assert(
              !exist_compatible_member<typename type::key_type>,
              "we don't support compatible field in map's key_type now.");
          if constexpr (NotSkip) {
            for (auto &e : item) {
              code = deserialize_one<size_type, version, NotSkip>(e.second);
              if SP_UNLIKELY (code != struct_pack::errc{}) {
                return code;
              }
            }
          }
          else {
            // TODO: support it.
            static_assert(
                !sizeof(type),
                "we don't support skip compatible field in container now.");
          }
          // how to deserialize it quickly?
        }
        else {
          if constexpr (NotSkip) {
            for (auto &i : item) {
              code = deserialize_one<size_type, version, NotSkip>(i);
              if SP_UNLIKELY (code != struct_pack::errc{}) {
                return code;
              }
            }
          }
          else {
            // TODO: support it.
            static_assert(
                !sizeof(type),
                "we don't support skip compatible field in container now.");
          }
        }
      }
      else if constexpr (!pair<type> && tuple<type> &&
                         !is_trivial_tuple<type>) {
        std::apply(
            [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
              code = deserialize_many<size_type, version, NotSkip>(items...);
            },
            item);
      }
      else if constexpr (optional<type> || expected<type>) {
        bool has_value = item.has_value();
        if (!has_value) {
          if constexpr (expected<type>) {
            deserialize_one<size_type, version, NotSkip>(item.error());
          }
        }
        else {
          if constexpr (expected<type>) {
            if constexpr (!std::is_same_v<typename type::value_type, void>)
              deserialize_one<size_type, version, NotSkip>(item.value());
          }
          else {
            deserialize_one<size_type, version, NotSkip>(item.value());
          }
        }
      }
      else if constexpr (is_variant_v<type>) {
        std::visit(
            [this](auto &item) {
              deserialize_one<size_type, version, NotSkip>(item);
            },
            item);
      }
      else if constexpr (std::is_class_v<type>) {
        visit_members(item, [&](auto &&...items) CONSTEXPR_INLINE_LAMBDA {
          code = deserialize_many<size_type, version, NotSkip>(items...);
        });
      }
    }
    return code;
  }

  // partial deserialize_to
  template <size_t size_type, uint64_t version, size_t I, size_t FieldIndex,
            typename FieldType, typename T>
  STRUCT_PACK_INLINE constexpr bool set_value(struct_pack::errc &err_code,
                                              FieldType &field, T &&t) {
    if constexpr (FieldIndex == I) {
      static_assert(
          std::is_same_v<remove_cvref_t<FieldType>, remove_cvref_t<T>>);
      err_code = deserialize_one<size_type, version, true>(field);
      return /*don't skip=*/true;
    }
    else {
      err_code = deserialize_one<size_type, version, false>(t);
      return /*skip=*/false;
    }
  }

  template <int I, class... Ts>
  decltype(auto) get_nth(Ts &&...ts) {
    return std::get<I>(std::forward_as_tuple(ts...));
  }

  template <size_t size_type, uint64_t version, size_t FieldIndex,
            typename FieldType, typename... Args, std::size_t... I>
  STRUCT_PACK_INLINE constexpr void for_each_helper(struct_pack::errc &code,
                                                    FieldType &field,
                                                    std::index_sequence<I...>,
                                                    Args &&...items) {
    [[maybe_unused]] auto result =
        (!set_value<size_type, version, I, FieldIndex>(code, field,
                                                       get_nth<I>(items...)) &&
         ...);
  }

  template <size_t size_type, uint64_t version, size_t FieldIndex,
            typename FieldType, typename... Args>
  STRUCT_PACK_INLINE constexpr struct_pack::errc for_each(FieldType &field,
                                                          Args &&...items) {
    struct_pack::errc code{};
    for_each_helper<size_type, version, FieldIndex, FieldType>(
        code, field, std::make_index_sequence<sizeof...(Args)>(), items...);
    return code;
  }

 public:
  std::size_t data_len_;

 private:
  Reader &reader_;
  unsigned char size_type_;
};

}  // namespace detail
}  // namespace struct_pack
