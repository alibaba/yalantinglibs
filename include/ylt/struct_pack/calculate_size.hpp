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

#include "alignment.hpp"
#include "marco.h"
#include "reflection.hpp"
#include "size_info.hpp"
#include "type_id.hpp"
#include "type_trait.hpp"
#include "util.h"
#include "varint.hpp"

namespace struct_pack {

struct serialize_buffer_size;
namespace detail {
template <typename... Args>
constexpr size_info STRUCT_PACK_INLINE
calculate_payload_size(const Args &...items);

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
                     id == type_id::int128_t || id == type_id::uint128_t ||
                     id == type_id::bitset_t) {
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
    ret.max_size = item.size();
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
    if (item) {
      ret += calculate_one_size(*item);
    }
  }
  else if constexpr (unique_ptr<type>) {
    ret.total = sizeof(char);
    if (item) {
      if constexpr (is_base_class<typename type::element_type>) {
        ret.total += sizeof(uint32_t);
        bool is_ok;
        auto index = search_type_by_md5<typename type::element_type>(
            item->get_struct_pack_id(), is_ok);
        if SP_UNLIKELY (!is_ok) {
          throw std::runtime_error{
              "illegal struct_pack_id in virtual function."};
        }
        ret += template_switch<calculate_one_size_derived_class_helper<
            derived_class_set_t<typename type::element_type>>>(index,
                                                               item.get());
      }
      else {
        ret += calculate_one_size(*item);
      }
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

template <typename... Args>
constexpr size_info STRUCT_PACK_INLINE
calculate_payload_size(const Args &...items) {
  return (calculate_one_size(items) + ...);
}
template <uint64_t conf, typename... Args>
STRUCT_PACK_INLINE constexpr serialize_buffer_size get_serialize_runtime_info(
    const Args &...args);
}  // namespace detail
struct serialize_buffer_size {
 private:
  std::size_t len_;
  unsigned char metainfo_;

 public:
  constexpr serialize_buffer_size() : len_(0), metainfo_(0) {}
  constexpr std::size_t size() const { return len_; }
  constexpr unsigned char metainfo() const { return metainfo_; }
  constexpr operator std::size_t() const { return len_; }

  template <uint64_t conf, typename... Args>
  friend STRUCT_PACK_INLINE constexpr serialize_buffer_size
  struct_pack::detail::get_serialize_runtime_info(const Args &...args);
};
namespace detail {
template <uint64_t conf, typename... Args>
[[nodiscard]] STRUCT_PACK_INLINE constexpr serialize_buffer_size
get_serialize_runtime_info(const Args &...args) {
  using Type = get_args_type<Args...>;
  constexpr bool has_compatible = serialize_static_config<Type>::has_compatible;
  constexpr bool has_type_literal = check_if_add_type_literal<conf, Type>();
  constexpr bool disable_hash_head = check_if_disable_hash_head<conf, Type>();
  constexpr bool has_container = check_if_has_container<Type>();
  constexpr bool has_compile_time_determined_meta_info =
      check_has_metainfo<conf, Type>();
  serialize_buffer_size ret;
  auto sz_info = calculate_payload_size(args...);
  if constexpr (has_compile_time_determined_meta_info) {
    ret.len_ = sizeof(unsigned char);
  }
  if constexpr (!has_container) {
    ret.len_ += sz_info.total;
  }
  else {
    if SP_LIKELY (sz_info.max_size < (int64_t{1} << 8)) {
      ret.len_ += sz_info.total + sz_info.size_cnt;
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
      if constexpr (!has_compile_time_determined_meta_info) {
        ret.len_ += sizeof(unsigned char);
      }
      // size_type >= 1 , has metainfo
    }
  }
  if constexpr (!disable_hash_head) {
    ret.len_ += sizeof(uint32_t);  // for record hash code
    if constexpr (has_type_literal) {
      constexpr auto type_literal = struct_pack::get_type_literal<Args...>();
      // struct_pack::get_type_literal<Args...>().size() crash in clang13.
      // Bug?
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
  }
  return ret;
}
}  // namespace detail
}  // namespace struct_pack