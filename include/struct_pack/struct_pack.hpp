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
#include "struct_pack/struct_pack_impl.hpp"

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
 * \subsection struct_pack_speed How to optimize serialization or deserialization
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
 * 2. no pointer or reference allowed in object, otherwise compile-time error reported.
 * 3. the object must be
 * [aggregate](https://en.cppreference.com/w/cpp/language/aggregate_initialization),
 * otherwise compile-time error reported.
 *
 */
namespace struct_pack {
/*!
 * \ingroup struct_pack
 * Get the error message corresponding to the error code `err`.
 * @param err error code.
 * @return error message.
 */

template <size_t N>
struct members_count_t : public std::integral_constant<size_t, N> {};

STRUCT_PACK_INLINE std::string error_message(std::errc err) {
  return std::make_error_code(err).message();
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
STRUCT_PACK_INLINE consteval std::size_t get_type_code() {
  static_assert(sizeof...(Args) > 0);
  if constexpr (sizeof...(Args) == 1) {
    return detail::get_types_code<decltype(detail::get_types(
        std::declval<Args...>()))>();
  }
  else {
    return detail::get_types_code<std::tuple<Args...>>();
  }
}

template <typename... Args>
STRUCT_PACK_INLINE consteval decltype(auto) get_type_literal() {
  static_assert(sizeof...(Args) > 0);
  if constexpr (sizeof...(Args) == 1) {
    return detail::get_types_literal<Args...>();
  }
  else {
    return detail::get_types_literal<std::tuple<Args...>>();
  }
}

template <typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE constexpr size_t get_needed_size(
    const Args &...args) {
  if constexpr ((detail::unexist_compatible_member<Args> && ...))
    return detail::calculate_needed_size(args...) + sizeof(uint32_t);
  else
    return detail::calculate_needed_size(args...) + sizeof(uint32_t) +
           sizeof(uint64_t);
}

template <detail::struct_pack_byte Byte, typename... Args>
std::size_t STRUCT_PACK_INLINE serialize_to(Byte *buffer, std::size_t len,
                                            const Args &...args) noexcept {
  static_assert(sizeof...(args) > 0);
  auto size = get_needed_size(args...);
  if (size > len) [[unlikely]] {
    return 0;
  }
  detail::packer o(buffer);
  if constexpr ((detail::unexist_compatible_member<Args> && ...)) {
    o.serialize(args...);
  }
  else {
    o.serialize_with_size(size, args...);
  }
  return size;
}

template <detail::struct_pack_buffer Buffer, typename... Args>
void STRUCT_PACK_INLINE serialize_to(Buffer &buffer, const Args &...args) {
  static_assert(sizeof...(args) > 0);
  auto data_offset = buffer.size();
  auto need_size = get_needed_size(args...);
  auto total = data_offset + need_size;
  buffer.resize(total);
  detail::packer o(buffer.data() + data_offset);
  if constexpr ((detail::unexist_compatible_member<Args> && ...)) {
    o.serialize(args...);
  }
  else {
    o.serialize(need_size, args...);
  }
}

template <detail::struct_pack_buffer Buffer, typename... Args>
void STRUCT_PACK_INLINE serialize_to_with_offset(Buffer &buffer,
                                                 std::size_t offset,
                                                 const Args &...args) {
  static_assert(sizeof...(args) > 0);
  buffer.resize(buffer.size() + offset);
  serialize_to(buffer, args...);
}

template <detail::struct_pack_buffer Buffer = std::vector<char>,
          typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE Buffer serialize(const Args &...args) {
  static_assert(sizeof...(args) > 0);
  Buffer buffer;
  serialize_to(buffer, args...);
  return buffer;
}

template <detail::struct_pack_buffer Buffer = std::vector<char>,
          typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE Buffer
serialize_with_offset(std::size_t offset, const Args &...args) {
  static_assert(sizeof...(args) > 0);
  Buffer buffer;
  buffer.resize(offset);
  serialize_to(buffer, args...);
  return buffer;
}

template <typename T, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_to(T &t, const View &v) {
  detail::unpacker in(v.data(), v.size());
  return in.deserialize(t);
}

template <typename T, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_to(T &t,
                                                          const Byte *data,
                                                          size_t size) {
  detail::unpacker in(data, size);
  return in.deserialize(t);
}

template <typename T, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_to(T &t, const View &v,
                                                          size_t &consume_len) {
  detail::unpacker in(v.data(), v.size());
  return in.deserialize(t, consume_len);
}

template <typename T, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_to(T &t,
                                                          const Byte *data,
                                                          size_t size,
                                                          size_t &consume_len) {
  detail::unpacker in(data, size);
  return in.deserialize(t, consume_len);
}

template <typename T, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_to_with_offset(
    T &t, const View &v, size_t &offset) {
  detail::unpacker in(v.data() + offset, v.size() - offset);
  size_t sz;
  auto ret = in.deserialize(t, sz);
  offset += sz;
  return ret;
}

template <typename T, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE std::errc deserialize_to_with_offset(
    T &t, const Byte *data, size_t size, size_t &offset) {
  detail::unpacker in(data + offset, size - offset);
  size_t sz;
  auto ret = in.deserialize(t, sz);
  offset += sz;
  return ret;
}



template <typename T, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE deserialize_result<T> deserialize(
    const View &v) {
  deserialize_result<T> ret;
  ret.errc = deserialize_to(ret.value, v);
  return ret;
}

template <typename T, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE deserialize_result<T> deserialize(
    const Byte *data, size_t size) {
  deserialize_result<T> ret;
  ret.errc = deserialize_to(ret.value, data, size);
  return ret;
}

template <typename T, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE deserialize_result<T> deserialize(
    const View &v, size_t &consume_len) {
  deserialize_result<T> ret;
  ret.errc = deserialize_to(ret.value, v, consume_len);
  return ret;
}

template <typename T, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE deserialize_result<T> deserialize(
    const Byte *data, size_t size, size_t &consume_len) {
  deserialize_result<T> ret;
  ret.errc = deserialize_to(ret.value, data, size, consume_len);
  return ret;
}

template <typename T, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE deserialize_result<T> deserialize_with_offset(
    const View &v, size_t &offset) {
  deserialize_result<T> ret;
  ret.errc = deserialize_to(ret.value, v, offset);
  return ret;
}

template <typename T, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE deserialize_result<T> deserialize_with_offset(
    const Byte *data, size_t size, size_t &offset) {
  deserialize_result<T> ret;
  ret.errc = deserialize_to_with_offset(ret.value, data, size, offset);
  return ret;
}

template <typename T, size_t I, detail::deserialize_view View>
[[nodiscard]] STRUCT_PACK_INLINE decltype(auto) get_field(const View &v) {
  detail::unpacker in(v.data(), v.size());
  return in.template get_field<T, I>();
}

template <typename T, size_t I, detail::struct_pack_byte Byte>
[[nodiscard]] STRUCT_PACK_INLINE decltype(auto) get_field(const Byte *data,
                                                          size_t size) {
  detail::unpacker in(data, size);
  return in.template get_field<T, I>();
}
}  // namespace struct_pack