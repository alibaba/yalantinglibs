// Glaze Library
// For the license information refer to glaze.hpp

#pragma once

#include <math.h>

#include <bit>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "define.h"
#include "reflection.hpp"
#include "value.hpp"

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

template <class T>
concept arithmetic_t = std::is_arithmetic_v<std::decay_t<T>>;

template <typename T>
constexpr inline bool is_basic_string_view = false;

template <typename T>
constexpr inline bool is_basic_string_view<std::basic_string_view<T>> = true;

template <typename Type>
constexpr inline bool is_std_vector_v = false;

template <typename... args>
constexpr inline bool is_std_vector_v<std::vector<args...>> = true;

template <typename Type>
concept vector_container = is_std_vector_v<std::remove_reference_t<Type>>;

template <typename Type>
concept optional = requires(Type optional) {
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
concept c_array = std::is_array_v<std::remove_cvref_t<T>> &&
                  std::extent_v<std::remove_cvref_t<T>> >
0;

template <typename Type>
concept array = requires(Type arr) {
  arr.size();
  std::tuple_size<std::remove_cvref_t<Type>>{};
};

template <typename Type>
concept fixed_array = c_array<Type> || array<Type>;

template <typename T>
concept str_view_t = is_basic_string_view<std::remove_reference_t<T>>;

// eliminate char a[]
template <class T>
concept str_t =
    std::convertible_to<std::decay_t<T>, std::string_view> && !str_view_t<T> &&
    !c_array<T>;

template <typename Type>
concept tuple = !array<Type> && requires(Type tuple) {
  std::get<0>(tuple);
  sizeof(std::tuple_size<std::remove_cvref_t<Type>>);
};

template <typename Type>
concept json_view = requires(Type container) {
  container.size();
  container.begin();
  container.end();
};

template <typename T>
concept json_byte = std::is_same_v<char, T> ||
    std::is_same_v<unsigned char, T> || std::is_same_v<std::byte, T>;

template <typename Type>
constexpr inline bool is_std_list_v = false;
template <typename... args>
constexpr inline bool is_std_list_v<std::list<args...>> = true;

template <typename Type>
constexpr inline bool is_std_deque_v = false;
template <typename... args>
constexpr inline bool is_std_deque_v<std::deque<args...>> = true;

template <typename Type>
concept sequence_container = is_std_list_v<std::remove_reference_t<Type>> ||
    is_std_vector_v<std::remove_reference_t<Type>> ||
    is_std_deque_v<std::remove_reference_t<Type>>;

template <class T>
concept associat_container_t =
    is_associat_container<std::remove_cvref_t<T>>::value;

template <class T>
concept sequence_container_t =
    is_sequence_container<std::remove_cvref_t<T>>::value;

template <class T>
concept tuple_t = is_tuple<std::remove_cvref_t<T>>::value;

template <class T>
concept string_container_t = str_t<T> || str_view_t<T>;

template <typename Type>
concept unique_ptr_t = requires(Type ptr) {
  ptr.operator*();
  typename std::remove_cvref_t<Type>::element_type;
}
&&!requires(Type ptr, Type ptr2) { ptr = ptr2; };

template <class T>
concept non_refletable = container<T> || c_array<T> || tuple<T> ||
    optional<T> || unique_ptr_t<T> || std::is_fundamental_v<T>;

template <size_t N>
struct string_literal {
  static constexpr size_t size = (N > 0) ? (N - 1) : 0;

  constexpr string_literal() = default;

  constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, value); }

  char value[N];
  constexpr const char *end() const noexcept { return value + size; }

  constexpr const std::string_view sv() const noexcept { return {value, size}; }
};

template <char c>
IGUANA_INLINE void match(auto &&it, auto &&end) {
  if (it == end || *it != c) [[unlikely]] {
    static constexpr char b[] = {c, '\0'};
    //         static constexpr auto error = concat_arrays("Expected:", b);
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

IGUANA_INLINE void skip_comment(auto &&it, auto &&end) {
  ++it;
  if (it == end) [[unlikely]]
    throw std::runtime_error("Unexpected end, expected comment");
  else if (*it == '/') {
    while (++it != end && *it != '\n')
      ;
  }
  else if (*it == '*') {
    while (++it != end) {
      if (*it == '*') [[unlikely]] {
        if (++it == end) [[unlikely]]
          break;
        else if (*it == '/') [[likely]] {
          ++it;
          break;
        }
      }
    }
  }
  else [[unlikely]]
    throw std::runtime_error("Expected / or * after /");
}

IGUANA_INLINE void skip_ws(auto &&it, auto &&end) {
  while (it != end) {
    // assuming ascii
    if (static_cast<uint8_t>(*it) < 33) {
      ++it;
    }
    else if (*it == '/') {
      skip_comment(it, end);
    }
    else {
      break;
    }
  }
}

IGUANA_INLINE void skip_ws_no_comments(auto &&it, auto &&end) {
  while (it != end) {
    // assuming ascii
    if (static_cast<uint8_t>(*it) < 33) [[likely]] {
      ++it;
    }
    else {
      break;
    }
  }
}

IGUANA_INLINE void skip_till_escape_or_qoute(auto &&it, auto &&end) {
  static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);

  auto has_zero = [](uint64_t chunk) {
    return (((chunk - 0x0101010101010101) & ~chunk) & 0x8080808080808080);
  };

  auto has_qoute = [&](uint64_t chunk) IGUANA__INLINE_LAMBDA {
    return has_zero(
        chunk ^
        0b0010001000100010001000100010001000100010001000100010001000100010);
  };

  auto has_escape = [&](uint64_t chunk) IGUANA__INLINE_LAMBDA {
    return has_zero(
        chunk ^
        0b0101110001011100010111000101110001011100010111000101110001011100);
  };

  if (std::distance(it, end) >= 7) [[likely]] {
    const auto end_m7 = end - 7;
    for (; it < end_m7; it += 8) {
      const auto chunk = *reinterpret_cast<const uint64_t *>(&*it);
      uint64_t test = has_qoute(chunk) | has_escape(chunk);
      if (test != 0) {
        it += (std::countr_zero(test) >> 3);
        return;
      }
    }
  }

  // Tail end of buffer. Should be rare we even get here
  while (it < end) {
    switch (*it) {
      case '\\':
      case '"':
        return;
    }
    ++it;
  }
  throw std::runtime_error("Expected \"");
}

IGUANA_INLINE void skip_string(auto &&it, auto &&end) noexcept {
  ++it;
  while (it < end) {
    if (*it == '"') {
      ++it;
      break;
    }
    else if (*it == '\\' && ++it == end) [[unlikely]]
      break;
    ++it;
  }
}

template <char open, char close>
IGUANA_INLINE void skip_until_closed(auto &&it, auto &&end) {
  ++it;
  size_t open_count = 1;
  size_t close_count = 0;
  while (it < end && open_count > close_count) {
    switch (*it) {
      case '/':
        skip_comment(it, end);
        break;
      case '"':
        skip_string(it, end);
        break;
      case open:
        ++open_count;
        ++it;
        break;
      case close:
        ++close_count;
        ++it;
        break;
      default:
        ++it;
    }
  }
}

IGUANA_INLINE constexpr bool is_numeric(const auto c) noexcept {
  switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':  //
    case '4':
    case '5':
    case '6':
    case '7':  //
    case '8':
    case '9':  //
    case '.':
    case '+':
    case '-':  //
    case 'e':
    case 'E':  //
      return true;
  }
  return false;
}

constexpr bool is_digit(char c) { return c <= '9' && c >= '0'; }

constexpr size_t stoui(std::string_view s, size_t value = 0) {
  if (s.empty()) {
    return value;
  }

  else if (is_digit(s[0])) {
    return stoui(s.substr(1), (s[0] - '0') + value * 10);
  }

  else {
    throw std::runtime_error("not a digit");
  }
}
}  // namespace iguana
