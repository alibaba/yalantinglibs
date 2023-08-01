#pragma once
#include "util.hpp"

namespace iguana {
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

template <typename T = std::string_view,
          std::enable_if_t<string_container_v<T>, int> = 0>
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
constexpr inline bool attr_v = is_attr_t_v<std::remove_cvref_t<T>>;

template <typename>
struct is_cdata_t : std::false_type {};

template <typename T>
struct is_cdata_t<xml_cdata_t<T>> : std::true_type {};

template <typename T>
constexpr inline bool cdata_v = is_cdata_t<std::remove_cvref_t<T>>::value;

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

template <typename It>
IGUANA_INLINE void skip_sapces_and_newline(It &&it, It &&end) {
  while (it != end && (*it < 33)) {
    ++it;
  }
}

template <typename It>
IGUANA_INLINE void match_close_tag(It &&it, It &&end, std::string_view key) {
  if (it == end || (*it++) != '/')
    IGUANA_UNLIKELY {
      throw std::runtime_error("unclosed tag: " + std::string(key));
    }
  size_t size = key.size();
  if (static_cast<size_t>(std::distance(it, end)) <= size ||
      std::string_view{&*it, size} != key)
    IGUANA_UNLIKELY {
      throw std::runtime_error("unclosed tag: " + std::string(key));
    }
  it += size;
  match<'>'>(it, end);

  // skip_till<'>'>(it, end); // not check
  // ++it;
}

template <char c, typename It>
IGUANA_INLINE void skip_till(It &&it, It &&end) {
  static_assert(contiguous_iterator<std::decay_t<It>>);

  if (std::distance(it, end) >= 7)
    IGUANA_LIKELY {
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
          it += (countr_zero(test) >> 3);
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
template <typename It>
IGUANA_INLINE void skip_till_greater_or_space(It &&it, It &&end) {
  static_assert(contiguous_iterator<std::decay_t<It>>);

  if (std::distance(it, end) >= 7)
    IGUANA_LIKELY {
      const auto end_m7 = end - 7;
      for (; it < end_m7; it += 8) {
        const auto chunk = *reinterpret_cast<const uint64_t *>(&*it);
        uint64_t test = has_greater(chunk) | has_space(chunk);
        if (test != 0) {
          it += (countr_zero(test) >> 3);
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

template <char c, typename It>
IGUANA_INLINE auto skip_pass(It &&it, It &&end) {
  skip_till<c>(it, end);
  auto res = it++ - 1;
  while (*res == ' ') {
    --res;
  }
  return res + 1;
}

}  // namespace iguana
