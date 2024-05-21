#pragma once
#include "detail/string_resize.hpp"
#include "pb_util.hpp"

namespace iguana {
namespace detail {

template <uint32_t key, typename V, typename It>
IGUANA_INLINE void encode_varint_field(V val, It&& it) {
  static_assert(std::is_integral_v<V>, "must be integral");
  if constexpr (key != 0) {
    serialize_varint_u32_constexpr<key>(it);
  }
  serialize_varint(val, it);
}

template <uint32_t key, typename V, typename It>
IGUANA_INLINE void encode_fixed_field(V val, It&& it) {
  if constexpr (key != 0) {
    serialize_varint_u32_constexpr<key>(it);
  }
  constexpr size_t size = sizeof(V);
  // TODO: check Stream continuous
  memcpy(it, &val, size);
  it += size;
}

template <uint32_t key, bool omit_default_val = true, typename Type,
          typename It>
IGUANA_INLINE void to_pb_impl(Type&& t, It&& it);

template <uint32_t key, typename V, typename It>
IGUANA_INLINE void encode_pair_value(V&& val, It&& it, size_t size) {
  if (size == 0)
    IGUANA_UNLIKELY {
      // map keys can't be omitted even if values are empty
      // TODO: repeated ?
      serialize_varint_u32_constexpr<key>(it);
      serialize_varint(0, it);
    }
  else {
    to_pb_impl<key, false>(val, it);
  }
}

template <uint32_t key, bool omit_default_val, typename T, typename It>
IGUANA_INLINE void encode_numeric_field(T t, It&& it) {
  if constexpr (omit_default_val) {
    if constexpr (is_fixed_v<T> || is_signed_varint_v<T>) {
      if (t.val == 0) {
        return;
      }
    }
    else {
      if (t == static_cast<T>(0))
        IGUANA_UNLIKELY { return; }
    }
  }
  if constexpr (std::is_integral_v<T>) {
    detail::encode_varint_field<key>(t, it);
  }
  else if constexpr (detail::is_signed_varint_v<T>) {
    detail::encode_varint_field<key>(encode_zigzag(t.val), it);
  }
  else if constexpr (detail::is_fixed_v<T>) {
    detail::encode_fixed_field<key>(t.val, it);
  }
  else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
    detail::encode_fixed_field<key>(t, it);
  }
  else if constexpr (std::is_enum_v<T>) {
    using U = std::underlying_type_t<T>;
    detail::encode_varint_field<key>(static_cast<U>(t), it);
  }
  else {
    static_assert(!sizeof(T), "unsupported type");
  }
}

template <uint32_t field_no, typename Type, typename It>
IGUANA_INLINE void to_pb_oneof(Type&& t, It&& it) {
  using T = std::decay_t<Type>;
  std::visit(
      [&it](auto&& value) IGUANA__INLINE_LAMBDA {
        using value_type =
            std::remove_const_t<std::remove_reference_t<decltype(value)>>;
        constexpr auto offset =
            get_variant_index<T, value_type, std::variant_size_v<T> - 1>();
        constexpr uint32_t key =
            ((field_no + offset) << 3) |
            static_cast<uint32_t>(get_wire_type<value_type>());
        to_pb_impl<key, false>(std::forward<value_type>(value), it);
      },
      std::forward<Type>(t));
}

// omit_default_val = true indicates to omit the default value in searlization
template <uint32_t key, bool omit_default_val, typename Type, typename It>
IGUANA_INLINE void to_pb_impl(Type&& t, It&& it) {
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  if constexpr (is_reflection_v<T> || is_custom_reflection_v<T>) {
    // TODO: improve the key serialize
    auto len = pb_value_size(t);
    // can't be omitted even if values are empty
    if constexpr (key != 0) {
      serialize_varint_u32_constexpr<key>(it);
      serialize_varint(len, it);
      if (len == 0)
        IGUANA_UNLIKELY { return; }
    }
    static constexpr auto tuple = get_members_tuple<T>();
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
    for_each_n(
        [&t, &it](auto i) IGUANA__INLINE_LAMBDA {
          using field_type =
              std::tuple_element_t<decltype(i)::value,
                                   std::decay_t<decltype(tuple)>>;
          constexpr auto value = std::get<decltype(i)::value>(tuple);
          auto& val = value.value(t);

          using U = typename field_type::value_type;
          if constexpr (variant_v<U>) {
            constexpr auto offset =
                get_variant_index<U, typename field_type::sub_type,
                                  std::variant_size_v<U> - 1>();
            if constexpr (offset == 0) {
              to_pb_oneof<value.field_no>(val, it);
            }
          }
          else {
            constexpr uint32_t sub_key =
                (value.field_no << 3) |
                static_cast<uint32_t>(get_wire_type<U>());
            to_pb_impl<sub_key>(val, it);
          }
        },
        std::make_index_sequence<SIZE>{});
  }
  else if constexpr (is_sequence_container<T>::value) {
    // TODO support std::array
    // repeated values can't be omitted even if values are empty
    using item_type = typename T::value_type;
    if constexpr (is_lenprefix_v<item_type>) {
      // non-packed
      for (auto& item : t) {
        to_pb_impl<key, false>(item, it);
      }
    }
    else {
      if (t.empty())
        IGUANA_UNLIKELY { return; }
      serialize_varint_u32_constexpr<key>(it);
      serialize_varint(pb_value_size(t), it);
      for (auto& item : t) {
        encode_numeric_field<false, 0>(item, it);
      }
    }
  }
  else if constexpr (is_map_container<T>::value) {
    using first_type = typename T::key_type;
    using second_type = typename T::mapped_type;
    constexpr uint32_t key1 =
        (1 << 3) | static_cast<uint32_t>(get_wire_type<first_type>());
    constexpr auto key1_size = variant_uint32_size_constexpr(key1);
    constexpr uint32_t key2 =
        (2 << 3) | static_cast<uint32_t>(get_wire_type<second_type>());
    constexpr auto key2_size = variant_uint32_size_constexpr(key2);

    for (auto& [k, v] : t) {
      serialize_varint_u32_constexpr<key>(it);
      auto k_len = pb_value_size(k);
      auto v_len = pb_value_size(v);
      auto pair_len = key1_size + key2_size + k_len + v_len;
      if constexpr (is_lenprefix_v<first_type>) {
        pair_len += variant_uint32_size(k_len);
      }
      if constexpr (is_lenprefix_v<second_type>) {
        pair_len += variant_uint32_size(v_len);
      }
      serialize_varint(pair_len, it);
      // map k and v can't be omitted even if values are empty
      encode_pair_value<key1>(k, it, k_len);
      encode_pair_value<key2>(v, it, v_len);
    }
  }
  else if constexpr (optional_v<T>) {
    if (!t.has_value()) {
      return;
    }
    to_pb_impl<key, omit_default_val>(*t, it);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    if constexpr (omit_default_val) {
      if (t.size() == 0)
        IGUANA_UNLIKELY { return; }
    }
    serialize_varint_u32_constexpr<key>(it);
    serialize_varint(t.size(), it);
    memcpy(it, t.data(), t.size());
    it += t.size();
  }
  else {
    encode_numeric_field<key, omit_default_val>(t, it);
  }
}
}  // namespace detail

template <typename T, typename Stream>
IGUANA_INLINE void to_pb(T& t, Stream& out) {
  auto byte_len = detail::pb_key_value_size<0>(t);
  detail::resize(out, byte_len);
  detail::to_pb_impl<0>(t, &out[0]);
}
}  // namespace iguana
