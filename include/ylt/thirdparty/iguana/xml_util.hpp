#pragma once

#include <math.h>

#include <bit>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "define.h"
#include "reflection.hpp"

namespace iguana {

template <class T>
concept char_t = std::same_as < std::decay_t<T>,
char > || std::same_as<std::decay_t<T>, char16_t> ||
    std::same_as<std::decay_t<T>, char32_t> ||
    std::same_as<std::decay_t<T>, wchar_t>;

template <class T>
concept bool_t = std::same_as < std::decay_t<T>,
bool > || std::same_as<std::decay_t<T>, std::vector<bool>::reference>;

template <class T>
concept int_t =
    std::integral<std::decay_t<T>> && !char_t<std::decay_t<T>> && !bool_t<T>;

template <class T>
concept float_t = std::floating_point<std::decay_t<T>>;

template <class T>
concept num_t = std::floating_point<std::decay_t<T>> || int_t<T>;

template <class T>
concept enum_t = std::is_enum_v<std::decay_t<T>>;

template <typename T>
constexpr inline bool is_basic_string_view = false;

template <typename T>
constexpr inline bool is_basic_string_view<std::basic_string_view<T>> = true;

template <typename T>
concept str_view_t = is_basic_string_view<std::remove_reference_t<T>>;

template <class T>
concept str_t =
    std::convertible_to<std::decay_t<T>, std::string_view> && !str_view_t<T>;

template <class T>
concept string_t = std::convertible_to<std::decay_t<T>, std::string_view>;

template <class T>
concept tuple_t = is_tuple<std::remove_cvref_t<T>>::value;

template <class T>
concept sequence_container_t =
    is_sequence_container<std::remove_cvref_t<T>>::value;

template <typename Type>
concept unique_ptr_t = requires(Type ptr) {
  ptr.operator*();
  typename std::remove_cvref_t<Type>::element_type;
}
&&!requires(Type ptr, Type ptr2) { ptr = ptr2; };

template <typename Type>
concept optional_t = requires(Type optional) {
  optional.value();
  optional.has_value();
  optional.operator*();
  typename std::remove_cvref_t<Type>::value_type;
};

template <typename Type>
concept container = requires(Type container) {
  typename std::remove_cvref_t<Type>::value_type;
  container.size();
  container.begin();
  container.end();
};
template <typename Type>
concept map_container = container<Type> && requires(Type container) {
  typename std::remove_cvref_t<Type>::mapped_type;
};

template <class T>
concept plain_t =
    string_t<T> || num_t<T> || char_t<T> || bool_t<T> || enum_t<T>;

template <class T>
concept c_array = std::is_array_v<std::remove_cvref_t<T>> &&
                  std::extent_v<std::remove_cvref_t<T>> >
0;

template <typename Type>
concept array = requires(Type arr) {
  arr.size();
  std::tuple_size<std::remove_cvref_t<Type>>{};
};

template <typename Type>
concept tuple = !array<Type> && requires(Type tuple) {
  std::get<0>(tuple);
  sizeof(std::tuple_size<std::remove_cvref_t<Type>>);
};

template <class T>
concept non_refletable = container<T> || c_array<T> || tuple<T> ||
    optional_t<T> || unique_ptr_t<T> || std::is_fundamental_v<T>;

template <typename T, typename map_type = std::unordered_map<std::string_view,
                                                             std::string_view>>
class xml_attr_t {
 public:
  T &value() { return val_; }
  map_type &attr() { return attr_; }
  const T &value() const { return val_; }
  const map_type &attr() const { return attr_; }
  using value_type = std::remove_cvref_t<T>;

 private:
  T val_;
  map_type attr_;
};

template <string_t T = std::string_view>
class xml_cdata_t {
 public:
  T &value() { return val_; }
  const T &value() const { return val_; }
  using value_type = std::remove_cvref_t<T>;

 private:
  T val_;
};

template <typename T>
constexpr inline bool is_attr_t_v = false;

template <typename T, typename map_type>
constexpr inline bool is_attr_t_v<xml_attr_t<T, map_type>> = true;

template <typename T>
concept attr_t = is_attr_t_v<std::decay_t<T>>;

template <typename>
struct is_cdata_t : std::false_type {};

template <typename T>
struct is_cdata_t<xml_cdata_t<T>> : std::true_type {};

template <typename T>
concept cdata_t = is_cdata_t<std::decay_t<T>>::value;

template <size_t N>
struct string_literal {
  static constexpr size_t size = (N > 0) ? (N - 1) : 0;

  constexpr string_literal() = default;

  constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, value); }

  char value[N];
  constexpr const char *end() const noexcept { return value + size; }

  constexpr const std::string_view sv() const noexcept { return {value, size}; }
};

inline constexpr auto has_zero = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return (((chunk - 0x0101010101010101) & ~chunk) & 0x8080808080808080);
};

inline constexpr auto has_greater = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return has_zero(
      chunk ^
      0b0011111000111110001111100011111000111110001111100011111000111110);
};

inline constexpr auto has_space = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return has_zero(
      chunk ^
      0b0010000000100000001000000010000000100000001000000010000000100000);
};

inline constexpr auto has_smaller = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return has_zero(
      chunk ^
      0b0011110000111100001111000011110000111100001111000011110000111100);
};

inline constexpr auto has_square_bracket =
    [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
      return has_zero(
          chunk ^
          0b0101110101011101010111010101110101011101010111010101110101011101);
    };

inline constexpr auto has_equal = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return has_zero(
      chunk ^
      0b0011110100111101001111010011110100111101001111010011110100111101);
};

inline constexpr auto has_qoute = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return has_zero(
      chunk ^
      0b0010001000100010001000100010001000100010001000100010001000100010);
};

IGUANA_INLINE void skip_sapces_and_newline(auto &&it, auto &&end) {
  while (it != end && (*it < 33)) {
    ++it;
  }
}

// match c and skip
template <char c>
IGUANA_INLINE void match(auto &&it, auto &&end) {
  if (it == end || *it != c) [[unlikely]] {
    static constexpr char b[] = {c, '\0'};
    std::string error = std::string("Expected:").append(b);
    throw std::runtime_error(error);
  }
  else [[likely]] {
    ++it;
  }
}

template <string_literal str>
IGUANA_INLINE void match(auto &&it, auto &&end) {
  const auto n = static_cast<size_t>(std::distance(it, end));
  if (n < str.size) [[unlikely]] {
    // TODO: compile time generate this message, currently borken with
    // MSVC
    static constexpr auto error = "Unexpected end of buffer. Expected:";
    throw std::runtime_error(error);
  }
  size_t i{};
  // clang and gcc will vectorize this loop
  for (auto *c = str.value; c < str.end(); ++it, ++c) {
    i += *it != *c;
  }
  if (i != 0) [[unlikely]] {
    // TODO: compile time generate this message, currently borken with
    // MSVC
    static constexpr auto error = "Expected: ";
    throw std::runtime_error(error);
  }
}

IGUANA_INLINE void match_close_tag(auto &&it, auto &&end,
                                   std::string_view key) {
  if (it == end || (*it++) != '/') [[unlikely]] {
    throw std::runtime_error("unclosed tag: " + std::string(key));
  }
  size_t size = key.size();
  if (static_cast<size_t>(std::distance(it, end)) <= size ||
      std::string_view{&*it, size} != key) [[unlikely]] {
    throw std::runtime_error("unclosed tag: " + std::string(key));
  }
  it += size;
  match<'>'>(it, end);

  // skip_till<'>'>(it, end); // not check
  // ++it;
}

template <char c>
IGUANA_INLINE void skip_till(auto &&it, auto &&end) {
  static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);

  if (std::distance(it, end) >= 7) [[likely]] {
    const auto end_m7 = end - 7;
    for (; it < end_m7; it += 8) {
      const auto chunk = *reinterpret_cast<const uint64_t *>(&*it);
      uint64_t test;
      if constexpr (c == '>')
        test = has_greater(chunk);
      else if constexpr (c == '<')
        test = has_smaller(chunk);
      else if constexpr (c == '"')
        test = has_qoute(chunk);
      else if constexpr (c == ' ')
        test = has_space(chunk);
      else if constexpr (c == ']')
        test = has_square_bracket(chunk);
      else if constexpr (c == '=')
        test = has_equal(chunk);
      else
        static_assert(!c, "not support this character");
      if (test != 0) {
        it += (std::countr_zero(test) >> 3);
        return;
      }
    }
  }

  // Tail end of buffer. Should be rare we even get here
  while (it < end) {
    if (*it == c)
      return;
    ++it;
  }
  static constexpr char b[] = {c, '\0'};
  std::string error = std::string("Expected: ").append(b);
  throw std::runtime_error(error);
}

// skip_till<'>', '<'>(it, end);
IGUANA_INLINE void skip_till_greater_or_space(auto &&it, auto &&end) {
  static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);

  if (std::distance(it, end) >= 7) [[likely]] {
    const auto end_m7 = end - 7;
    for (; it < end_m7; it += 8) {
      const auto chunk = *reinterpret_cast<const uint64_t *>(&*it);
      uint64_t test = has_greater(chunk) | has_space(chunk);
      if (test != 0) {
        it += (std::countr_zero(test) >> 3);
        return;
      }
    }
  }

  // Tail end of buffer. Should be rare we even get here
  while (it < end) {
    switch (*it) {
      case '>':
      case ' ':
        return;
    }
    ++it;
  }
  throw std::runtime_error("Expected > or space");
}

template <char c>
IGUANA_INLINE auto skip_pass(auto &&it, auto &&end) {
  skip_till<c>(it, end);
  auto res = it++ - 1;
  while (*res == ' ') {
    --res;
  }
  return res + 1;
}

}  // namespace iguana
