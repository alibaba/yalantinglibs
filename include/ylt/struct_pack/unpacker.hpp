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

#include "alignment.hpp"
#include "calculate_size.hpp"
#include "derived_helper.hpp"
#include "endian_wrapper.hpp"
#include "error_code.hpp"
#include "md5_constexpr.hpp"
#include "packer.hpp"
#include "reflection.hpp"
#include "trivial_view.hpp"
#include "type_calculate.hpp"
#include "type_id.hpp"
#include "type_trait.hpp"
#include "varint.hpp"

namespace struct_pack {

namespace detail {

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
template <reader_t Reader, uint64_t conf = sp_config::DEFAULT>
#else
template <typename Reader, uint64_t conf = sp_config::DEFAULT>
#endif
class unpacker {
 public:
  unpacker() = delete;
  unpacker(const unpacker &) = delete;
  unpacker &operator=(const unpacker &) = delete;

  template <typename DerivedClasses, typename size_type, typename version,
            typename NotSkip>
  friend struct deserialize_one_derived_class_helper;

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
        unreachable();
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
    uint64_t data_len64;
    uint32_t data_len32;
    uint16_t data_len16;
    bool result;
    switch (compatible_sz_len) {
      case 1:
        if SP_LIKELY (read_wrapper<2>(reader_, (char *)&data_len16)) {
          return {errc{}, data_len16};
        }
        break;
      case 2:
        if SP_LIKELY (read_wrapper<4>(reader_, (char *)&data_len32)) {
          return {errc{}, data_len32};
        }
        break;
      case 3:
        if SP_LIKELY (read_wrapper<8>(reader_, (char *)&data_len64)) {
          return {errc{}, data_len64};
        }
        break;
      default:
        unreachable();
    }
    return {errc::no_buffer_space, 0};
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
      if SP_UNLIKELY (!read_bytes_array(reader_, buffer, literal.size() + 1)) {
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
    if constexpr (check_if_disable_hash_head<conf, T>()) {
      if constexpr (is_MD5_reader_wrapper<Reader>) {
        static_assert(!sizeof(T),
                      "it's illegal if you want to deserialize a derived class "
                      "without md5");
      }
      else if constexpr (check_if_has_container<T>()) {
        unsigned char metainfo;
        if SP_UNLIKELY (!read_wrapper<sizeof(unsigned char)>(
                            reader_, (char *)&metainfo)) {
          return {struct_pack::errc::no_buffer_space, 0};
        }
        size_type_ = (metainfo & 0b11000) >> 3;
        return {};
      }
      else {
        size_type_ = 0;
        return {};
      }
    }
    else {
      if constexpr (is_MD5_reader_wrapper<Reader>) {
        reader_.read_head((char *)&current_types_code);
        if SP_LIKELY (current_types_code % 2 == 0)  // unexist metainfo
        {
          size_type_ = 0;
          return {};
        }
      }
      else {
        if SP_UNLIKELY (!read_wrapper<sizeof(uint32_t)>(
                            reader_, (char *)&current_types_code)) {
          return {struct_pack::errc::no_buffer_space, 0};
        }
        constexpr uint32_t types_code = get_types_code<T>();
        if SP_UNLIKELY ((current_types_code / 2) != (types_code / 2)) {
          return {struct_pack::errc::invalid_buffer, 0};
        }
        if SP_LIKELY (current_types_code % 2 == 0)  // unexist metainfo
        {
          size_type_ = 0;
          return {};
        }
      }
      unsigned char metainfo;
      if SP_UNLIKELY (!read_wrapper<sizeof(unsigned char)>(reader_,
                                                           (char *)&metainfo)) {
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
      static_assert(
          is_little_endian_copyable<sizeof(typename type::value_type)>,
          "get a trivial view with byte width > 1 in big-endian system is "
          "not allowed.");
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
          if SP_UNLIKELY (!read_wrapper<sizeof(type)>(reader_, (char *)&item)) {
            return struct_pack::errc::no_buffer_space;
          }
        }
        else {
          return reader_.ignore(sizeof(type)) ? errc{} : errc::no_buffer_space;
        }
      }
      else if constexpr (id == type_id::bitset_t) {
        if constexpr (NotSkip) {
          if SP_UNLIKELY (!read_bytes_array(reader_, (char *)&item,
                                            sizeof(type))) {
            return struct_pack::errc::no_buffer_space;
          }
        }
        else {
          return reader_.ignore(sizeof(type)) ? errc{} : errc::no_buffer_space;
        }
      }
      else if constexpr (unique_ptr<type>) {
        bool has_value{};
        if SP_UNLIKELY (!read_wrapper<sizeof(bool)>(reader_,
                                                    (char *)&has_value)) {
          return struct_pack::errc::no_buffer_space;
        }
        if (!has_value) {
          return {};
        }
        if constexpr (is_base_class<typename type::element_type>) {
          uint32_t id;
          read_wrapper<sizeof(id)>(reader_, (char *)&id);
          bool ok;
          auto index = search_type_by_md5<typename type::element_type>(id, ok);
          if SP_UNLIKELY (!ok) {
            return errc::invalid_buffer;
          }
          else {
            return template_switch<deserialize_one_derived_class_helper<
                derived_class_set_t<typename type::element_type>,
                std::integral_constant<std::size_t, size_type>,
                std::integral_constant<std::uint64_t, version>,
                std::integral_constant<std::uint64_t, NotSkip>>>(index, this,
                                                                 item);
          }
        }
        else {
          item = std::make_unique<typename type::element_type>();
          deserialize_one<size_type, version, NotSkip>(*item);
        }
      }
      else if constexpr (detail::varint_t<type>) {
        code = detail::deserialize_varint<NotSkip>(reader_, item);
      }
      else if constexpr (id == type_id::array_t) {
        if constexpr (is_trivial_serializable<type>::value &&
                      is_little_endian_copyable<sizeof(item[0])>) {
          if constexpr (NotSkip) {
            if SP_UNLIKELY (!read_bytes_array(reader_, (char *)&item,
                                              sizeof(item))) {
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
        uint16_t size16;
        uint32_t size32;
        uint64_t size64;
        bool result;
        if constexpr (size_type == 1) {
          uint8_t size8;
          if SP_UNLIKELY (!read_wrapper<size_type>(reader_, (char *)&size8)) {
            return struct_pack::errc::no_buffer_space;
          }
          size64 = size8;
        }
#ifdef STRUCT_PACK_OPTIMIZE
        else if constexpr (size_type == 2) {
          if SP_UNLIKELY (!read_wrapper<size_type>(reader_, (char *)&size16)) {
            return struct_pack::errc::no_buffer_space;
          }
          size64 = size16;
        }
        else if constexpr (size_type == 4) {
          if SP_UNLIKELY (!read_wrapper<size_type>(reader_, (char *)&size32)) {
            return struct_pack::errc::no_buffer_space;
          }
          size64 = size32;
        }
        else if constexpr (size_type == 8) {
          if SP_UNLIKELY (!read_wrapper<size_type>(reader_, (char *)&size64)) {
            return struct_pack::errc::no_buffer_space;
          }
        }
        else {
          static_assert(!sizeof(T), "illegal size_type");
        }
#else
        else {
          switch (size_type_) {
            case 1:
              if SP_UNLIKELY (!read_wrapper<2>(reader_, (char *)&size16)) {
                return struct_pack::errc::no_buffer_space;
              }
              size64 = size16;
              break;
            case 2:
              if SP_UNLIKELY (!read_wrapper<4>(reader_, (char *)&size32)) {
                return struct_pack::errc::no_buffer_space;
              }
              size64 = size32;
              break;
            case 3:
              if SP_UNLIKELY (!read_wrapper<8>(reader_, (char *)&size64)) {
                return struct_pack::errc::no_buffer_space;
              }
              break;
            default:
              unreachable();
          }
        }
#endif
        if SP_UNLIKELY (size64 == 0) {
          return {};
        }
        if constexpr (map_container<type>) {
          std::pair<typename type::key_type, typename type::mapped_type>
              value{};
          if constexpr (is_trivial_serializable<decltype(value)>::value &&
                        !NotSkip) {
            return reader_.ignore(size64 * sizeof(value))
                       ? errc{}
                       : errc::no_buffer_space;
          }
          else {
            for (uint64_t i = 0; i < size64; ++i) {
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
            return reader_.ignore(size64 * sizeof(value))
                       ? errc{}
                       : errc::no_buffer_space;
          }
          else {
            for (uint64_t i = 0; i < size64; ++i) {
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
            uint64_t mem_sz = size64 * sizeof(value_type);
            if constexpr (NotSkip) {
              if constexpr (string_view<type> || dynamic_span<type>) {
                static_assert(
                    view_reader_t<Reader>,
                    "The Reader isn't a view_reader, can't deserialize "
                    "a string_view/span");
                static_assert(is_little_endian_copyable<sizeof(value_type)>,
                              "zero-copy in big endian is limit.");
                const char *view = reader_.read_view(mem_sz);
                if SP_UNLIKELY (view == nullptr) {
                  return struct_pack::errc::no_buffer_space;
                }
                item = {(value_type *)(view), (std::size_t)size64};
              }
              else if constexpr (is_little_endian_copyable<sizeof(
                                     value_type)>) {
                if SP_UNLIKELY (mem_sz >= PTRDIFF_MAX)
                  unreachable();
                else {
                  item.resize(size64);
                  if SP_UNLIKELY (!read_bytes_array(
                                      reader_, (char *)item.data(),
                                      size64 * sizeof(value_type))) {
                    return struct_pack::errc::no_buffer_space;
                  }
                }
              }
              else {
                item.resize(size64);
                for (auto &i : item) {
                  code = deserialize_one<size_type, version, NotSkip>(i);
                  if SP_UNLIKELY (code != struct_pack::errc{}) {
                    return code;
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
                item.resize(size64);
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
              for (size_t i = 0; i < size64; ++i) {
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
        if SP_UNLIKELY (!read_wrapper<sizeof(bool)>(reader_,
                                                    (char *)&has_value)) {
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
        if SP_UNLIKELY (!read_wrapper<sizeof(index)>(reader_, (char *)&index)) {
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
        if constexpr (is_trivial_serializable<type>::value &&
                      is_little_endian_copyable<sizeof(type)>) {
          if constexpr (NotSkip) {
            if SP_UNLIKELY (!read_wrapper<sizeof(type)>(reader_,
                                                        (char *)&item)) {
              return struct_pack::errc::no_buffer_space;
            }
          }
          else {
            return reader_.ignore(sizeof(type)) ? errc{}
                                                : errc::no_buffer_space;
          }
        }
        else if constexpr ((is_trivial_serializable<type>::value &&
                            !is_little_endian_copyable<sizeof(type)>) ||
                           is_trivial_serializable<type, true>::value) {
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
          if SP_UNLIKELY (!read_wrapper<sizeof(bool)>(reader_,
                                                      (char *)&has_value)) {
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
        if constexpr (is_base_class<typename type::element_type>) {
          uint32_t id = item->get_struct_pack_id();
          bool ok;
          auto index = search_type_by_md5<typename type::element_type>(id, ok);
          assert(ok);
          return template_switch<deserialize_one_derived_class_helper<
              derived_class_set_t<typename type::element_type>,
              std::integral_constant<std::size_t, size_type>,
              std::integral_constant<std::uint64_t, version>,
              std::integral_constant<std::uint64_t, NotSkip>>>(index, this,
                                                               item);
        }
        else {
          deserialize_one<size_type, version, NotSkip>(*item);
        }
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

template <typename Reader>
struct MD5_reader_wrapper : public Reader {
  MD5_reader_wrapper(Reader &&reader) : Reader(std::move(reader)) {
    is_failed =
        !read_wrapper<sizeof(head_chars)>(*(Reader *)this, (char *)&head_chars);
  }
  bool read_head(char *target) {
    if SP_UNLIKELY (is_failed) {
      return false;
    }
    memcpy(target, &head_chars, sizeof(head_chars));
    return true;
  }
  Reader &&release_reader() { return std::move(*(Reader *)this); }
  bool is_failed;
  uint32_t get_md5() { return head_chars & 0xFFFFFFFE; }

 private:
  std::uint32_t head_chars;
  std::size_t read_pos;
};

template <typename BaseClass, typename... DerivedClasses, typename Reader>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_derived_class(
    std::unique_ptr<BaseClass> &base, Reader &reader) {
  MD5_reader_wrapper wrapper{std::move(reader)};
  if (wrapper.is_failed) {
    return struct_pack::errc::no_buffer_space;
  }
  unpacker<MD5_reader_wrapper<Reader>> unpack{wrapper};
  constexpr auto &MD5s = MD5_set<std::tuple<DerivedClasses...>>::value;
  MD5_pair md5_pair{wrapper.get_md5(), 0};
  auto result = std::lower_bound(MD5s.begin(), MD5s.end(), md5_pair);
  if (result == MD5s.end() || result->md5 != md5_pair.md5) {
    return struct_pack::errc::invalid_buffer;
  }
  auto ret = template_switch<
      deserialize_derived_class_helper<std::tuple<DerivedClasses...>>>(
      result->index, base, unpack);
  reader = std::move(wrapper.release_reader());
  return ret;
}
}  // namespace detail

}  // namespace struct_pack
