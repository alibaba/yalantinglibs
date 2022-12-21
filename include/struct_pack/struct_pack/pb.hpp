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
#include <frozen/map.h>
#include <frozen/set.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <numeric>
#include <ostream>

#include "struct_pack/struct_pack.hpp"
#include "struct_pack/struct_pack/reflection.h"
#include "struct_pack/struct_pack/struct_pack_impl.hpp"
namespace struct_pack::pb {
template <typename T>
class varint {
 public:
  using value_type = T;
  varint() = default;
  varint(T t) : val(t) {}
  [[nodiscard]] operator T() const { return val; }
  auto& operator=(T t) {
    val = t;
    return *this;
  }
  [[nodiscard]] auto operator<=>(const varint&) const = default;
  [[nodiscard]] bool operator==(const varint<T>& t) const {
    return val == t.val;
  }
  [[nodiscard]] bool operator==(T t) const { return val == t; }
  [[nodiscard]] auto operator&(uint8_t mask) const {
    T new_val = val & mask;
    return varint(new_val);
  }
  template <std::unsigned_integral U>
  [[nodiscard]] auto operator<<(U shift) const {
    T new_val = val << shift;
    return varint(new_val);
  }
  template <typename U>
  [[nodiscard]] auto operator|=(U shift) {
    if constexpr (std::same_as<U, varint<T>>) {
      val |= shift.val;
    }
    else {
      val |= shift;
    }
    return *this;
  }
  friend std::ostream& operator<<(std::ostream& os, const varint& varint) {
    os << varint.val;
    return os;
  }

 private:
  T val;
};

template <typename T>
class sint {
 public:
  using value_type = T;
  sint() = default;
  sint(T t) : val(t) {}
  [[nodiscard]] operator T() const { return val; }
  auto& operator=(T t) {
    val = t;
    return *this;
  }
  [[nodiscard]] auto operator<=>(const sint<T>&) const = default;
  [[nodiscard]] bool operator==(T t) const { return val == t; }
  [[nodiscard]] bool operator==(const sint& t) const { return val == t.val; }
  [[nodiscard]] auto operator&(uint8_t mask) const {
    T new_val = val & mask;
    return sint(new_val);
  }
  template <std::unsigned_integral U>
  auto operator<<(U shift) const {
    T new_val = val << shift;
    return sint(new_val);
  }
  friend std::ostream& operator<<(std::ostream& os, const sint& t) {
    os << t.val;
    return os;
  }

 private:
  T val;
};

using varint32_t = varint<int32_t>;
using varuint32_t = varint<uint32_t>;
using varint64_t = varint<int64_t>;
using varuint64_t = varint<uint64_t>;

using sint32_t = sint<int32_t>;
using sint64_t = sint<int64_t>;

using fixed32_t = uint32_t;
using fixed64_t = uint64_t;
using sfixed32_t = int32_t;
using sfixed64_t = int64_t;

// clang-format off
template <typename T>
concept varintable_t =
       std::is_same_v<T, varint32_t>
    || std::is_same_v<T, varint64_t>
    || std::is_same_v<T, varuint32_t>
    || std::is_same_v<T, varuint64_t>
;
template <typename T>
concept sintable_t =
       std::is_same_v<T, sint32_t>
    || std::is_same_v<T, sint64_t>
;
// clang-format on

template <typename T>
consteval auto get_field_varint_type() {
  if constexpr (detail::optional<T>) {
    return get_field_varint_type<typename T::value_type>();
  }
  else if constexpr (std::same_as<T, bool> || std::is_enum_v<T>) {
    return uint64_t{};
  }
  else if constexpr (varintable_t<T> || sintable_t<T>) {
    return typename T::value_type{};
  }
  else {
    static_assert(!sizeof(T), "error field");
  }
}

template <typename T>
using field_varint_t = decltype(get_field_varint_type<T>());

// clang-format off
template <typename T>
concept VARINT =
       std::same_as<T, int8_t>
    || std::same_as<T, uint8_t>
    || std::same_as<T, int16_t>
    || std::same_as<T, uint16_t>
    || std::same_as<T, varint32_t>
    || std::same_as<T, varuint32_t>
    || std::same_as<T, varint64_t>
    || std::same_as<T, varuint64_t>
    || std::same_as<T, sint32_t>
    || std::same_as<T, sint64_t>
    || std::same_as<T, bool>
    || std::is_enum_v<T>
;
template <typename T>
concept I64 =
       std::same_as<T, int64_t>
    || std::same_as<T, uint64_t>
    || std::same_as<T, double>
;
template <typename T>
concept LEN =
       std::same_as<T, std::string>
    || std::is_class_v<T>
    || detail::container<T>;
template <typename T>
concept I32 =
       std::same_as<T, int32_t>
    || std::same_as<T, uint32_t>
    || std::same_as<T, float>
;
// There are six wire types: VARINT, I64, LEN, SGROUP, EGROUP, and I32
// tag = (field_number << 3) | wire_type
enum class wire_type_t : uint8_t {
  varint = 0, // int32, int64, uint32, uint64, sint32, sint64, bool, enum
  i64 = 1,    // fixed64, sfixed64, double
  len = 2,    // string, bytes, embedded messages, packed repeated fields
  sgroup = 3, // group start (deprecated)
  egroup = 4, // group end (deprecated)
  i32 = 5     // fixed32, sfixed32, float
};
template <typename T>
consteval wire_type_t get_wire_type() {
  if constexpr (detail::optional<T>) {
    return get_wire_type<typename std::remove_cvref_t<T>::value_type>();
  }
  else if constexpr (VARINT<T>) {
    return wire_type_t::varint;
  }
  else if constexpr (I64<T>) {
    return wire_type_t::i64;
  }
  else if constexpr (LEN<T>) {
    return wire_type_t::len;
  }
  else if constexpr (I32<T>) {
    return wire_type_t::i32;
  }
  else {
    static_assert(!sizeof(T), "SGROUP and EGROUP are deprecated");
  }
}
// clang-format on

//#include "pb_get_field_from_ifelse.hpp"
#include "pb_get_field_from_tuple.hpp"

template <std::size_t FieldIndex, std::size_t FieldNumber, typename T,
          typename Field>
[[nodiscard]] STRUCT_PACK_INLINE std::size_t calculate_one_size(
    const T& t, const Field& field);

template <typename T>
consteval auto get_field_number_to_index_map();

template <typename T, std::size_t Size, std::size_t... I>
[[nodiscard]] STRUCT_PACK_INLINE std::size_t get_needed_size_impl(
    const T& t, std::index_sequence<I...>) {
  constexpr auto Map = get_field_number_to_index_map<T>();
  constexpr auto i2n_map = Map.second;
  std::array<std::size_t, Size> size_array{
      calculate_one_size<I, i2n_map.at(I)>(t, get_field<T, I>(t))...};
  return std::accumulate(size_array.begin(), size_array.end(), 0);
}
template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE std::size_t get_needed_size(const T& t) {
  static_assert(std::is_class_v<T>);
  constexpr auto Count = detail::member_count<T>();
  return get_needed_size_impl<T, Count>(t, std::make_index_sequence<Count>());
}
template <typename U, typename T, unsigned Shift>
[[nodiscard]] STRUCT_PACK_INLINE U encode_zigzag(T t) {
  return (static_cast<U>(t) << 1U) ^
         static_cast<U>(-static_cast<T>(static_cast<U>(t) >> Shift));
}
template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE auto encode_zigzag(T t) {
  if constexpr (std::is_same_v<T, int32_t>) {
    return encode_zigzag<uint32_t, int32_t, 31U>(t);
  }
  else if constexpr (std::is_same_v<T, int64_t>) {
    return encode_zigzag<uint64_t, int64_t, 63U>(t);
  }
  else {
    static_assert(!sizeof(T), "error zigzag type");
  }
}
template <typename T, typename U>
[[nodiscard]] STRUCT_PACK_INLINE T decode_zigzag(U u) {
  return static_cast<T>((u >> 1U)) ^ static_cast<U>(-static_cast<T>(u & 1U));
}
template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE T decode_zigzag(T t) {
  if constexpr (std::is_same_v<T, int32_t>) {
    return decode_zigzag<int32_t, uint32_t>(t);
  }
  else if constexpr (std::is_same_v<T, int64_t>) {
    return decode_zigzag<int64_t, uint64_t>(t);
  }
  else {
    static_assert(!sizeof(T), "error type of zigzag");
  }
}
template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE constexpr std::size_t calculate_varint_size(
    T t) {
  if constexpr (std::unsigned_integral<T>) {
    std::size_t ret = 0;
    do {
      ret++;
      t >>= 7;
    } while (t != 0);
    return ret;
  }
  else if constexpr (varintable_t<T>) {
    using value_type = typename T::value_type;
    uint64_t v = value_type(t);
    return calculate_varint_size(v);
  }
  else if constexpr (sintable_t<T>) {
    using value_type = typename T::value_type;
    auto v = encode_zigzag(value_type(t));
    return calculate_varint_size(v);
  }
  else {
    static_assert(!sizeof(T), "error type");
  }
}
template <std::size_t FieldNumber, wire_type_t wire_type>
consteval std::size_t calculate_tag_size() {
  auto tag = (FieldNumber << 3U) | uint8_t(wire_type);
  return calculate_varint_size(tag);
}

template <typename T, std::size_t FieldIndex>
struct oneof_field_number_array {
  using T_Field =
      std::tuple_element_t<FieldIndex,
                           decltype(detail::get_types(std::declval<T>()))>;
  static_assert(detail::variant<T_Field>);
  using array_type = std::array<std::size_t, std::variant_size_v<T_Field>>;
};
template <typename T, std::size_t FieldIndex>
using oneof_field_number_array_t =
    typename oneof_field_number_array<T, FieldIndex>::array_type;
template <typename T, std::size_t FieldIndex>
constexpr oneof_field_number_array_t<T, FieldIndex> oneof_field_number_seq{};

template <std::size_t FieldIndex>
struct oneof_size_helper_with_field_index {
  template <std::size_t VariantIndex, typename T, typename OneofField>
  [[nodiscard]] STRUCT_PACK_INLINE std::size_t run(const T& t,
                                                   const OneofField& field) {
    constexpr auto seq = struct_pack::pb::oneof_field_number_seq<T, FieldIndex>;
    constexpr auto FieldNumber = std::get<VariantIndex>(seq);
    return calculate_one_size<FieldIndex, FieldNumber>(t, field);
  }
};

template <std::size_t VariantIndex>
struct oneof_size_helper {
  template <typename Helper, typename T, typename Variant>
  [[nodiscard]] STRUCT_PACK_INLINE static std::size_t run(Helper& u, const T& t,
                                                          const Variant& v) {
    if constexpr (VariantIndex < std::variant_size_v<Variant>) {
      assert(VariantIndex == v.index());
      return u.template run<VariantIndex>(t, std::get<VariantIndex>(v));
    }
    else {
      assert(!sizeof(Variant) && "error path");
      // never reached
      return 0;
    }
  }
};
template <std::size_t FieldIndex, typename T, detail::variant Variant>
[[nodiscard]] STRUCT_PACK_INLINE std::size_t calculate_oneof_size(
    const T& t, const Variant& v) {
  oneof_size_helper_with_field_index<FieldIndex> helper;
  return detail::template_switch<oneof_size_helper>(v.index(), helper, t, v);
}
template <std::size_t FieldIndex, std::size_t FieldNumber, typename T,
          typename Field>
[[nodiscard]] STRUCT_PACK_INLINE std::size_t calculate_one_size(
    const T& t, const Field& field) {
  if constexpr (detail::variant<Field>) {
    return calculate_oneof_size<FieldIndex>(t, field);
  }
  else {
    constexpr auto wire_type = get_wire_type<Field>();
    constexpr auto tag_size = calculate_tag_size<FieldNumber, wire_type>();
    if constexpr (detail::optional<Field>) {
      if (field.has_value()) {
        return calculate_one_size<FieldIndex, FieldNumber>(t, field.value());
      }
      else {
        return 0;
      }
    }
    else if constexpr (VARINT<Field>) {
      if constexpr (std::is_enum_v<Field>) {
        auto v = static_cast<uint64_t>(field);
        if (v == 0) {
          return 0;
        }
        return tag_size + calculate_varint_size(v);
      }
      else {
        if (field == Field{}) {
          return 0;
        }
        return tag_size + calculate_varint_size(field);
      }
    }
    else if constexpr (LEN<Field>) {
      if constexpr (std::same_as<Field, std::string>) {
        if (field.empty()) {
          return 0;
        }
        return tag_size + calculate_varint_size(field.size()) + field.size();
      }
      else if constexpr (detail::container<Field>) {
        if (field.empty()) {
          return 0;
        }
        using value_type = typename Field::value_type;
        if constexpr (VARINT<value_type>) {
          std::size_t sz = 0;
          for (auto&& i : field) {
            sz += calculate_varint_size(i);
          }
          return tag_size + calculate_varint_size(field.size()) + sz;
        }
        else if constexpr (I32<value_type> || I64<value_type>) {
          auto total = field.size() * sizeof(value_type);
          return tag_size + calculate_varint_size(total) + total;
        }
        else {
          if constexpr (detail::map_container<Field>) {
            using key_type = typename Field::key_type;
            using mapped_type = typename Field::mapped_type;
            static_assert(
                std::same_as<key_type, std::string> || std::integral<key_type>,
                "the key_type must be integral or string type");
            static_assert(
                !detail::map_container<mapped_type>,
                "the mapped_type can be any type except another map.");
          }
          std::size_t total = 0;
          for (auto&& e : field) {
            auto size = get_needed_size(e);
            total += tag_size + calculate_varint_size(size) + size;
          }
          return total;
        }
      }
      else if constexpr (std::is_class_v<Field>) {
        auto size = get_needed_size(field);
        return tag_size + calculate_varint_size(size) + size;
      }
      else {
        static_assert(!sizeof(Field), "ERROR type");
        return 0;
      }
    }
    else if constexpr (I64<Field>) {
      static_assert(sizeof(Field) == 8);
      if (field == 0) {
        return 0;
      }
      return tag_size + sizeof(Field);
    }
    else if constexpr (I32<Field>) {
      static_assert(sizeof(Field) == 4);
      if (field == 0) {
        return 0;
      }
      return tag_size + sizeof(Field);
    }
    else {
      static_assert(!sizeof(Field), "ERROR type");
      return 0;
    }
  }
}
template <typename T>
constexpr std::size_t first_field_number = 1;

template <typename T>
struct field_number_array {
  using array_type = std::array<std::size_t, detail::member_count<T>()>;
};
template <typename T>
using field_number_array_t = typename field_number_array<T>::array_type;
template <typename T>
constexpr field_number_array_t<T> field_number_seq{};
template <typename... Args>
consteval bool all_field_number_greater_than_zero(Args... args) {
  return (... && args);
}

template <typename... Args>
consteval bool field_number_seq_not_init(Args... args) {
  return (... && (args == 0));
}

template <typename T>
consteval bool has_duplicate_element() {
  constexpr auto seq = field_number_seq<T>;
  // TODO: a better way to check duplicate
  for (int i = 0; i < seq.size(); ++i) {
    for (int j = 0; j < i; ++j) {
      if (seq[i] == seq[j]) {
        return true;
      }
    }
  }
  return false;
}
template <typename T>
consteval bool check_oneof();
template <typename T>
consteval std::size_t field_number_count();

template <typename T, std::size_t FieldIndex, std::size_t I>
consteval void fill_ni_array_oneof_impl_sub(std::size_t& index, auto& n_array,
                                            auto& i_array) {
  constexpr auto seq = struct_pack::pb::oneof_field_number_seq<T, FieldIndex>;
  n_array[index] = seq[I];
  i_array[index] = FieldIndex;
  index++;
}

template <typename T, std::size_t FieldIndex, detail::variant Variant,
          std::size_t... I>
consteval void fill_ni_array_oneof_impl(std::size_t& index, auto& n_array,
                                        auto& i_array,
                                        std::index_sequence<I...>) {
  // constexpr auto VariantCount = std::variant_size_v<Variant>;
  (fill_ni_array_oneof_impl_sub<T, FieldIndex, I>(index, n_array, i_array),
   ...);
}
template <typename T, std::size_t FieldIndex, detail::variant Variant>
consteval void fill_ni_array_oneof(std::size_t& index, auto& n_array,
                                   auto& i_array) {
  constexpr auto VariantCount = std::variant_size_v<Variant>;
  fill_ni_array_oneof_impl<T, FieldIndex, Variant>(
      index, n_array, i_array, std::make_index_sequence<VariantCount>());
}

template <typename T, std::size_t I>
consteval void fill_ni_array(std::size_t& index, auto& n_array, auto& i_array) {
  using Tuple = decltype(detail::get_types(std::declval<T>()));
  using Field = std::tuple_element_t<I, Tuple>;
  if constexpr (detail::variant<Field>) {
    fill_ni_array_oneof<T, I, Field>(index, n_array, i_array);
  }
  else {
    n_array[index] = first_field_number<T> + I;
    i_array[index] = I;
    index++;
  }
}

template <typename T, std::size_t FieldNumberCount, std::size_t... I>
consteval auto build_n2i_with_oneof_impl(std::index_sequence<I...>) {
  std::array<std::size_t, FieldNumberCount> n_array;
  std::array<std::size_t, FieldNumberCount> i_array;
  std::size_t index = 0;
  (fill_ni_array<T, I>(index, n_array, i_array), ...);
  return std::make_pair(n_array, i_array);
}

template <typename T, std::size_t FieldNumberCount>
consteval auto build_n2i_with_oneof() {
  // has oneof
  constexpr auto Count = detail::member_count<T>();
  return build_n2i_with_oneof_impl<T, FieldNumberCount>(
      std::make_index_sequence<Count>());
}
template <typename T, std::size_t FieldNumberCount, std::size_t... I>
consteval auto build_n2i_map_with_oneof(std::index_sequence<I...>) {
  constexpr auto ni_array = build_n2i_with_oneof<T, FieldNumberCount>();
  return frozen::map<std::size_t, std::size_t, FieldNumberCount>{
      {ni_array.first[I], ni_array.second[I]}...};
}
template <typename T, std::size_t Size, std::size_t... I>
consteval auto get_field_number_to_index_map_impl(std::index_sequence<I...>) {
  constexpr auto seq = field_number_seq<T>;
  static_assert(check_oneof<T>(), "please check field number in oneof");
  if constexpr (field_number_seq_not_init(std::get<I>(seq)...)) {
    constexpr auto FieldNumberCount = field_number_count<T>();
    if constexpr (FieldNumberCount == Size) {
      frozen::map<std::size_t, std::size_t, Size> n2i{
          {first_field_number<T> + I, I}...};
      frozen::map<std::size_t, std::size_t, Size> i2n{
          {I, first_field_number<T> + I}...};
      return std::make_pair(n2i, i2n);
    }
    else {
      constexpr auto n2i = build_n2i_map_with_oneof<T, FieldNumberCount>(
          std::make_index_sequence<FieldNumberCount>());
      frozen::map<std::size_t, std::size_t, Size> i2n{
          {I, first_field_number<T> + I}...};
      return std::make_pair(n2i, i2n);
    }
  }
  else {
    static_assert(all_field_number_greater_than_zero(std::get<I>(seq)...),
                  "all field number must be specified");
    static_assert(!has_duplicate_element<T>());
    static_assert(first_field_number<T> == 1,
                  "field_number_seq and first_field_number cannot be specified "
                  "at the same time");
    auto field_number_set = frozen::make_set(seq);
    static_assert(field_number_set.size() == Size);
    frozen::map<std::size_t, std::size_t, Size> n2i{{std::get<I>(seq), I}...};
    frozen::map<std::size_t, std::size_t, Size> i2n{{I, std::get<I>(seq)}...};
    return std::make_pair(n2i, i2n);
  }
}
template <typename T>
consteval auto get_field_number_to_index_map() {
  constexpr auto Count = detail::member_count<T>();
  return get_field_number_to_index_map_impl<T, Count>(
      std::make_index_sequence<Count>());
}
template <typename T>
consteval auto get_field_n2i_map() {
  constexpr auto Map = get_field_number_to_index_map<T>();
  return Map.first;
}
template <typename T>
consteval auto get_field_i2n_map() {
  constexpr auto Map = get_field_number_to_index_map<T>();
  return Map.second;
}

template <typename T, std::size_t Size, std::size_t... I>
consteval auto get_sorted_field_number_array_impl(std::index_sequence<I...>) {
  constexpr auto n2i_map = get_field_n2i_map<T>();
  std::array<std::size_t, Size> array;  //{i2n_map.at(I)...};
  std::size_t i = 0;
  for (auto [k, v] : n2i_map) {
    array[i++] = k;
  }
  std::sort(array.begin(), array.end());
  return array;
}

template <typename T>
consteval auto get_sorted_field_number_array() {
  constexpr auto Count = field_number_count<T>();
  return get_sorted_field_number_array_impl<T, Count>(
      std::make_index_sequence<Count>());
}

template <typename T, std::size_t FieldIndex, typename U>
consteval bool check_oneof_field_number() {
  if constexpr (detail::variant<U>) {
    constexpr auto array = oneof_field_number_seq<T, FieldIndex>;
    for (auto i : array) {
      if (i == 0) {
        return false;
      }
    }
    for (int i = 0; i < array.size(); ++i) {
      for (int j = 0; j < i; ++j) {
        if (array[i] == array[j]) {
          return false;
        }
      }
    }
    return true;
  }
  else {
    return true;
  }
}

template <typename T, detail::tuple Tuple, std::size_t... I>
consteval bool check_oneof_impl(std::index_sequence<I...>) {
  return (... &&
          check_oneof_field_number<T, I, std::tuple_element_t<I, Tuple>>());
}

template <typename T>
consteval bool check_oneof() {
  constexpr auto Count = detail::member_count<T>();
  using Tuple = decltype(detail::get_types(std::declval<T>()));
  return check_oneof_impl<T, Tuple>(std::make_index_sequence<Count>());
}

template <std::size_t i2n_Size, std::size_t n2i_Size>
struct FieldMap {
  std::array<std::pair<std::size_t, std::size_t>, i2n_Size> i2n;
  std::array<std::pair<std::size_t, std::size_t>, n2i_Size> n2i;
};

template <typename T, typename Field>
consteval std::size_t one_field_number_count() {
  if constexpr (detail::variant<Field>) {
    return std::variant_size_v<Field>;
  }
  else {
    return 1;
  }
}

template <typename T, std::size_t... I>
consteval std::size_t field_number_count_impl(std::index_sequence<I...>) {
  using Tuple = decltype(detail::get_types(std::declval<T>()));
  return (0 + ... +
          one_field_number_count<T, std::tuple_element_t<I, Tuple>>());
}
template <typename T>
consteval std::size_t field_number_count() {
  constexpr auto Count = detail::member_count<T>();
  //  if constexpr (Count == 1) {
  //    using Tuple = decltype(detail::get_types(std::declval<T>()));
  //    return one_field_number_count<T, std::tuple_element_t<0, Tuple>>();
  //  }
  //  else {
  return field_number_count_impl<T>(std::make_index_sequence<Count>());
  //  }
}
template <typename T>
consteval auto rebuild_n2i() {}

template <typename T, std::size_t Size, std::size_t... I>
consteval auto create_map_impl(std::index_sequence<I...>) {
  constexpr auto i2n_map = get_field_i2n_map<T>();
  constexpr auto n2i_map = get_field_n2i_map<T>();
  constexpr auto n_array = get_sorted_field_number_array<T>();
  constexpr auto n2i_Size = field_number_count<T>();
  FieldMap<Size, n2i_Size> m{
      .i2n = {std::make_pair(I, i2n_map.at(I))...},
      //      .n2i = {std::make_pair(std::get<I>(n_array),
      //                             n2i_map.at(std::get<I>(n_array)))...}
  };
  static_assert(n2i_map.size() == n_array.size());
  for (int i = 0; i < n_array.size(); i++) {
    auto n = n_array[i];
    m.n2i[i] = std::make_pair(n, n2i_map.at(n));
  }
  return m;
}

template <typename T>
consteval auto create_map() {
  constexpr auto Count = detail::member_count<T>();
  return create_map_impl<T, Count>(std::make_index_sequence<Count>());
}

template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE constexpr std::size_t get_field_index(
    std::size_t FieldNumber) {
  constexpr auto Count = detail::member_count<T>();
  constexpr auto map = create_map<T>();
  // TODO: optimize with binary search
  for (int i = 0; i < map.n2i.size(); ++i) {
    if (FieldNumber == map.n2i[i].first) {
      return map.n2i[i].second;
    }
  }
  return Count;
}
template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE constexpr std::size_t get_field_number(
    std::size_t FieldIndex) {
  constexpr auto Count = detail::member_count<T>();
  constexpr auto map = create_map<T>();
  for (int i = 0; i < map.i2n.size(); ++i) {
    if (FieldIndex == map.i2n[i].first) {
      return map.i2n[i].second;
    }
  }
  return 0;
}
template <typename T, std::size_t FieldIndex, std::size_t Size,
          std::size_t... I>
[[nodiscard]] STRUCT_PACK_INLINE consteval auto get_oneof_n2i_map_impl(
    std::index_sequence<I...>) {
  constexpr auto seq = struct_pack::pb::oneof_field_number_seq<T, FieldIndex>;
  frozen::map<std::size_t, std::size_t, Size> map{{seq[I], I}...};
  return map;
}
template <typename T, std::size_t FieldIndex>
[[nodiscard]] STRUCT_PACK_INLINE consteval auto get_oneof_n2i_map() {
  using Variant =
      std::tuple_element_t<FieldIndex,
                           decltype(detail::get_types(std::declval<T>()))>;
  static_assert(detail::variant<Variant>);
  constexpr auto VariantCount = std::variant_size_v<Variant>;
  return get_oneof_n2i_map_impl<T, FieldIndex, VariantCount>(
      std::make_index_sequence<VariantCount>());
}

template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE consteval auto get_max_field_number() {
  auto array = get_sorted_field_number_array<T>();
  static_assert(!array.empty());
  return array.back();
}

template <detail::struct_pack_byte Byte>
class packer {
 public:
  packer(Byte* data, std::size_t max) : data_(data), max_(max) {}
  template <typename T>
  STRUCT_PACK_INLINE void serialize(const T& t) {
    constexpr auto Count = detail::member_count<T>();
    serialize(t, std::make_index_sequence<Count>());
  }

 private:
  template <typename T, std::size_t... I>
  STRUCT_PACK_INLINE void serialize(const T& t, std::index_sequence<I...>) {
    constexpr auto FieldArray = get_sorted_field_number_array<T>();
    constexpr auto n2i_map = get_field_n2i_map<T>();
    (serialize<n2i_map.at(std::get<I>(FieldArray)), std::get<I>(FieldArray)>(
         t, get_field<T, n2i_map.at(std::get<I>(FieldArray))>(t)),
     ...);
  }
  template <std::size_t FieldIndex, std::size_t FieldNumber, typename T,
            typename Field>
  STRUCT_PACK_INLINE void serialize(const T& t, const Field& field) {
    if constexpr (detail::optional<Field>) {
      if (field.has_value()) {
        serialize<FieldIndex, FieldNumber>(t, field.value());
      }
      else {
        return;
      }
    }
    else if constexpr (detail::variant<Field>) {
      serialize_oneof<FieldIndex>(t, field);
      return;
    }
    else {
      constexpr auto wire_type = get_wire_type<Field>();
      if constexpr (VARINT<Field>) {
        if constexpr (std::is_enum_v<Field>) {
          auto v = static_cast<uint64_t>(field);
          if (v == 0) {
            return;
          }
          assert(pos_ < max_);
          // why '0'
          data_[pos_++] = '0';
          serialize_varint(v);
        }
        else {
          if (field == Field{}) {
            return;
          }
          write_tag<FieldNumber, wire_type>();
          serialize_varint(field);
        }
      }
      else if constexpr (I64<Field> || I32<Field>) {
        if (field == 0) {
          return;
        }
        write_tag<FieldNumber, wire_type>();
        std::memcpy(data_ + pos_, &field, sizeof(Field));
        pos_ += sizeof(Field);
      }
      else if constexpr (LEN<Field>) {
        if constexpr (std::same_as<Field, std::string>) {
          if (field.empty()) {
            return;
          }
          write_tag<FieldNumber, wire_type>();
          serialize_varint(field.size());
          assert(pos_ + field.size() <= max_);
          std::memcpy(data_ + pos_, field.data(), field.size());
          pos_ += field.size();
        }
        else if constexpr (detail::map_container<Field> ||
                           detail::container<Field>) {
          if (field.empty()) {
            return;
          }
          using value_type = typename Field::value_type;
          if constexpr (VARINT<value_type>) {
            write_tag<FieldNumber, wire_type>();
            auto sz_pos = pos_;
            // risk to data len > 1byte
            pos_++;
            std::size_t sz = 0;
            for (auto&& e : field) {
              sz += calculate_varint_size(e);
              serialize_varint(e);
            }
            pos_ = sz_pos;
            auto new_pos = pos_;
            serialize_varint(sz);
            pos_ = new_pos;
          }
          else if constexpr (I64<value_type> || I32<value_type>) {
            write_tag<FieldNumber, wire_type>();
            auto size = field.size() * sizeof(value_type);
            serialize_varint(size);
            assert(pos_ + size <= max_);
            std::memcpy(data_ + pos_, field.data(), size);
            pos_ += size;
          }
          else {
            for (auto&& e : field) {
              write_tag<FieldNumber, wire_type>();
              std::size_t sz = 0;
              if constexpr (I32<value_type> || I64<value_type>) {
                sz = calculate_one_size(e);
              }
              else {
                sz = get_needed_size(e);
              }
              serialize_varint(sz);
              serialize(e);
            }
          }
        }
        else {
          static_assert(std::is_class_v<Field>);
          write_tag<FieldNumber, wire_type>();
          auto sz = get_needed_size(field);
          serialize_varint(sz);
          serialize(field);
        }
      }
      else {
        static_assert(!sizeof(Field), "SGROUP and EGROUP are deprecated");
      }
    }
  }
  template <std::size_t FieldIndex, typename T, detail::variant Variant>
  STRUCT_PACK_INLINE void serialize_oneof(const T& t, const Variant& v) {
    oneof_packer_helper_with_field_index<FieldIndex> helper;
    detail::template_switch<oneof_packer_helper>(v.index(), helper, *this, t,
                                                 v);
  }
  template <std::size_t FieldIndex>
  struct oneof_packer_helper_with_field_index {
    template <std::size_t VariantIndex, typename Packer, typename T,
              typename OneofField>
    STRUCT_PACK_INLINE void run(Packer& self, const T& t,
                                const OneofField& field) {
      constexpr auto seq =
          struct_pack::pb::oneof_field_number_seq<T, FieldIndex>;
      constexpr auto FieldNumber = std::get<VariantIndex>(seq);
      return self.template serialize<FieldIndex, FieldNumber>(t, field);
    }
  };
  template <std::size_t VariantIndex>
  struct oneof_packer_helper {
    template <typename Helper, typename Packer, typename T, typename Variant>
    STRUCT_PACK_INLINE static void run(Helper& u, Packer& self, const T& t,
                                       const Variant& v) {
      if constexpr (VariantIndex < std::variant_size_v<Variant>) {
        assert(VariantIndex == v.index());
        return u.template run<VariantIndex>(self, t, std::get<VariantIndex>(v));
      }
      else {
        assert(!sizeof(Variant) && "error path");
        // never reached
        return;
      }
    }
  };
  template <typename T>
  STRUCT_PACK_INLINE void encode_varint_v1(T t) {
    uint64_t v = t;
    do {
      assert(pos_ < max_);
      data_[pos_++] = 0b1000'0000 | uint8_t(v);
      v >>= 7;
    } while (v != 0);
    assert(pos_ > 0);
    data_[pos_ - 1] = uint8_t(data_[pos_ - 1]) & 0b0111'1111;
  }
  // from google/protobuf/io/coded_stream.h EpsCopyOutputStream::UnsafeVarint
  template <std::unsigned_integral T>
  STRUCT_PACK_INLINE void encode_varint_v2(T t) {
    while (t >= 0x80) {
      assert(pos_ < max_);
      data_[pos_++] = static_cast<uint8_t>(t | 0x80);
      t >>= 7;
    }
    data_[pos_++] = static_cast<uint8_t>(t);
  }
  template <typename T>
  STRUCT_PACK_INLINE void serialize_varint(T t) {
    if constexpr (varintable_t<T>) {
      // using value_type = std::make_unsigned_t<typename T::value_type>;
      serialize_varint(uint64_t(t));
    }
    else if constexpr (sintable_t<T>) {
      using value_type = typename T::value_type;
      auto v = encode_zigzag(value_type(t));
      serialize_varint(v);
    }
    else {
      encode_varint_v2(t);
    }
  }
  template <std::size_t field_number, wire_type_t wire_type>
  STRUCT_PACK_INLINE void write_tag() {
    constexpr auto tag = (field_number << 3) | uint8_t(wire_type);
    // https://developers.google.com/protocol-buffers/docs/proto3#assigning_field_numbers
    // The smallest field number you can specify is 1,
    // and the largest is 2^29 - 1, or 536,870,911.
    // You also cannot use the numbers 19000 through 19999
    // (FieldDescriptor::kFirstReservedNumber through
    // FieldDescriptor::kLastReservedNumber),
    // as they are reserved for the Protocol Buffers implementation
    assert((tag > 0 && tag < 19000) ||
           (tag > 19999 && tag <= ((1U << 29U) - 1)));
    serialize_varint(tag);
  }

 private:
  Byte* data_;
  std::size_t pos_ = 0;
  std::size_t max_;
};

template <detail::struct_pack_byte Byte>
class unpacker {
 public:
  unpacker(const Byte* data, std::size_t size) : data_(data), size_(size) {}
  template <typename T>
  [[nodiscard]] STRUCT_PACK_INLINE constexpr std::errc deserialize(T& t) {
    if (size_ == 0) [[unlikely]] {
      return std::errc{};
    }
    while (pos_ < size_) [[likely]] {
        auto ec = deserialize_one(t);
        if (ec != std::errc{}) {
          return ec;
        }
      }
    return std::errc{};
  }
  [[nodiscard]] STRUCT_PACK_INLINE std::size_t consume_len() const {
    return pos_;
  }

 private:
  template <typename T>
  [[nodiscard]] STRUCT_PACK_INLINE constexpr std::errc deserialize_one(T& t) {
    assert(pos_ < size_);
    uint32_t tag{};
    auto ec = read_tag(t, tag);
    if (ec != std::errc{}) [[unlikely]] {
      return ec;
    }
    std::size_t field_number = tag >> 3;
    auto wire_type = static_cast<wire_type_t>(tag & 0b0000'0111);
    if constexpr (get_max_field_number<T>() < 256) {
      return detail::template_switch<unpacker_helper_with_field_number>(
          field_number, *this, t, wire_type);
    }
    else {
      if (field_number < 256) {
        return detail::template_switch<unpacker_helper_with_field_number>(
            field_number, *this, t, wire_type);
      }
      else {
        constexpr auto n2i_map = get_field_n2i_map<T>();
        auto it = n2i_map.find(field_number);
        if (it == n2i_map.end()) [[unlikely]] {
          return std::errc::invalid_argument;
        }
        else {
          return detail::template_switch<unpacker_helper>(
              it.second, *this, t, wire_type, field_number);
        }
      }
    }
  }

  template <std::size_t FieldIndex, typename T, typename field_number_t>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_one(
      T& t, wire_type_t wire_type, field_number_t field_number) {
    static_assert(std::same_as<field_number_t, std::size_t>);
    constexpr auto Count = detail::member_count<T>();
    if constexpr (FieldIndex < Count) {
      using Tuple = decltype(detail::get_types(std::declval<T>()));
      using FieldType = std::tuple_element_t<FieldIndex, Tuple>;
      if constexpr (detail::variant<FieldType>) {
        // field_number -> variant index
        constexpr auto oneof_n2i_map = get_oneof_n2i_map<T, FieldIndex>();
        auto it = oneof_n2i_map.find(field_number);
        if (it == oneof_n2i_map.end()) {
          diagnose<T>(FieldIndex, field_number);
          assert(false && "not support now");
          return std::errc::invalid_argument;
        }
        else {
          auto&& f = get_field<T, FieldIndex>(t);
          oneof_unpacker_helper_with_field_index<FieldIndex> helper;
          return detail::template_switch<oneof_unpacker_helper>(
              field_number, *this, t, f, wire_type, helper);
        }
      }
      else {
        constexpr auto field_wire_type = get_wire_type<FieldType>();
        if (field_wire_type != wire_type) {
          return std::errc::invalid_argument;
        }
        return deserialize_one<T, FieldIndex, field_wire_type>(t);
      }
    }
    else {
      diagnose<T>(FieldIndex);
      assert(false && "not support now");
      return std::errc::invalid_argument;
    }
  }

  template <typename T, std::size_t FieldIndex, wire_type_t WireType>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_one(T& t) {
    static_assert(!std::is_const_v<T>);
    constexpr auto Count = detail::member_count<T>();
    static_assert(FieldIndex < Count);
    auto&& f = get_field<T, FieldIndex>(t);
    return deserialize_one_impl<T, WireType>(t, f);
  }
  template <typename T, wire_type_t WireType>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_one_impl(T& t,
                                                                  auto& f) {
    static_assert(!std::is_const_v<std::remove_reference_t<decltype(f)>>);
    if constexpr (WireType == wire_type_t::varint) {
      using field_type = std::remove_reference_t<decltype(f)>;
      using value_type = field_varint_t<field_type>;
      value_type v = 0;
      auto ec = deserialize_varint(t, v);
      if constexpr (sintable_t<field_type>) {
        v = decode_zigzag(v);
      }
      if (ec == std::errc{}) [[likely]] {
        if constexpr (detail::optional<field_type>) {
          using optional_value_type = typename field_type::value_type;
          if constexpr (std::is_enum_v<optional_value_type>) {
            f = static_cast<optional_value_type>(v);
          }
          else {
            f = v;
          }
        }
        else if constexpr (std::is_enum_v<field_type>) {
          f = static_cast<field_type>(v);
        }
        else {
          f = v;
        }
      }
      return ec;
    }
    else if constexpr (WireType == wire_type_t::len) {
      return deserialize_len(t, f);
    }
    else if constexpr (WireType == wire_type_t::i32 ||
                       WireType == wire_type_t::i64) {
      return deserialize_fixedint(t, f);
    }
    else {
      return std::errc::invalid_argument;
    }
  }
  [[nodiscard]] STRUCT_PACK_INLINE std::errc decode_varint_v2(uint64_t& v) {
    if ((static_cast<uint64_t>(data_[pos_]) & 0x80U) == 0) {
      v = static_cast<uint64_t>(data_[pos_]);
      pos_++;
      return std::errc{};
    }
    constexpr const int8_t max_varint_length = sizeof(uint64_t) * 8 / 7 + 1;
    uint64_t val = 0;
    if (size_ - pos_ >= max_varint_length) [[likely]] {
      do {
        // clang-format off
        int64_t b = data_[pos_++];
                           val  = ((uint64_t(b) & 0x7fU)       ); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) <<  7U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 14U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 21U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 28U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 35U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 42U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 49U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 56U); if (b >= 0) { break; }
        b = data_[pos_++]; val |= ((uint64_t(b) & 0x01U) << 63U); if (b >= 0) { break; }
        // clang-format on
        return std::errc::argument_out_of_domain;
      } while (false);
    }
    else {
      unsigned int shift = 0;
      while (pos_ != size_ && int64_t(data_[pos_]) < 0) {
        val |= (uint64_t(data_[pos_++]) & 0x7fU) << shift;
        shift += 7;
      }
      if (pos_ == size_) {
        return std::errc::no_buffer_space;
      }
      val |= uint64_t(data_[pos_++]) << shift;
    }
    v = val;
    return std::errc{};
  }
  template <typename Field>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc decode_varint_v1(Field& f) {
    uint64_t n = 0;
    std::size_t i = 0;
    bool finished = false;
    while (pos_ < size_) {
      if ((uint8_t(data_[pos_]) >> 7) == 1) {
        n |= static_cast<uint64_t>(data_[pos_] & 0b0111'1111) << 7 * i;
        pos_++;
        i++;
      }
      else {
        finished = true;
        break;
      }
    }
    if (finished) {
      n |= static_cast<Field>(data_[pos_] & 0b0111'1111) << 7 * i;
      pos_++;
      f = n;
      return std::errc{};
    }
    else {
      if (pos_ >= size_) {
        return std::errc::no_buffer_space;
      }
      return std::errc::invalid_argument;
    }
  }
  template <typename T, typename Field>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_varint(T& t,
                                                                Field& f) {
    if constexpr (detail::optional<Field>) {
      return deserialize_varint(t, f.value());
    }
    else {
      uint64_t v;
      auto ec = decode_varint_v2(v);
      if (ec == std::errc{}) [[likely]] {
        f = v;
      }
      return ec;
      // Variable-width integers, or varints,
      // are at the core of the wire format.
      // They allow encoding unsigned 64-bit integers using anywhere
      // between one and ten bytes, with small values using fewer bytes.
      // return decode_varint_v1(f);
    }
  }

  template <typename T, typename Field>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_fixedint(T& t,
                                                                  Field& f) {
    if constexpr (detail::optional<Field>) {
      return deserialize_fixedint(t, f.value());
    }
    else {
      if (pos_ + sizeof(Field) > size_) [[unlikely]] {
        return std::errc::no_buffer_space;
      }
      assert(pos_ + sizeof(Field) <= size_);
      std::memcpy(&f, data_ + pos_, sizeof(Field));
      pos_ += sizeof(Field);
      return std::errc{};
    }
  }

  template <typename T, typename Field>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_len(T& t, Field& f) {
    if constexpr (detail::optional<Field>) {
      using value_type = typename Field::value_type;
      if (!f.has_value()) {
        f = value_type{};
      }
      auto ec = deserialize_len(t, f.value());
      if (ec != std::errc{}) [[unlikely]] {
        return ec;
      }
      return ec;
    }
    else if constexpr (std::same_as<Field, std::string>) {
      uint64_t sz = 0;
      auto ec = deserialize_varint(t, sz);
      if (ec != std::errc{}) [[unlikely]] {
        return ec;
      }
      f.resize(sz);
      std::memcpy(f.data(), data_ + pos_, sz);
      pos_ += sz;
      return std::errc{};
    }
    else if constexpr (detail::container<Field>) {
      uint64_t sz = 0;
      auto ec = deserialize_varint(t, sz);
      if (ec != std::errc{}) [[unlikely]] {
        return ec;
      }
      using value_type = typename Field::value_type;
      if constexpr (VARINT<value_type>) {
        auto max_pos = pos_ + sz;
        while (pos_ < max_pos) [[likely]] {
            uint64_t val = 0;
            ec = deserialize_varint(t, val);
            if (ec != std::errc{}) [[unlikely]] {
              return ec;
            }
            f.push_back(val);
          }
        return ec;
      }
      else if constexpr (I64<value_type> || I32<value_type>) {
        auto element_size = sz / sizeof(value_type);
        if (element_size * sizeof(value_type) != sz) [[unlikely]] {
          return std::errc::invalid_argument;
        }
        if (pos_ + sz > size_) [[unlikely]] {
          return std::errc::no_buffer_space;
        }
        f.resize(element_size);
        std::memcpy(&f[0], data_ + pos_, sz);
        pos_ += sz;
        return std::errc{};
      }
      else {
        if constexpr (detail::map_container<Field>) {
          using key_type = typename Field::key_type;
          using mapped_type = typename Field::mapped_type;
          key_type key{};
          uint32_t tag{};
          ec = read_tag(t, tag);
          if (ec != std::errc{}) [[unlikely]] {
            return std::errc::invalid_argument;
          }
          constexpr auto key_wire_type = get_wire_type<key_type>();
          constexpr auto key_tag = (1 << 3) | uint8_t(key_wire_type);
          if (tag != key_tag) [[unlikely]] {
            return std::errc::invalid_argument;
          }
          ec = deserialize_one_impl<T, key_wire_type>(t, key);
          if (ec != std::errc{}) [[unlikely]] {
            return std::errc::invalid_argument;
          }
          auto it = f.emplace(std::move(key), mapped_type{});
          ec = read_tag(t, tag);
          if (ec != std::errc{}) [[unlikely]] {
            return std::errc::invalid_argument;
          }
          constexpr auto mapped_wire_type = get_wire_type<mapped_type>();
          constexpr auto mapped_tag = (2 << 3) | uint8_t(mapped_wire_type);
          if (tag != mapped_tag) [[unlikely]] {
            return std::errc::invalid_argument;
          }
          ec = deserialize_one_impl<T, mapped_wire_type>(t, it.first->second);
          if (ec != std::errc{}) [[unlikely]] {
            return std::errc::invalid_argument;
          }
        }
        else {
          //          auto pos_bak = pos_;
          //          std::size_t container_size = 0;
          //          std::size_t message_size = sz;
          //          while (true) {
          //            pos_ += message_size;
          //            container_size++;
          //            uint32_t new_tag{};
          //            auto e = read_tag(t, new_tag);
          //            if (e != std::errc{} || new_tag != cur_tag_) {
          //              break ;
          //            }
          //            e = deserialize_varint(t, message_size);
          //            if (e != std::errc{}) {
          //              break ;
          //            }
          //          }
          //          f.reserve(f.size() + container_size);
          //          pos_ = pos_bak;
          // huge performance effect
          // if (f.empty()) {
          //  f.reserve(32);
          // }
          unpacker o(data_ + pos_, sz);
          ec = o.deserialize(f.emplace_back());
          if (ec != std::errc{}) [[unlikely]] {
            return ec;
          }
          pos_ += sz;
        }
        return ec;
      }
    }
    else {
      uint64_t sz = 0;
      auto ec = deserialize_varint(t, sz);
      if (ec != std::errc{}) [[unlikely]] {
        return ec;
      }
      unpacker o(data_ + pos_, sz);
      ec = o.deserialize(f);
      if (ec != std::errc{}) [[unlikely]] {
        return ec;
      }
      pos_ += sz;
      return ec;
    }
  }

  template <typename T, std::size_t FieldIndex, std::size_t VariantIndex,
            detail::variant Variant>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_oneof(T& t,
                                                               Variant& v) {
    constexpr auto VariantCount = std::variant_size_v<Variant>;
    if constexpr (VariantIndex < VariantCount) {
      using field_type =
          std::remove_cvref_t<decltype(std::get<VariantIndex>(v))>;
      field_type tmp{};
      constexpr auto wire_type = get_wire_type<field_type>();
      auto ec = deserialize_one_impl<T, wire_type>(t, tmp);
      if (ec != std::errc{}) [[unlikely]] {
        return ec;
      }
      v.template emplace<VariantIndex>(tmp);
      return ec;
    }
    else {
      // can not reach
      assert(false && "not support now");
      return std::errc::invalid_argument;
    }
  }
  template <typename T>
  [[nodiscard]] STRUCT_PACK_INLINE std::errc read_tag(T& t, uint32_t& tag) {
    return deserialize_varint(t, tag);
  }

  template <typename T>
  [[maybe_unused]] void diagnose(
      std::optional<std::size_t> field_index = std::nullopt,
      std::optional<std::size_t> field_number = std::nullopt) {
    constexpr auto Count = detail::member_count<T>();
    constexpr auto i2n_map = get_field_i2n_map<T>();
    std::cout << "message field number: ";
    for (int i = 0; i < Count; i++) {
      std::cout << i2n_map.at(i) << " ";
    }
    std::cout << std::endl;
    if (field_number) {
      std::cout << "current field number: " << *field_number << std::endl;
    }
    if (field_index) {
      std::cout << "current field index: " << *field_index << std::endl;
    }
    std::cout << "member count: " << Count << std::endl;
  }

  template <std::size_t FieldNumber>
  struct unpacker_helper_with_field_number {
    template <typename Unpacker, typename T, typename WireType>
    [[nodiscard]] STRUCT_PACK_INLINE static constexpr std::errc run(
        Unpacker& o, T& t, WireType wire_type) {
      static_assert(std::same_as<WireType, wire_type_t>);
      constexpr auto n2i_map = get_field_n2i_map<T>();
      if constexpr (FieldNumber == 256) {
        return std::errc::invalid_argument;
      }
      else {
        if constexpr (n2i_map.find(FieldNumber) != n2i_map.end()) {
          constexpr auto FieldIndex = n2i_map.at(FieldNumber);
          using Tuple = decltype(detail::get_types(std::declval<T>()));
          using FieldType = std::tuple_element_t<FieldIndex, Tuple>;
          if constexpr (detail::variant<FieldType>) {
            // field_number -> variant index
            constexpr auto oneof_n2i_map = get_oneof_n2i_map<T, FieldIndex>();
            if constexpr (oneof_n2i_map.find(FieldNumber) !=
                          oneof_n2i_map.end()) {
              constexpr auto VariantIndex = oneof_n2i_map.at(FieldNumber);
              return o.template deserialize_oneof<T, FieldIndex, VariantIndex>(
                  t, get_field<T, FieldIndex>(t));
            }
            else {
              return std::errc::invalid_argument;
            }
          }
          else {
            constexpr auto field_wire_type = get_wire_type<FieldType>();
            if (field_wire_type != wire_type) [[unlikely]] {
              return std::errc::invalid_argument;
            }
            return o.template deserialize_one<T, FieldIndex, field_wire_type>(
                t);
          }
        }
        else {
          return std::errc::invalid_argument;
        }
      }
    }
  };

  template <std::size_t FieldIndex>
  struct unpacker_helper {
    template <typename Unpacker, typename T, typename WireType,
              typename field_number_t>
    [[nodiscard]] STRUCT_PACK_INLINE static std::errc run(
        Unpacker& o, T& t, WireType wire_type, field_number_t field_number) {
      static_assert(std::same_as<WireType, wire_type_t>);
      return o.template deserialize_one<FieldIndex>(t, wire_type, field_number);
    }
  };
  template <std::size_t FieldIndex>
  struct oneof_unpacker_helper_with_field_index {
    static constexpr std::size_t field_index = FieldIndex;
  };
  template <std::size_t VariantIndex, typename Unpacker, typename T,
            detail::variant Variant, typename Helper>
  struct oneof_unpacker_helper_with_variant_index {
    [[nodiscard]] STRUCT_PACK_INLINE static std::errc run(Unpacker& o, T& t,
                                                          Variant& v,
                                                          Helper h) {
      return o.template deserialize_oneof<T, Helper::field_index, VariantIndex>(
          t, v);
    }
  };

  template <std::size_t FieldNumber>
  struct oneof_unpacker_helper {
    template <typename Unpacker, typename T, detail::variant Variant,
              typename WireType, typename Helper>
    [[nodiscard]] STRUCT_PACK_INLINE static std::errc run(Unpacker& o, T& t,
                                                          Variant& v,
                                                          WireType wire_type,
                                                          Helper h) {
      static_assert(std::same_as<WireType, wire_type_t>);
      constexpr auto oneof_n2i_map =
          get_oneof_n2i_map<T, Helper::field_index>();
      auto it = oneof_n2i_map.find(FieldNumber);
      if (it == oneof_n2i_map.end()) {
        assert(false && "not support now");
        return std::errc::invalid_argument;
      }
      else {
        auto variant_index = oneof_n2i_map.at(FieldNumber);
        return detail::template_switch<
            oneof_unpacker_helper_with_variant_index>(variant_index, o, t, v,
                                                      h);
      }
    }
  };

 private:
  const Byte* data_;
  std::size_t size_;
  std::size_t pos_ = 0;
  // uint32_t cur_tag_{};
};

template <detail::struct_pack_buffer Buffer = std::vector<char>,
          typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE Buffer serialize(const Args&... args) {
  static_assert(sizeof...(args) == 1);
  Buffer buffer;
  auto size = get_needed_size(args...);
  buffer.resize(size);
  packer o(buffer.data(), size);
  o.serialize(args...);
  return buffer;
}
template <detail::struct_pack_buffer Buffer, typename... Args>
void STRUCT_PACK_INLINE serialize_to(Buffer& buffer, const Args&... args) {
  static_assert(sizeof...(args) == 1);
  auto data_offset = buffer.size();
  auto need_size = get_needed_size(args...);
  auto total = data_offset + need_size;
  buffer.resize(total);
  packer o(buffer.data(), need_size);
  o.serialize(args...);
}
template <typename T, detail::struct_pack_byte Byte>
auto STRUCT_PACK_INLINE deserialize(const Byte* data, std::size_t size,
                                    std::size_t& consume_len) {
  expected<T, std::errc> ret;
  unpacker o(data, size);
  auto ec = o.deserialize(ret.value());
  if (ec != std::errc{}) {
    ret = struct_pack::unexpected<std::errc>{ec};
  }
  else {
    consume_len = o.consume_len();
  }
  return ret;
}
template <typename T, detail::struct_pack_byte Byte>
auto STRUCT_PACK_INLINE deserialize_to(T& t, const Byte* data, std::size_t size,
                                       std::size_t& consume_len) {
  unpacker o(data, size);
  auto ec = o.deserialize(t);
  consume_len = o.consume_len();
  return ec;
}
}  // namespace struct_pack::pb