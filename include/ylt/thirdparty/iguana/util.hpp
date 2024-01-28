#pragma once

#include <math.h>

#include <filesystem>
#include <forward_list>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "define.h"
#include "detail/charconv.h"
#include "detail/utf.hpp"
#include "enum_reflection.hpp"
#include "error_code.h"
#include "reflection.hpp"

namespace iguana {

template <typename T>
inline constexpr bool char_v = std::is_same_v<std::decay_t<T>, char> ||
                               std::is_same_v<std::decay_t<T>, char16_t> ||
                               std::is_same_v<std::decay_t<T>, char32_t> ||
                               std::is_same_v<std::decay_t<T>, wchar_t>;

template <typename T>
inline constexpr bool bool_v =
    std::is_same_v<std::decay_t<T>, bool> ||
    std::is_same_v<std::decay_t<T>, std::vector<bool>::reference>;

template <typename T>
inline constexpr bool int_v = std::is_integral_v<std::decay_t<T>> &&
                              !char_v<std::decay_t<T>> && !bool_v<T>;

template <typename T>
inline constexpr bool float_v = std::is_floating_point_v<std::decay_t<T>>;

template <typename T>
inline constexpr bool num_v = float_v<T> || int_v<T>;

template <typename T>
inline constexpr bool enum_v = std::is_enum_v<std::decay_t<T>>;

template <typename T>
constexpr inline bool optional_v =
    is_template_instant_of<std::optional, std::remove_cvref_t<T>>::value;

template <class, class = void>
struct is_container : std::false_type {};

template <class T>
struct is_container<
    T, std::void_t<decltype(std::declval<T>().size(), std::declval<T>().begin(),
                            std::declval<T>().end())>> : std::true_type {};

template <class, class = void>
struct is_map_container : std::false_type {};

template <class T>
struct is_map_container<
    T, std::void_t<decltype(std::declval<typename T::mapped_type>())>>
    : is_container<T> {};

template <typename T>
constexpr inline bool container_v = is_container<std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool map_container_v =
    is_map_container<std::remove_cvref_t<T>>::value;

template <class T>
constexpr inline bool c_array_v = std::is_array_v<std::remove_cvref_t<T>>&&
                                      std::extent_v<std::remove_cvref_t<T>> > 0;

template <typename Type, typename = void>
struct is_array : std::false_type {};

template <typename T>
struct is_array<
    T, std::void_t<decltype(std::declval<T>().size()),
                   typename std::enable_if_t<(std::tuple_size<T>::value != 0)>>>
    : std::true_type {};

template <typename T>
constexpr inline bool array_v = is_array<std::remove_cvref_t<T>>::value;

template <typename Type>
constexpr inline bool fixed_array_v = c_array_v<Type> || array_v<Type>;

template <typename T>
constexpr inline bool string_view_v =
    is_template_instant_of<std::basic_string_view,
                           std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool string_v =
    is_template_instant_of<std::basic_string, std::remove_cvref_t<T>>::value;

// TODO: type must be char
template <typename T>
constexpr inline bool json_view_v = container_v<T>;

template <typename T>
constexpr inline bool json_byte_v =
    std::is_same_v<char, std::remove_cvref_t<T>> ||
    std::is_same_v<unsigned char, std::remove_cvref_t<T>> ||
    std::is_same_v<std::byte, std::remove_cvref_t<T>>;

template <typename T>
constexpr inline bool sequence_container_v =
    is_sequence_container<std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool tuple_v = is_tuple<std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool string_container_v = string_v<T> || string_view_v<T>;

template <typename T>
constexpr inline bool unique_ptr_v =
    is_template_instant_of<std::unique_ptr, std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool shared_ptr_v =
    is_template_instant_of<std::shared_ptr, std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool smart_ptr_v = shared_ptr_v<T> || unique_ptr_v<T>;

template <typename T>
struct is_variant : std::false_type {};

template <typename... T>
struct is_variant<std::variant<T...>> : std::true_type {};

template <typename T>
constexpr inline bool variant_v = is_variant<std::remove_cvref_t<T>>::value;

template <class T>
constexpr inline bool non_refletable_v =
    container_v<T> || c_array_v<T> || tuple_v<T> || optional_v<T> ||
    smart_ptr_v<T> || std::is_fundamental_v<std::remove_cvref_t<T>> ||
    variant_v<T>;

template <typename T>
constexpr inline bool refletable_v = is_reflection_v<std::remove_cvref_t<T>>;

template <typename T>
constexpr inline bool plain_v =
    string_container_v<T> || num_v<T> || char_v<T> || bool_v<T> || enum_v<T>;

template <typename T>
struct underline_type {
  using type = T;
};

template <typename T>
struct underline_type<std::unique_ptr<T>> {
  using type = typename underline_type<T>::type;
};

template <typename T>
struct underline_type<std::shared_ptr<T>> {
  using type = typename underline_type<T>::type;
};

template <typename T>
struct underline_type<std::optional<T>> {
  using type = typename underline_type<T>::type;
};

template <typename T>
using underline_type_t = typename underline_type<std::remove_cvref_t<T>>::type;

template <char... C, typename It>
IGUANA_INLINE void match(It&& it, It&& end) {
  const auto n = static_cast<size_t>(std::distance(it, end));
  if (n < sizeof...(C))
    IGUANA_UNLIKELY {
      // TODO: compile time generate this message, currently borken with
      // MSVC
      static constexpr auto error = "Unexpected end of buffer. Expected:";
      throw std::runtime_error(error);
    }
  if (((... || (*it++ != C))))
    IGUANA_UNLIKELY {
      // TODO: compile time generate this message, currently borken with
      // MSVC
      static constexpr char b[] = {C..., '\0'};
      throw std::runtime_error(std::string("Expected these: ").append(b));
    }
}

inline constexpr auto has_zero = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return (((chunk - 0x0101010101010101) & ~chunk) & 0x8080808080808080);
};

inline constexpr auto has_qoute = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return has_zero(
      chunk ^
      0b0010001000100010001000100010001000100010001000100010001000100010);
};

// https://github.com/Tencent/rapidjson/blob/master/include/rapidjson/writer.h
template <typename Ch, typename SizeType, typename Stream>
inline void write_string_with_escape(const Ch* it, SizeType length,
                                     Stream& ss) {
  static const char hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  static const char escape[256] = {
#define Z16 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
      // 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E
      // F
      'u', 'u', 'u',  'u', 'u', 'u', 'u', 'u', 'b', 't',
      'n', 'u', 'f',  'r', 'u', 'u',  // 00
      'u', 'u', 'u',  'u', 'u', 'u', 'u', 'u', 'u', 'u',
      'u', 'u', 'u',  'u', 'u', 'u',  // 10
      0,   0,   '"',  0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,    0,   0,   0,  // 20
      Z16, Z16,                     // 30~4F
      0,   0,   0,    0,   0,   0,   0,   0,   0,   0,
      0,   0,   '\\', 0,   0,   0,                       // 50
      Z16, Z16, Z16,  Z16, Z16, Z16, Z16, Z16, Z16, Z16  // 60~FF
#undef Z16
  };
  auto end = it;
  std::advance(end, length);
  while (it < end) {
    if (static_cast<unsigned>(*it) >= 0x80)
      IGUANA_UNLIKELY {
        unsigned codepoint = 0;
        if (!decode_utf8(it, codepoint))
          IGUANA_UNLIKELY {
            throw std::runtime_error("illegal unicode character");
          }
        ss.push_back('\\');
        ss.push_back('u');
        if (codepoint <= 0xD7FF ||
            (codepoint >= 0xE000 && codepoint <= 0xFFFF)) {
          ss.push_back(hexDigits[(codepoint >> 12) & 15]);
          ss.push_back(hexDigits[(codepoint >> 8) & 15]);
          ss.push_back(hexDigits[(codepoint >> 4) & 15]);
          ss.push_back(hexDigits[(codepoint)&15]);
        }
        else {
          if (codepoint < 0x010000 || codepoint > 0x10FFFF)
            IGUANA_UNLIKELY { throw std::runtime_error("illegal codepoint"); }
          // Surrogate pair
          unsigned s = codepoint - 0x010000;
          unsigned lead = (s >> 10) + 0xD800;
          unsigned trail = (s & 0x3FF) + 0xDC00;
          ss.push_back(hexDigits[(lead >> 12) & 15]);
          ss.push_back(hexDigits[(lead >> 8) & 15]);
          ss.push_back(hexDigits[(lead >> 4) & 15]);
          ss.push_back(hexDigits[(lead)&15]);
          ss.push_back('\\');
          ss.push_back('u');
          ss.push_back(hexDigits[(trail >> 12) & 15]);
          ss.push_back(hexDigits[(trail >> 8) & 15]);
          ss.push_back(hexDigits[(trail >> 4) & 15]);
          ss.push_back(hexDigits[(trail)&15]);
        }
      }
    else if (escape[static_cast<unsigned char>(*it)])
      IGUANA_UNLIKELY {
        ss.push_back('\\');
        ss.push_back(escape[static_cast<unsigned char>(*it)]);

        if (escape[static_cast<unsigned char>(*it)] == 'u') {
          // escape other control characters
          ss.push_back('0');
          ss.push_back('0');
          ss.push_back(hexDigits[static_cast<unsigned char>(*it) >> 4]);
          ss.push_back(hexDigits[static_cast<unsigned char>(*it) & 0xF]);
        }
        ++it;
      }
    else {
      ss.push_back(*(it++));
    }
  }
}

}  // namespace iguana
