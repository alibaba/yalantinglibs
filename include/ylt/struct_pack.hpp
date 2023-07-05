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

STRUCT_PACK_INLINE std::error_code make_error_code(struct_pack::errc err) {
  return std::error_code(static_cast<int>(err),
                         struct_pack::detail::category());
}

/*!
 * \defgroup struct_pack struct_pack
 *
 * \brief yaLanTingLibs struct_pack 序列化库
 *
 * coro_rpc分为服务端和客户端，服务端包括rpc函数注册API和服务器对象的API，客户端包括rpc调用API。
 *
 */

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
    ret = detail::get_types_code<Args...,
                                 decltype(detail::get_types<Args...>())>();
  }
  else {
    ret = detail::get_types_code<std::tuple<std::remove_cvref_t<Args>...>,
                                 std::tuple<Args...>>();
  }
  ret = ret - ret % 2;
  return ret;
}

template <typename... Args>
STRUCT_PACK_INLINE constexpr decltype(auto) get_type_literal() {
  static_assert(sizeof...(Args) > 0);
  if constexpr (sizeof...(Args) == 1) {
    using Types = decltype(detail::get_types<Args...>());
    return detail::get_types_literal<Args..., Types>(
        std::make_index_sequence<std::tuple_size_v<Types>>());
  }
  else {
    return detail::get_types_literal<std::tuple<std::remove_cvref_t<Args>...>,
                                     std::remove_cvref_t<Args>...>();
  }
}

template <serialize_config conf = serialize_config{}, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE constexpr serialize_buffer_size
get_needed_size(const Args &...args) {
  return detail::get_serialize_runtime_info<conf>(args...);
}

template <serialize_config conf = serialize_config{},
          struct_pack::writer_t Writer, typename... Args>
STRUCT_PACK_INLINE void serialize_to(Writer &writer, const Args &...args) {
  static_assert(sizeof...(args) > 0);
  auto info = detail::get_serialize_runtime_info<conf>(args...);
  struct_pack::detail::serialize_to<conf>(writer, info, args...);
}

template <serialize_config conf = serialize_config{}, typename... Args>
void STRUCT_PACK_INLINE serialize_to(char *buffer, serialize_buffer_size info,
                                     const Args &...args) noexcept {
  static_assert(sizeof...(args) > 0);
  auto writer = struct_pack::detail::memory_writer{(char *)buffer};
  struct_pack::detail::serialize_to<conf>(writer, info, args...);
}

template <serialize_config conf = serialize_config{},
          detail::struct_pack_buffer Buffer, typename... Args>
void STRUCT_PACK_INLINE serialize_to(Buffer &buffer, const Args &...args) {
  static_assert(sizeof...(args) > 0);
  auto data_offset = buffer.size();
  auto info = detail::get_serialize_runtime_info<conf>(args...);
  auto total = data_offset + info.size();
  buffer.resize(total);
  auto writer =
      struct_pack::detail::memory_writer{(char *)buffer.data() + data_offset};
  struct_pack::detail::serialize_to<conf>(writer, info, args...);
}

template <serialize_config conf = serialize_config{},
          detail::struct_pack_buffer Buffer, typename... Args>
void STRUCT_PACK_INLINE serialize_to_with_offset(Buffer &buffer,
                                                 std::size_t offset,
                                                 const Args &...args) {
  static_assert(sizeof...(args) > 0);
  auto info = detail::get_serialize_runtime_info<conf>(args...);
  auto old_size = buffer.size();
  buffer.resize(old_size + offset + info.size());
  auto writer = struct_pack::detail::memory_writer{(char *)buffer.data() +
                                                   old_size + offset};
  struct_pack::detail::serialize_to<conf>(writer, info, args...);
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
  serialize_to_with_offset<conf>(buffer, offset, args...);
  return buffer;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, const View &v, Args &...args) {
  detail::memory_reader reader{(const char *)v.data(),
                               (const char *)v.data() + v.size()};
  detail::unpacker in(reader);
  return in.deserialize(t, args...);
}

template <typename T, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, const char *data, size_t size, Args &...args) {
  detail::memory_reader reader{data, data + size};
  detail::unpacker in(reader);
  return in.deserialize(t, args...);
}

template <typename T, typename... Args, struct_pack::reader_t Reader>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, Reader &reader, Args &...args) {
  detail::unpacker in(reader);
  std::size_t consume_len;
  auto old_pos = reader.tellg();
  auto ret = in.deserialize_with_len(consume_len, t, args...);
  std::size_t delta = reader.tellg() - old_pos;
  if (ret == errc{}) [[likely]] {
    if (consume_len > 0) [[likely]] {
      if (delta > consume_len) [[unlikely]] {
        ret = struct_pack::errc::invalid_buffer;
        if constexpr (struct_pack::seek_reader_t<Reader>)
          if (!reader.seekg(old_pos)) [[unlikely]] {
            return struct_pack::errc::seek_failed;
          }
      }
      else {
        reader.ignore(consume_len - delta);
      }
    }
  }
  else {
    if constexpr (struct_pack::seek_reader_t<Reader>)
      if (!reader.seekg(old_pos)) [[unlikely]] {
        return struct_pack::errc::seek_failed;
      }
  }
  return ret;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, const View &v, size_t &consume_len, Args &...args) {
  detail::memory_reader reader{(const char *)v.data(),
                               (const char *)v.data() + v.size()};
  detail::unpacker in(reader);
  auto ret = in.deserialize_with_len(consume_len, t, args...);
  if (ret == errc{}) [[likely]] {
    consume_len = (std::max)((size_t)(reader.now - v.data()), consume_len);
  }
  else {
    consume_len = 0;
  }
  return ret;
}

template <typename T, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to(
    T &t, const char *data, size_t size, size_t &consume_len, Args &...args) {
  detail::memory_reader reader{data, data + size};
  detail::unpacker in(reader);
  auto ret = in.deserialize_with_len(consume_len, t, args...);
  if (ret == errc{}) [[likely]] {
    consume_len = (std::max)((size_t)(reader.now - data), consume_len);
  }
  else {
    consume_len = 0;
  }
  return ret;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to_with_offset(
    T &t, const View &v, size_t &offset, Args &...args) {
  size_t sz;
  auto ret =
      deserialize_to(t, v.data() + offset, v.size() - offset, sz, args...);
  offset += sz;
  return ret;
}

template <typename T, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_to_with_offset(
    T &t, const char *data, size_t size, size_t &offset, Args &...args) {
  size_t sz;
  auto ret = deserialize_to(t, data + offset, size - offset, sz, args...);
  offset += sz;
  return ret;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(const View &v) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), v); errc != struct_pack::errc{})
      [[unlikely]] {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(const char *data,
                                                  size_t size) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), data, size);
      errc != struct_pack::errc{}) {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args, struct_pack::reader_t Reader>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(Reader &v) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), v); errc != struct_pack::errc{})
      [[unlikely]] {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(const View &v,
                                                  size_t &consume_len) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), v, consume_len);
      errc != struct_pack::errc{}) [[unlikely]] {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize(const char *data, size_t size,
                                                  size_t &consume_len) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to(ret.value(), data, size, consume_len);
      errc != struct_pack::errc{}) [[unlikely]] {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize_with_offset(const View &v,
                                                              size_t &offset) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to_with_offset(ret.value(), v, offset);
      errc != struct_pack::errc{}) [[unlikely]] {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE auto deserialize_with_offset(const char *data,
                                                              size_t size,
                                                              size_t &offset) {
  expected<detail::get_args_type<T, Args...>, struct_pack::errc> ret;
  if (auto errc = deserialize_to_with_offset(ret.value(), data, size, offset);
      errc != struct_pack::errc{}) [[unlikely]] {
    ret = unexpected<struct_pack::errc>{errc};
  }
  return ret;
}

template <typename T, size_t I, typename Field, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc get_field_to(Field &dst,
                                                                const View &v) {
  using T_Field = std::tuple_element_t<I, decltype(detail::get_types<T>())>;
  static_assert(std::is_same_v<Field, T_Field>,
                "The dst's type is not correct. It should be as same as the "
                "T's Ith field's type");
  detail::memory_reader reader((const char *)v.data(),
                               (const char *)v.data() + v.size());
  detail::unpacker in(reader);
  return in.template get_field<T, I>(dst);
}

template <typename T, size_t I, typename Field>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc get_field_to(
    Field &dst, const char *data, size_t size) {
  using T_Field = std::tuple_element_t<I, decltype(detail::get_types<T>())>;
  static_assert(std::is_same_v<Field, T_Field>,
                "The dst's type is not correct. It should be as same as the "
                "T's Ith field's type");
  detail::memory_reader reader{data, data + size};
  detail::unpacker in(reader);
  return in.template get_field<T, I>(dst);
}

template <typename T, size_t I, typename Field, struct_pack::reader_t Reader>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc get_field_to(
    Field &dst, Reader &reader) {
  using T_Field = std::tuple_element_t<I, decltype(detail::get_types<T>())>;
  static_assert(std::is_same_v<Field, T_Field>,
                "The dst's type is not correct. It should be as same as the "
                "T's Ith field's type");
  detail::unpacker in(reader);
  return in.template get_field<T, I>(dst);
}

template <typename T, size_t I, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE auto get_field(const View &v) {
  using T_Field = std::tuple_element_t<I, decltype(detail::get_types<T>())>;
  expected<T_Field, struct_pack::errc> ret;
  if (auto ec = get_field_to<T, I>(ret.value(), v); ec != struct_pack::errc{})
      [[unlikely]] {
    ret = unexpected<struct_pack::errc>{ec};
  }
  return ret;
}

template <typename T, size_t I>
[[nodiscard]] STRUCT_PACK_INLINE auto get_field(const char *data, size_t size) {
  using T_Field = std::tuple_element_t<I, decltype(detail::get_types<T>())>;
  expected<T_Field, struct_pack::errc> ret;
  if (auto ec = get_field_to<T, I>(ret.value(), data, size);
      ec != struct_pack::errc{}) [[unlikely]] {
    ret = unexpected<struct_pack::errc>{ec};
  }
  return ret;
}

template <typename T, size_t I, struct_pack::reader_t Reader>
[[nodiscard]] STRUCT_PACK_INLINE auto get_field(Reader &reader) {
  using T_Field = std::tuple_element_t<I, decltype(detail::get_types<T>())>;
  expected<T_Field, struct_pack::errc> ret;
  if (auto ec = get_field_to<T, I>(ret.value(), reader);
      ec != struct_pack::errc{}) [[unlikely]] {
    ret = unexpected<struct_pack::errc>{ec};
  }
  return ret;
}
}  // namespace struct_pack