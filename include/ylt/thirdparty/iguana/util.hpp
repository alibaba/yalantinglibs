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
constexpr inline bool c_array_v = std::is_array_v<std::remove_cvref_t<T>> &&
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
IGUANA_INLINE void match(It &&it, It &&end) {
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

}  // namespace iguana
