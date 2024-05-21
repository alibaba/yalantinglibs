#pragma once
#include <cassert>
#include <cstddef>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "detail/pb_type.hpp"
#include "reflection.hpp"
#include "util.hpp"

namespace iguana {

enum class WireType : uint32_t {
  Varint = 0,
  Fixed64 = 1,
  LengthDelimeted = 2,
  StartGroup = 3,
  EndGroup = 4,
  Fixed32 = 5,
  Unknown
};

struct pb_base {
  size_t cache_size;
};

template <typename T>
constexpr bool inherits_from_pb_base_v = std::is_base_of_v<pb_base, T>;

namespace detail {
template <typename T>
constexpr bool is_fixed_v =
    std::is_same_v<T, fixed32_t> || std::is_same_v<T, fixed64_t> ||
    std::is_same_v<T, sfixed32_t> || std::is_same_v<T, sfixed64_t>;

template <typename T>
constexpr bool is_signed_varint_v =
    std::is_same_v<T, sint32_t> || std::is_same_v<T, sint64_t>;

template <typename T>
constexpr inline WireType get_wire_type() {
  if constexpr (std::is_integral_v<T> || is_signed_varint_v<T> ||
                std::is_enum_v<T> || std::is_same_v<T, bool>) {
    return WireType::Varint;
  }
  else if constexpr (std::is_same_v<T, fixed32_t> ||
                     std::is_same_v<T, sfixed32_t> ||
                     std::is_same_v<T, float>) {
    return WireType::Fixed32;
  }
  else if constexpr (std::is_same_v<T, fixed64_t> ||
                     std::is_same_v<T, sfixed64_t> ||
                     std::is_same_v<T, double>) {
    return WireType::Fixed64;
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view> ||
                     is_reflection_v<T> || is_sequence_container<T>::value ||
                     is_map_container<T>::value) {
    return WireType::LengthDelimeted;
  }
  else if constexpr (optional_v<T>) {
    return get_wire_type<typename T::value_type>();
  }
  else {
    throw std::runtime_error("unknown type");
  }
}

template <typename T>
constexpr bool is_lenprefix_v = (get_wire_type<T>() ==
                                 WireType::LengthDelimeted);

[[nodiscard]] IGUANA_INLINE uint32_t encode_zigzag(int32_t v) {
  return (static_cast<uint32_t>(v) << 1U) ^
         static_cast<uint32_t>(
             -static_cast<int32_t>(static_cast<uint32_t>(v) >> 31U));
}
[[nodiscard]] IGUANA_INLINE uint64_t encode_zigzag(int64_t v) {
  return (static_cast<uint64_t>(v) << 1U) ^
         static_cast<uint64_t>(
             -static_cast<int64_t>(static_cast<uint64_t>(v) >> 63U));
}

[[nodiscard]] IGUANA_INLINE int64_t decode_zigzag(uint64_t u) {
  return static_cast<int64_t>((u >> 1U)) ^
         static_cast<uint64_t>(-static_cast<int64_t>(u & 1U));
}
[[nodiscard]] IGUANA_INLINE int64_t decode_zigzag(uint32_t u) {
  return static_cast<int64_t>((u >> 1U)) ^
         static_cast<uint64_t>(-static_cast<int64_t>(u & 1U));
}

template <class T>
IGUANA_INLINE uint64_t decode_varint(T& data, size_t& pos) {
  const int8_t* begin = reinterpret_cast<const int8_t*>(data.data());
  const int8_t* end = begin + data.size();
  const int8_t* p = begin;
  uint64_t val = 0;

  if ((static_cast<uint64_t>(*p) & 0x80) == 0) {
    pos = 1;
    return static_cast<uint64_t>(*p);
  }

  // end is always greater than or equal to begin, so this subtraction is safe
  if (size_t(end - begin) >= 10)
    IGUANA_LIKELY {  // fast path
      int64_t b;
      do {
        b = *p++;
        val = (b & 0x7f);
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x7f) << 7;
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x7f) << 14;
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x7f) << 21;
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x7f) << 28;
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x7f) << 35;
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x7f) << 42;
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x7f) << 49;
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x7f) << 56;
        if (b >= 0) {
          break;
        }
        b = *p++;
        val |= (b & 0x01) << 63;
        if (b >= 0) {
          break;
        }
        throw std::invalid_argument("Invalid varint value: too many bytes.");
      } while (false);
    }
  else {
    int shift = 0;
    while (p != end && *p < 0) {
      val |= static_cast<uint64_t>(*p++ & 0x7f) << shift;
      shift += 7;
    }
    if (p == end)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid varint value: too few bytes.");
      }
    val |= static_cast<uint64_t>(*p++) << shift;
  }

  pos = (p - begin);
  return val;
}

// value == 0 ? 1 : floor(log2(value)) / 7 + 1
constexpr size_t variant_uint32_size_constexpr(uint32_t value) {
  if (value == 0) {
    return 1;
  }
  int log = 0;
  while (value >>= 1) ++log;
  return log / 7 + 1;
}

template <uint64_t v, size_t I, typename It>
IGUANA_INLINE void append_varint_u32_constexpr_help(It&& it) {
  *(it++) = static_cast<uint8_t>((v >> (7 * I)) | 0x80);
}

template <uint64_t v, typename It, size_t... I>
IGUANA_INLINE void append_varint_u32_constexpr(It&& it,
                                               std::index_sequence<I...>) {
  (append_varint_u32_constexpr_help<v, I>(it), ...);
}

template <uint32_t v, typename It>
IGUANA_INLINE void serialize_varint_u32_constexpr(It&& it) {
  constexpr auto size = variant_uint32_size_constexpr(v);
  append_varint_u32_constexpr<v>(it, std::make_index_sequence<size - 1>{});
  *(it++) = static_cast<uint8_t>(v >> (7 * (size - 1)));
}

template <typename It>
IGUANA_INLINE void serialize_varint(uint64_t v, It&& it) {
  if (v < 0x80) {
    *(it++) = static_cast<uint8_t>(v);
    return;
  }
  *(it++) = static_cast<uint8_t>(v | 0x80);
  v >>= 7;
  if (v < 0x80) {
    *(it++) = static_cast<uint8_t>(v);
    return;
  }
  do {
    *(it++) = static_cast<uint8_t>(v | 0x80);
    v >>= 7;
  } while (v >= 0x80);
  *(it++) = static_cast<uint8_t>(v);
}

IGUANA_INLINE uint32_t log2_floor_uint32(uint32_t n) {
#if defined(__GNUC__)
  return 31 ^ static_cast<uint32_t>(__builtin_clz(n));
#else
  unsigned long where;
  _BitScanReverse(&where, n);
  return where;
#endif
}

IGUANA_INLINE size_t variant_uint32_size(uint32_t value) {
  // This computes value == 0 ? 1 : floor(log2(value)) / 7 + 1
  // Use an explicit multiplication to implement the divide of
  // a number in the 1..31 range.
  // Explicit OR 0x1 to avoid calling Bits::Log2FloorNonZero(0), which is
  // undefined.
  uint32_t log2value = log2_floor_uint32(value | 0x1);
  return static_cast<size_t>((log2value * 9 + 73) / 64);
}

IGUANA_INLINE uint32_t log2_floor_uint64(uint64_t n) {
#if defined(__GNUC__)
  return 63 ^ static_cast<uint32_t>(__builtin_clzll(n));
#else
  unsigned long where;
  _BitScanReverse64(&where, n);
  return where;
#endif
}

IGUANA_INLINE size_t variant_uint64_size(uint64_t value) {
  // This computes value == 0 ? 1 : floor(log2(value)) / 7 + 1
  // Use an explicit multiplication to implement the divide of
  // a number in the 1..63 range.
  // Explicit OR 0x1 to avoid calling Bits::Log2FloorNonZero(0), which is
  // undefined.
  uint32_t log2value = log2_floor_uint64(value | 0x1);
  return static_cast<size_t>((log2value * 9 + 73) / 64);
}

template <typename U>
constexpr IGUANA_INLINE size_t variant_intergal_size(U value) {
  using T = std::remove_reference_t<U>;
  if constexpr (sizeof(T) == 8) {
    return variant_uint64_size(static_cast<uint64_t>(value));
  }
  else if constexpr (sizeof(T) <= 4) {
    if constexpr (std::is_signed_v<T>) {
      if (value < 0) {
        return 10;
      }
    }
    return variant_uint32_size(static_cast<uint32_t>(value));
  }
  else {
    static_assert(!sizeof(T), "intergal in not supported");
  }
}

template <typename F, size_t... I>
IGUANA_INLINE constexpr void for_each_n(F&& f, std::index_sequence<I...>) {
  (std::forward<F>(f)(std::integral_constant<size_t, I>{}), ...);
}

// cache the size of reflection type
template <typename T>
IGUANA_INLINE auto& get_set_size_cache(T& t) {
  static std::map<size_t, size_t> cache;
  return cache[reinterpret_cast<size_t>(&t)];
}

template <size_t key_size, bool omit_default_val, typename T>
IGUANA_INLINE size_t numeric_size(T&& t) {
  using value_type = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (omit_default_val) {
    if constexpr (is_fixed_v<value_type> || is_signed_varint_v<value_type>) {
      if (t.val == 0)
        IGUANA_UNLIKELY { return 0; }
    }
    else {
      if (t == static_cast<value_type>(0))
        IGUANA_UNLIKELY { return 0; }
    }
  }
  if constexpr (std::is_integral_v<value_type>) {
    if constexpr (std::is_same_v<bool, value_type>) {
      return 1 + key_size;
    }
    else {
      return key_size + variant_intergal_size(t);
    }
  }
  else if constexpr (detail::is_signed_varint_v<value_type>) {
    return key_size + variant_intergal_size(encode_zigzag(t.val));
  }
  else if constexpr (detail::is_fixed_v<value_type>) {
    return key_size + sizeof(typename value_type::value_type);
  }
  else if constexpr (std::is_same_v<value_type, double> ||
                     std::is_same_v<value_type, float>) {
    return key_size + sizeof(value_type);
  }
  else if constexpr (std::is_enum_v<value_type>) {
    using U = std::underlying_type_t<value_type>;
    return key_size + variant_intergal_size(static_cast<U>(t));
  }
  else {
    static_assert(!sizeof(value_type), "err");
  }
}

template <size_t key_size, bool omit_default_val = true, typename T>
IGUANA_INLINE size_t pb_key_value_size(T&& t);

template <typename Variant, typename T, size_t I>
constexpr inline size_t get_variant_index() {
  if constexpr (I == 0) {
    static_assert(std::is_same_v<std::variant_alternative_t<0, Variant>, T>,
                  "Type T is not found in Variant");
    return 0;
  }
  else if constexpr (std::is_same_v<std::variant_alternative_t<I, Variant>,
                                    T>) {
    return I;
  }
  else {
    return get_variant_index<Variant, T, I - 1>();
  }
}

template <size_t field_no, typename Type>
IGUANA_INLINE size_t pb_oneof_size(Type&& t) {
  using T = std::decay_t<Type>;
  int len = 0;
  std::visit(
      [&len](auto&& value) IGUANA__INLINE_LAMBDA {
        using value_type =
            std::remove_const_t<std::remove_reference_t<decltype(value)>>;
        constexpr auto offset =
            get_variant_index<T, value_type, std::variant_size_v<T> - 1>();
        constexpr uint32_t key =
            ((field_no + offset) << 3) |
            static_cast<uint32_t>(get_wire_type<value_type>());
        len = pb_key_value_size<variant_uint32_size_constexpr(key), false>(
            std::forward<value_type>(value));
      },
      std::forward<Type>(t));
  return len;
}

// returns size = key_size + optional(len_size) + len
// when key_size == 0, return len
template <size_t key_size, bool omit_default_val, typename Type>
IGUANA_INLINE size_t pb_key_value_size(Type&& t) {
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  if constexpr (is_reflection_v<T> || is_custom_reflection_v<T>) {
    size_t len = 0;
    constexpr auto tuple = get_members_tuple<T>();
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
    for_each_n(
        [&len, &t](auto i) IGUANA__INLINE_LAMBDA {
          constexpr auto tuple = get_members_tuple<T>();
          using field_type =
              std::tuple_element_t<decltype(i)::value,
                                   std::decay_t<decltype(tuple)>>;
          constexpr auto value = std::get<decltype(i)::value>(tuple);
          using U = typename field_type::value_type;
          auto& val = value.value(t);
          if constexpr (variant_v<U>) {
            constexpr auto offset =
                get_variant_index<U, typename field_type::sub_type,
                                  std::variant_size_v<U> - 1>();
            if constexpr (offset == 0) {
              len += pb_oneof_size<value.field_no>(val);
            }
          }
          else {
            constexpr uint32_t sub_key =
                (value.field_no << 3) |
                static_cast<uint32_t>(get_wire_type<U>());
            constexpr auto sub_keysize = variant_uint32_size_constexpr(sub_key);
            len += pb_key_value_size<sub_keysize>(val);
          }
        },
        std::make_index_sequence<SIZE>{});
    if constexpr (inherits_from_pb_base_v<T>) {
      t.cache_size = len;
    }
    else {
      get_set_size_cache(t) = len;
    }
    if constexpr (key_size == 0) {
      // for top level
      return len;
    }
    else {
      if (len == 0) {
        // equals key_size  + variant_uint32_size(len)
        return key_size + 1;
      }
      else {
        return key_size + variant_uint32_size(static_cast<uint32_t>(len)) + len;
      }
    }
  }
  else if constexpr (is_sequence_container<T>::value) {
    using item_type = typename T::value_type;
    size_t len = 0;
    if constexpr (is_lenprefix_v<item_type>) {
      for (auto& item : t) {
        len += pb_key_value_size<key_size, false>(item);
      }
      return len;
    }
    else {
      for (auto& item : t) {
        // here 0 to get pakced size
        len += numeric_size<0, false>(item);
      }
      if (len == 0) {
        return 0;
      }
      else {
        return key_size + variant_uint32_size(static_cast<uint32_t>(len)) + len;
      }
    }
  }
  else if constexpr (is_map_container<T>::value) {
    size_t len = 0;
    for (auto& [k, v] : t) {
      // the key_size of  k and v  is constant 1
      size_t kv_len =
          pb_key_value_size<1, false>(k) + pb_key_value_size<1, false>(v);
      len += key_size + variant_uint32_size(static_cast<uint32_t>(kv_len)) +
             kv_len;
    }
    return len;
  }
  else if constexpr (optional_v<T>) {
    if (!t.has_value()) {
      return 0;
    }
    return pb_key_value_size<key_size, omit_default_val>(*t);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    if constexpr (omit_default_val) {
      if (t.size() == 0)
        IGUANA_UNLIKELY { return 0; }
    }
    if constexpr (key_size == 0) {
      return t.size();
    }
    else {
      return key_size + variant_uint32_size(static_cast<uint32_t>(t.size())) +
             t.size();
    }
  }
  else {
    return numeric_size<key_size, omit_default_val>(t);
  }
}

// return the payload size
template <typename T>
IGUANA_INLINE size_t pb_value_size(T&& t) {
  using value_type = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (is_reflection_v<value_type>) {
    if constexpr (inherits_from_pb_base_v<value_type>) {
      return t.cache_size;
    }
    else {
      return get_set_size_cache(t);
    }
  }
  else if constexpr (is_sequence_container<value_type>::value) {
    using item_type = typename value_type::value_type;
    size_t len = 0;
    if constexpr (!is_lenprefix_v<item_type>) {
      for (auto& item : t) {
        len += numeric_size<0, false>(item);
      }
      return len;
    }
    else {
      static_assert(!sizeof(item_type), "the size of this type is meaningless");
    }
  }
  else if constexpr (is_map_container<value_type>::value) {
    static_assert(!sizeof(value_type), "the size of this type is meaningless");
  }
  else if constexpr (optional_v<value_type>) {
    if (!t.has_value()) {
      return 0;
    }
    return pb_value_size(*t);
  }
  else {
    return pb_key_value_size<0>(t);
  }
}

}  // namespace detail
}  // namespace iguana