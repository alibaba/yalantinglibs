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

#include <cstdint>
#include <type_traits>
#include <utility>

#include "struct_pack/struct_pack_impl.hpp"

#if __has_include(<expected>) && __cplusplus > 202002L
#include <expected>
#if __cpp_lib_expected >= 202202L
#else
#include "util/expected.hpp"
#endif
#else
#include "util/expected.hpp"
#endif

/*! \defgroup struct_pack struct_pack
 *  \brief yaLanTingLibs Serialization Library
 *
 *  Based on compile-time reflection, very easy to use,
 *  high performance serialization library,
 *  struct_pack is a header only library, it is used by coro_rpc now.
 *
 *  \section struct_pack_compiler_support Compiler Support
 *
 * | Compiler      | Version |
 * | ----------- | ------------------ |
 * | GCC         | 10.2               |
 * | Clang       | 13.0.1             |
 * | Apple Clang | 13.1.16            |
 * | MSVC        | 14.32              |
 *
 * \section struct_pack_memory_layout Memory Layout
 *
 * a simple object after serialization
 *
 *
 * ![person](images/mem_layout_of_person.png)
 *
 * ![vec](images/mem_layout_of_vec_rect.png)
 *
 * ![std::vector<int32_t>](images/mem_layout_of_vec_int.png)
 *
 * The struct pack has a compact data format.
 * For fundamental types (e.g. int, float) and fixed length array,
 * the actual length is used.
 * For container concept (e.g string, stl container),
 * the element size is placed first, then all elements are wrote one by one.
 *
 * \subsection struct_pack_speed How to optimize serialization or
 * deserialization
 *
 * 1. using fixed-length string as far as possible
 * 2. encapsulating memory contiguous fields into a single object
 * 3. using memory contiguous container (e.g. std::vector)
 *
 * \subsection struct_pack_binary How to optimize serialization binary size
 *
 * For container and std::string, the field of element size can be compressed.
 * You can use `add_definitions` on CMake to get the optimization.
 * ```
 * # use 1 byte to represent the element size, require size < 256
 * add_definitions(-DSTRUCT_PACK_USE_INT8_SIZE)
 * # use 2 bytes to represent the element size, require size < 65536
 * add_definitions(-DSTRUCT_PACK_USE_INT16_SIZE)
 * ```
 *
 * \subsection struct_pack_limitation Limitation
 *
 * 1. the element size must less than UINT32_MAX.
 * 2. no pointer or reference allowed in object, otherwise compile-time error
 * reported.
 * 3. the object must be
 * [aggregate](https://en.cppreference.com/w/cpp/language/aggregate_initialization),
 * otherwise compile-time error reported.
 *
 */
namespace struct_pack {

#if __cpp_lib_expected >= 202202L && __cplusplus > 202002L
template <class T, class E>
using expected = std::expected<T, E>;

template <class T>
using unexpected = std::unexpected<T>;

using unexpect_t = std::unexpect_t;

#else
template <class T, class E>
using expected = tl::expected<T, E>;

template <class T>
using unexpected = tl::unexpected<T>;

using unexpect_t = tl::unexpect_t;
#endif

inline std::error_code make_error_code(struct_pack::errc err) {
  return std::error_code(static_cast<int>(err),
                         struct_pack::detail::category());
}

/*!
 * \ingroup struct_pack
 * Get the error message corresponding to the error code `err`.
 * @param err error code.
 * @return error message.
 */
STRUCT_PACK_INLINE std::string error_message(struct_pack::errc err) {
  return struct_pack::make_error_code(err).message();
}

/*!
 * \ingroup struct_pack
 * Get the byte size of the packing objects.
 * TODO: add doc
 * @tparam Args the types of packing objects.
 * @param args the packing objects.
 * @return byte size.
 */

template <typename... Args>
STRUCT_PACK_INLINE consteval std::uint32_t get_type_code() {
  static_assert(sizeof...(Args) > 0);
  std::uint32_t ret;
  if constexpr (sizeof...(Args) == 1) {
    ret = detail::get_types_code<Args..., decltype(detail::get_types(
                                              std::declval<Args...>()))>();
  }
  else {
    ret = detail::get_types_code<std::tuple<std::remove_cvref_t<Args>...>,
                                 std::tuple<Args...>>();
  }
  ret = ret - ret % 2;
  return ret;
}

template <typename... Args>
STRUCT_PACK_INLINE consteval decltype(auto) get_type_literal() {
  static_assert(sizeof...(Args) > 0);
  if constexpr (sizeof...(Args) == 1) {
    using Types = decltype(detail::get_types(Args{}...));
    return detail::get_types_literal<Args..., Types>(
        std::make_index_sequence<std::tuple_size_v<Types>>());
  }
  else {
    return detail::get_types_literal<std::tuple<std::remove_cvref_t<Args>...>,
                                     std::remove_cvref_t<Args>...>();
  }
}

template <serialize_config conf = serialize_config{}, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE constexpr size_t get_needed_size(
    const Args &...args) {
  return detail::get_serialize_runtime_info<conf>(args...).len;
}

template <serialize_config conf = serialize_config{},
          detail::struct_pack_byte Byte, typename... Args>
std::size_t STRUCT_PACK_INLINE serialize_to(Byte *buffer, std::size_t len,
                                            const Args &...args) noexcept {
  static_assert(sizeof...(args) > 0);
  auto config = detail::get_serialize_runtime_info<conf>(args...);
  if (config.len > len) [[unlikely]] {
    return 0;
  }
  detail::packer<Byte, detail::get_args_type<Args...>> o(buffer, config);
  switch ((config.metainfo & 0b11000) >> 3) {
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
  return config.len;
}

template <serialize_config conf = serialize_config{},
          detail::struct_pack_buffer Buffer, typename... Args>
STRUCT_PACK_INLINE void serialize_to(Buffer &buffer, const Args &...args) {
  static_assert(sizeof...(args) > 0);
  auto data_offset = buffer.size();
  auto config = detail::get_serialize_runtime_info<conf>(args...);
  auto total = data_offset + config.len;
  buffer.resize(total);

  detail::packer<std::remove_reference_t<decltype(*buffer.data())>,
                 detail::get_args_type<Args...>>
      o(buffer.data() + data_offset, config);
  switch ((config.metainfo & 0b11000) >> 3) {
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

template <serialize_config conf = serialize_config{},
          detail::struct_pack_buffer Buffer, typename... Args>
void STRUCT_PACK_INLINE serialize_to_with_offset(Buffer &buffer,
                                                 std::size_t offset,
                                                 const Args &...args) {
  static_assert(sizeof...(args) > 0);
  buffer.resize(buffer.size() + offset);
  serialize_to<conf>(buffer, args...);
}

template <detail::struct_pack_buffer Buffer = std::vector<char>,
          serialize_config conf = serialize_config{}, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE Buffer serialize(const Args &...args) {
  static_assert(sizeof...(args) > 0);
  Buffer buffer;
  serialize_to<conf>(buffer, args...);
  return buffer;
}

template <detail::struct_pack_buffer Buffer = std::vector<char>,
          serialize_config conf = serialize_config{}, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE Buffer
serialize_with_offset(std::size_t offset, const Args &...args) {
  static_assert(sizeof...(args) > 0);
  Buffer buffer;
  buffer.resize(offset);
  serialize_to<conf>(buffer, args...);
  return buffer;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, const View &v, Args &...args) {
  detail::unpacker in(v.data(), v.size());
  return in.deserialize(t, args...);
}

template <typename T, typename... Args, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, const Byte *data, size_t size, Args &...args) {
  detail::unpacker in(data, size);
  return in.deserialize(t, args...);
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, const View &v, size_t &consume_len, Args &...args) {
  detail::unpacker in(v.data(), v.size());
  return in.deserialize(consume_len, t, args...);
}

template <typename T, typename... Args, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, const Byte *data, size_t size, size_t &consume_len, Args &...args) {
  detail::unpacker in(data, size);
  return in.deserialize(consume_len, t, args...);
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to_with_offset(
    T &t, const View &v, size_t &offset, Args &...args) {
  detail::unpacker in(v.data() + offset, v.size() - offset);
  size_t sz;
  auto ret = in.deserialize(sz, t, args...);
  offset += sz;
  return ret;
}

template <typename T, typename... Args, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to_with_offset(
    T &t, const Byte *data, size_t size, size_t &offset, Args &...args) {
  detail::unpacker in(data + offset, size - offset);
  size_t sz;
  auto ret = in.deserialize(sz, t, args...);
  offset += sz;
  return ret;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(const View &v) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), v); errc != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(const Byte *data,
                                                  size_t size) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), data, size);
      errc != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(const View &v,
                                                  size_t &consume_len) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), v, consume_len);
      errc != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(const Byte *data, size_t size,
                                                  size_t &consume_len) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), data, size, consume_len);
      errc != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize_with_offset(const View &v,
                                                              size_t &offset) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to_with_offset(ret.value(), v, offset);
      errc != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize_with_offset(const Byte *data,
                                                              size_t size,
                                                              size_t &offset) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to_with_offset(ret.value(), data, size, offset);
      errc != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, size_t I, typename Field, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc get_field_to(Field &dst,
                                                                const View &v) {
  using T_Field =
      std::tuple_element_t<I, decltype(detail::get_types(std::declval<T>()))>;
  static_assert(std::is_same_v<Field, T_Field>,
                "The dst's type is not correct. It should be as same as the "
                "T's Ith field's type");
  detail::unpacker in(v.data(), v.size());
  return in.template get_field<T, I>(dst);
}

template <typename T, size_t I, typename Field, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc get_field_to(
    Field &dst, const Byte *data, size_t size) {
  using T_Field =
      std::tuple_element_t<I, decltype(detail::get_types(std::declval<T>()))>;
  static_assert(std::is_same_v<Field, T_Field>,
                "The dst's type is not correct. It should be as same as the "
                "T's Ith field's type");
  detail::unpacker in(data, size);
  return in.template get_field<T, I>(dst);
}

template <typename T, size_t I, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE auto get_field(const View &v) {
  using T_Field =
      std::tuple_element_t<I, decltype(detail::get_types(std::declval<T>()))>;
  detail::unpacker in(v.data(), v.size());
  expected<T_Field, struct_pack::errc> ret;

  if (auto ec = get_field_to<T, I>(ret.value(), v); ec != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{ec};
  }
  return ret;
}

template <typename T, size_t I, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE auto get_field(const Byte *data, size_t size) {
  using T_Field =
      std::tuple_element_t<I, decltype(detail::get_types(std::declval<T>()))>;
  detail::unpacker in(data, size);
  expected<T_Field, struct_pack::errc> ret;

  if (auto ec = get_field_to<T, I>(ret.value(), data, size);
      ec != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{ec};
  }
  return ret;
}
}  // namespace struct_pack