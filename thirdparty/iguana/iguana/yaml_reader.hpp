#pragma once

#include <charconv>

#include "detail/charconv.h"
#include "detail/utf.hpp"
#include "yaml_util.hpp"

namespace iguana {

template <refletable T, typename It>
void from_yaml(T &value, It &&it, It &&end, size_t min_spaces = 0);

namespace detail {

template <str_t U, class It>
IGUANA_INLINE void parse_escape_str(U &value, It &&it, It &&end) {
  auto start = it;
  while (it != end) {
    if (*(it++) == '\\') [[unlikely]] {
      if (*it == 'u') {
        value.append(&*start,
                     static_cast<size_t>(std::distance(start, it) - 1));
        ++it;
        if (std::distance(it, end) < 4) [[unlikely]]
          throw std::runtime_error(R"(Expected 4 hexadecimal digits)");
        auto code_point = parse_unicode_hex4(it);
        encode_utf8(value, code_point);
        start = it;
      }
      else if (*it == 'n') {
        value.append(&*start,
                     static_cast<size_t>(std::distance(start, it) - 1));
        start = ++it;
        value.push_back('\n');
      }
      else if (*it == 't') {
        value.append(&*start,
                     static_cast<size_t>(std::distance(start, it) - 1));
        start = ++it;
        value.push_back('\t');
      }
      else if (*it == 'r') {
        value.append(&*start,
                     static_cast<size_t>(std::distance(start, it) - 1));
        start = ++it;
        value.push_back('\r');
      }
      else if (*it == 'b') {
        value.append(&*start,
                     static_cast<size_t>(std::distance(start, it) - 1));
        start = ++it;
        value.push_back('\b');
      }
      else if (*it == 'f') {
        value.append(&*start,
                     static_cast<size_t>(std::distance(start, it) - 1));
        start = ++it;
        value.push_back('\f');
      }
    }
  }
  value.append(&*start, static_cast<size_t>(std::distance(start, end)));
}

// use '-' here to simply represent '>-'
template <char block_type, str_t U, class It>
IGUANA_INLINE void parse_block_str(U &value, It &&it, It &&end,
                                   size_t min_spaces) {
  auto spaces = skip_space_and_lines<false>(it, end, min_spaces);
  if (spaces < min_spaces) [[unlikely]] {
    // back to the end of the previous line
    if constexpr (block_type == '|' || block_type == '>') {
      value.push_back('\n');
    }
    it -= spaces + 1;
    return;
  }
  while (it != end) {
    auto start = it;
    auto value_end = skip_till<false, '\n'>(it, end);
    value.append(&*start, static_cast<size_t>(std::distance(start, value_end)));
    spaces = skip_space_and_lines<false>(it, end, min_spaces);
    if ((spaces < min_spaces) || (it == end)) [[unlikely]] {
      if constexpr (block_type == '|' || block_type == '>') {
        value.push_back('\n');
      }
      it -= spaces + 1;
      return;
    }
    if constexpr (block_type == '|') {
      value.push_back('\n');
    }
    else if constexpr (block_type == '-' || block_type == '>') {
      value.push_back(' ');
    }
  }
}

template <map_container U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces);

template <num_t U, class It>
IGUANA_INLINE void parse_value(U &value, It &&value_begin, It &&value_end) {
  if (value_begin == value_end) [[unlikely]] {
    return;
  }
  auto size = std::distance(value_begin, value_end);
  const auto start = &*value_begin;
  auto [p, ec] = detail::from_chars(start, start + size, value);
  if (ec != std::errc{}) [[unlikely]]
    throw std::runtime_error("Failed to parse number");
}

// string_view should be used  for string with ' " ?
template <string_t U, class It>
IGUANA_INLINE void parse_value(U &value, It &&value_begin, It &&value_end) {
  using T = std::decay_t<U>;
  auto start = value_begin;
  auto end = value_end;
  if (*value_begin == '"') {
    ++start;
    --end;
    if (*end != '"') [[unlikely]] {
      throw std::runtime_error(R"(Expected ")");
    }
    if constexpr (str_t<T>) {
      parse_escape_str(value, start, end);
      return;
    }
  }
  else if (*value_begin == '\'') {
    ++start;
    --end;
    if (*end != '\'') [[unlikely]] {
      throw std::runtime_error(R"(Expected ')");
    }
  }
  value = T(&*start, static_cast<size_t>(std::distance(start, end)));
}

template <char_t U, class It>
IGUANA_INLINE void parse_value(U &value, It &&value_begin, It &&value_end) {
  if (static_cast<size_t>(std::distance(value_begin, value_end)) != 1)
      [[unlikely]] {
    throw std::runtime_error("Expected one character");
  }
  value = *value_begin;
}

template <enum_t U, class It>
IGUANA_INLINE void parse_value(U &value, It &&value_begin, It &&value_end) {
  using T = std::underlying_type_t<std::decay_t<U>>;
  parse_value(reinterpret_cast<T &>(value), value_begin, value_end);
}

template <bool_t U, class It>
IGUANA_INLINE void parse_value(U &value, It &&value_begin, It &&value_end) {
  auto bool_v = std::string_view(
      &*value_begin,
      static_cast<size_t>(std::distance(value_begin, value_end)));
  if (bool_v == "true") {
    value = true;
  }
  else if (bool_v == "false") {
    value = false;
  }
  else [[unlikely]] {
    throw std::runtime_error("Expected true or false");
  }
}

template <refletable U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  from_yaml(value, it, end, min_spaces);
}

template <plain_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  using T = std::decay_t<U>;
  if constexpr (str_t<T>) {
    if (skip_space_till_end(it, end)) [[unlikely]] {
      return;
    }
    if (*it == '|') {
      ++it;
      parse_block_str<'|'>(value, it, end, min_spaces);
    }
    else if (*it == '>') {
      ++it;
      if (*it == '-') {
        ++it;
        parse_block_str<'-'>(value, it, end, min_spaces);
      }
      else {
        parse_block_str<'>'>(value, it, end, min_spaces);
      }
    }
    else {
      skip_space_and_lines(it, end, min_spaces);
      auto start = it;
      auto value_end = skip_till<false, '\n'>(it, end);
      parse_value(value, start, value_end);
    }
  }
  else {
    skip_space_and_lines(it, end, min_spaces);
    auto start = it;
    auto value_end = skip_till<false, '\n'>(it, end);
    parse_value(value, start, value_end);
  }
}

template <optional_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces);

template <unique_ptr_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces);

// minspaces : The minimum indentation
template <sequence_container_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  value.clear();
  auto spaces = skip_space_and_lines(it, end, min_spaces);
  using value_type = typename std::remove_cv_t<U>::value_type;
  if (*it == '[') {
    ++it;
    skip_space_and_lines(it, end, min_spaces);
    while (it != end) {
      if (*it == ']') [[unlikely]] {
        ++it;
        return;
      }
      if constexpr (plain_t<value_type>) {
        auto start = it;
        auto value_end = skip_till<true, ',', ']'>(it, end);
        parse_value(value.emplace_back(), start, value_end);
        if (*(it - 1) == ']') [[unlikely]] {
          return;
        }
      }
      else {
        parse_item(value.emplace_back(), it, end, spaces + 1);
        skip_space_and_lines(it, end, min_spaces);
        if (*it == ',') [[likely]] {
          ++it;
        }
      }
      skip_space_and_lines(it, end, min_spaces);
    }
  }
  else if (*it == '-') {
    while (it != end) {
      auto subspaces = skip_space_and_lines<false>(it, end, spaces);
      if (subspaces < spaces) [[unlikely]] {
        it -= subspaces + 1;
        return;
      }
      if (it == end) [[unlikely]]
        return;
      match<'-'>(it, end);
      // space + 1 because of the skipped '-'
      subspaces = skip_space_and_lines(it, end, spaces + 1);
      if constexpr (str_t<value_type>) {
        // except str_t because of scalar blocks
        parse_item(value.emplace_back(), it, end, spaces + 1);
      }
      else if constexpr (plain_t<value_type>) {
        auto start = it;
        auto value_end = skip_till<false, '\n'>(it, end);
        parse_value(value.emplace_back(), start, value_end);
      }
      else {
        parse_item(value.emplace_back(), it, end, subspaces);
      }
    }
  }
  else [[unlikely]] {
    throw std::runtime_error("Expected ']' or '-'");
  }
}

template <tuple_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  auto spaces = skip_space_and_lines(it, end, min_spaces);
  if (*it == '[') {
    ++it;
    skip_space_and_lines(it, end, min_spaces);
    for_each(value, [&](auto &v, auto i) IGUANA__INLINE_LAMBDA {
      using value_type = std::decay_t<decltype(v)>;
      skip_space_and_lines(it, end, min_spaces);
      if constexpr (plain_t<value_type>) {
        auto start = it;
        auto value_end = skip_till<true, ',', ']'>(it, end);
        parse_value(v, start, value_end);
      }
      else {
        parse_item(v, it, end, spaces + 1);
        skip_space_and_lines(it, end, min_spaces);
        ++it;  // skip ,
      }
    });
  }
  else if (*it == '-') {
    for_each(value, [&](auto &v, auto i) IGUANA__INLINE_LAMBDA {
      // we don't need to determine when it will end
      // because the user decides how many items there are
      // the following is similar to sequence_t
      using value_type = std::decay_t<decltype(v)>;
      skip_space_and_lines(it, end, spaces);
      match<'-'>(it, end);
      auto subspaces = skip_space_and_lines(it, end, spaces + 1);
      if constexpr (str_t<value_type>) {
        parse_item(v, it, end, spaces + 1);
      }
      else if constexpr (plain_t<value_type>) {
        auto start = it;
        auto value_end = skip_till<false, '\n'>(it, end);
        parse_value(v, start, value_end);
      }
      else {
        parse_item(v, it, end, subspaces);
      }
    });
  }
  else {
    throw std::runtime_error("Expected ']' or '-'");
  }
}

template <map_container U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  using T = std::remove_reference_t<U>;
  using key_type = typename T::key_type;
  using value_type = typename T::mapped_type;
  auto spaces = skip_space_and_lines(it, end, min_spaces);
  if (*it == '{') {
    ++it;
    auto subspaces = skip_space_and_lines(it, end, min_spaces);
    while (it != end) {
      if (*it == '}') [[unlikely]] {
        ++it;
        return;
      }
      auto start = it;
      auto value_end = skip_till<true, ':'>(it, end);
      key_type key;
      parse_value(key, start, value_end);
      subspaces = skip_space_and_lines(it, end, min_spaces);
      if constexpr (plain_t<value_type>) {
        start = it;
        value_end = skip_till<true, ',', '}'>(it, end);
        parse_value(value[key], start, value_end);
        if (*(it - 1) == '}') [[unlikely]] {
          return;
        }
      }
      else {
        parse_item(value[key], it, end, min_spaces);
        subspaces = skip_space_and_lines(it, end, min_spaces);
        if (*it == ',') [[likely]] {
          ++it;
        }
      }
      subspaces = skip_space_and_lines(it, end, min_spaces);
    }
  }
  else {
    auto subspaces = skip_space_and_lines<false>(it, end, min_spaces);
    while (it != end) {
      if (subspaces < min_spaces) [[unlikely]] {
        it -= subspaces + 1;
        return;
      }
      auto start = it;
      auto value_end = skip_till<true, ':'>(it, end);
      key_type key;
      parse_value(key, start, value_end);
      subspaces = skip_space_and_lines(it, end, min_spaces);
      if constexpr (plain_t<value_type> && !str_t<value_type>) {
        start = it;
        value_end = skip_till<false, '\n'>(it, end);
        parse_value(value[key], start, value_end);
      }
      else {
        parse_item(value[key], it, end, spaces + 1);
      }
      subspaces = skip_space_and_lines<false>(it, end, min_spaces);
    }
  }
}

template <unique_ptr_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  using T = std::remove_reference_t<U>;
  value = std::make_unique<typename T::element_type>();
  static_assert(!str_t<typename T::element_type>,
                "unique_ptr<string> is not allowed");
  parse_item(*value, it, end, min_spaces);
}

template <optional_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  using T = std::remove_reference_t<U>;
  using value_type = typename T::value_type;
  auto spaces = skip_space_and_lines<false>(it, end, min_spaces);
  if (it == end) [[unlikely]] {
    return;
  }
  if (spaces < min_spaces) [[unlikely]] {
    it -= spaces + 1;
    return;
  }
  auto start = it;
  std::decay_t<It> value_end = end;
  if (*it == 'n') {
    value_end = skip_till<false, '\n'>(it, end);
    auto opt_v = std::string_view(
        &*start, static_cast<size_t>(std::distance(start, value_end)));
    if (opt_v == "null") {
      return;
    }
  }
  if constexpr (str_t<value_type> || !plain_t<value_type>) {
    it = start;
    parse_item(value.emplace(), it, end, min_spaces);
  }
  else {
    if (value_end == end) [[likely]] {
      // if value_end isn't touched
      value_end = skip_till<false, '\n'>(it, end);
    }
    parse_value(value.emplace(), start, value_end);
  }
}

}  // namespace detail

template <refletable T, typename It>
IGUANA_INLINE void from_yaml(T &value, It &&it, It &&end, size_t min_spaces) {
  auto spaces = skip_space_and_lines(it, end, min_spaces);
  while (it != end) {
    auto start = it;
    auto keyend = skip_till<true, ':'>(it, end);
    std::string_view key = std::string_view{
        &*start, static_cast<size_t>(std::distance(start, keyend))};
    static constexpr auto frozen_map = get_iguana_struct_map<T>();
    if constexpr (frozen_map.size() > 0) {
      const auto &member_it = frozen_map.find(key);
      if (member_it != frozen_map.end()) [[likely]] {
        std::visit(
            [&](auto &&member_ptr) IGUANA__INLINE_LAMBDA {
              using V = std::decay_t<decltype(member_ptr)>;
              if constexpr (std::is_member_pointer_v<V>) {
                detail::parse_item(value.*member_ptr, it, end, spaces + 1);
              }
              else {
                static_assert(!sizeof(V), "type not supported");
              }
            },
            member_it->second);
      }
      else [[unlikely]] {
        // TODO: don't throw
        throw std::runtime_error("Unknown key: " + std::string(key));
      }
    }
    auto subspaces = skip_space_and_lines<false>(it, end, min_spaces);
    if (subspaces < min_spaces) [[unlikely]] {
      it -= subspaces + 1;  // back to the las line end
      return;
    }
  }
}

template <non_refletable T, typename It>
IGUANA_INLINE void from_yaml(T &value, It &&it, It &&end) {
  detail::parse_item(value, it, end, 0);
}

template <typename T, typename It>
IGUANA_INLINE void from_yaml(T &value, It &&it, It &&end,
                             std::error_code &ec) noexcept {
  try {
    from_yaml(value, it, end);
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
}

template <typename T, string_t View>
IGUANA_INLINE void from_yaml(T &value, const View &view) {
  from_yaml(value, std::begin(view), std::end(view));
}

template <typename T, string_t View>
IGUANA_INLINE void from_yaml(T &value, const View &view,
                             std::error_code &ec) noexcept {
  try {
    from_yaml(value, std::begin(view), std::end(view));
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
}

}  // namespace iguana
