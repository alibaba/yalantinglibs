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

#include "calculate_size.hpp"
#include "endian_wrapper.hpp"
#include "reflection.hpp"
namespace struct_pack::detail {
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

  template <typename DerivedClasses, typename size_type, typename version>
  friend struct serialize_one_derived_class_helper;

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
      return get_types_code<remove_cvref_t<T>>();
    }
    else {
      return get_types_code<
          std::tuple<remove_cvref_t<T>, remove_cvref_t<Args>...>>();
    }
  }
  template <uint64_t conf, typename T, typename... Args>
  static constexpr uint32_t STRUCT_PACK_INLINE calculate_hash_head() {
    constexpr uint32_t raw_types_code = calculate_raw_hash<T, Args...>();
    if constexpr (check_has_metainfo<conf, T>()) {
      return raw_types_code + 1;
    }
    else {  // default case, only has hash_code
      return raw_types_code;
    }
  }
  template <uint64_t conf, bool is_default_size_type, typename T,
            typename... Args>
  constexpr void STRUCT_PACK_INLINE serialize_metainfo() {
    constexpr auto hash_head = calculate_hash_head<conf, T, Args...>() |
                               (is_default_size_type ? 0 : 1);
    if constexpr (!check_if_disable_hash_head<conf, serialize_type>()) {
      write_wrapper<sizeof(uint32_t)>(writer_, (char *)&hash_head);
    }
    if constexpr (hash_head % 2) {  // has more metainfo
      auto metainfo = info.metainfo();
      write_wrapper<sizeof(char)>(writer_, (char *)&metainfo);
      if constexpr (serialize_static_config<serialize_type>::has_compatible) {
        uint16_t len16;
        uint32_t len32;
        uint64_t len64;
        switch (metainfo & 0b11) {
          case 1:
            len16 = info.size();
            write_wrapper<2>(writer_, (char *)&len16);
            break;
          case 2:
            len32 = info.size();
            write_wrapper<4>(writer_, (char *)&len32);
            break;
          case 3:
            len64 = info.size();
            write_wrapper<8>(writer_, (char *)&len64);
            break;
          default:
            unreachable();
        }
      }
      if constexpr (check_if_add_type_literal<conf, serialize_type>()) {
        constexpr auto type_literal =
            struct_pack::get_type_literal<T, Args...>();
        write_bytes_array(writer_, type_literal.data(),
                          type_literal.size() + 1);
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
      for (std::size_t i = 0; i < sz; ++i) write_wrapper<1>(writer_, &buf);
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
        write_wrapper<sizeof(item)>(writer_, (char *)&item);
      }
      else if constexpr (id == type_id::bitset_t) {
        write_bytes_array(writer_, (char *)&item, sizeof(item));
      }
      else if constexpr (unique_ptr<type>) {
        bool has_value = (item != nullptr);
        write_wrapper<sizeof(char)>(writer_, (char *)&has_value);
        if (has_value) {
          if constexpr (is_base_class<typename type::element_type>) {
            bool is_ok;
            uint32_t id = item->get_struct_pack_id();
            auto index = search_type_by_md5<typename type::element_type>(
                item->get_struct_pack_id(), is_ok);
            assert(is_ok);
            write_wrapper<sizeof(uint32_t)>(writer_, (char *)&id);
            template_switch<serialize_one_derived_class_helper<
                derived_class_set_t<typename type::element_type>,
                std::integral_constant<std::size_t, size_type>,
                std::integral_constant<std::uint64_t, version>>>(index, this,
                                                                 item.get());
          }
          else {
            serialize_one<size_type, version>(*item);
          }
        }
      }
      else if constexpr (detail::varint_t<type>) {
        detail::serialize_varint(writer_, item);
      }
      else if constexpr (id == type_id::array_t) {
        if constexpr (is_trivial_serializable<type>::value &&
                      is_little_endian_copyable<sizeof(item[0])>) {
          write_bytes_array(writer_, (char *)&item, sizeof(type));
        }
        else {
          for (const auto &i : item) {
            serialize_one<size_type, version>(i);
          }
        }
      }
      else if constexpr (map_container<type> || container<type>) {
        if constexpr (size_type == 1) {
          uint8_t size = item.size();
          write_wrapper<size_type>(writer_, (char *)&size);
        }
#ifdef STRUCT_PACK_OPTIMIZE
        else if constexpr (size_type == 2) {
          uint16_t size = item.size();
          write_wrapper<size_type>(writer_, (char *)&size);
        }
        else if constexpr (size_type == 4) {
          uint32_t size = item.size();
          write_wrapper<size_type>(writer_, (char *)&size);
        }
        else if constexpr (size_type == 8) {
          uint64_t size = item.size();
          write_wrapper<size_type>(writer_, (char *)&size);
        }
        else {
          static_assert(!sizeof(item), "illegal size_type.");
        }
#else
        else {
          uint16_t size16;
          uint32_t size32;
          uint64_t size64;
          switch ((info.metainfo() & 0b11000) >> 3) {
            case 1:
              size16 = item.size();
              write_wrapper<2>(writer_, (char *)&size16);
              break;
            case 2:
              size32 = item.size();
              write_wrapper<4>(writer_, (char *)&size32);
              break;
            case 3:
              size64 = item.size();
              write_wrapper<8>(writer_, (char *)&size64);
              break;
            default:
              unreachable();
          }
        }
#endif
        if constexpr (trivially_copyable_container<type> &&
                      is_little_endian_copyable<sizeof(
                          typename type::value_type)>) {
          using value_type = typename type::value_type;
          write_bytes_array(writer_, (char *)item.data(),
                            item.size() * sizeof(typename type::value_type));
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
        write_wrapper<sizeof(bool)>(writer_, (char *)&has_value);
        if (has_value) {
          serialize_one<size_type, version>(*item);
        }
      }
      else if constexpr (is_variant_v<type>) {
        static_assert(std::variant_size_v<type> < 256,
                      "variant's size is too large");
        uint8_t index = item.index();
        write_wrapper<sizeof(uint8_t)>(writer_, (char *)&index);
        std::visit(
            [this](auto &&e) {
              this->serialize_one<size_type, version>(e);
            },
            item);
      }
      else if constexpr (expected<type>) {
        bool has_value = item.has_value();
        write_wrapper<sizeof(bool)>(writer_, (char *)&has_value);
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
        if constexpr (is_trivial_serializable<type>::value &&
                      is_little_endian_copyable<sizeof(type)>) {
          write_wrapper<sizeof(type)>(writer_, (char *)&item);
        }
        else if constexpr ((is_trivial_serializable<type>::value &&
                            !is_little_endian_copyable<sizeof(type)>) ||
                           is_trivial_serializable<type, true>::value) {
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
          write_wrapper<sizeof(bool)>(writer_, (char *)&has_value);
          if (has_value) {
            serialize_one<size_type, UINT64_MAX>(*item);
          }
        }
      }
      else if constexpr (unique_ptr<type>) {
        if (item != nullptr) {
          if constexpr (is_base_class<typename type::element_type>) {
            bool is_ok;
            auto index = search_type_by_md5<typename type::element_type>(
                item->get_struct_pack_id(), is_ok);
            assert(is_ok);
            template_switch<serialize_one_derived_class_helper<
                derived_class_set_t<typename type::element_type>,
                std::integral_constant<std::size_t, size_type>,
                std::integral_constant<std::uint64_t, version>>>(index, this,
                                                                 item.get());
          }
          else {
            serialize_one<size_type, version>(*item);
          }
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
};

template <uint64_t conf = sp_config::DEFAULT,
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
}  // namespace struct_pack::detail