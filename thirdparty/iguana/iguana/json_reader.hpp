#pragma once
#include "detail/fast_float.h"
#include "detail/utf.hpp"
#include "json_util.hpp"
#include "reflection.hpp"
#include <charconv>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <string_view>
#include <type_traits>

#include "error_code.h"

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
concept num_t = std::floating_point<std::decay_t<T>> || int_t<T>;

template <class T>
concept enum_type_t = std::is_enum_v<std::decay_t<T>>;

template <class T>
concept str_t = std::convertible_to<std::decay_t<T>, std::string_view>;

template <typename Type> constexpr inline bool is_std_vector_v = false;

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

template <typename Type> constexpr inline bool is_std_list_v = false;
template <typename... args>
constexpr inline bool is_std_list_v<std::list<args...>> = true;

template <typename Type> constexpr inline bool is_std_deque_v = false;
template <typename... args>
constexpr inline bool is_std_deque_v<std::deque<args...>> = true;

template <typename Type>
concept sequence_container = is_std_list_v<std::remove_reference_t<Type>> ||
    is_std_vector_v<std::remove_reference_t<Type>> ||
    is_std_deque_v<std::remove_reference_t<Type>>;

template <class T>
concept non_refletable = container<T> || c_array<T> || tuple<T> ||
    optional<T> || std::is_fundamental_v<T>;

template <refletable T, typename It>
void from_json(T &value, It &&it, It &&end);

namespace detail {
template <refletable U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  from_json(value, it, end);
}

template <num_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  skip_ws(it, end);

  using T = std::remove_reference_t<U>;

  if constexpr (std::contiguous_iterator<std::decay_t<It>>) {
    if constexpr (std::is_floating_point_v<T>) {
      const auto size = std::distance(it, end);
      if (size == 0) [[unlikely]]
        throw std::runtime_error("Failed to parse number");
      const auto start = &*it;
      auto [p, ec] = fast_float::from_chars(start, start + size, value);
      if (ec != std::errc{}) [[unlikely]]
        throw std::runtime_error("Failed to parse number");
      it += (p - &*it);
    } else {
      const auto size = std::distance(it, end);
      const auto start = &*it;
      auto [p, ec] = std::from_chars(start, start + size, value);
      if (ec != std::errc{}) [[unlikely]]
        throw std::runtime_error("Failed to parse number");
      it += (p - &*it);
    }
  } else {
    double num;
    char buffer[256];
    size_t i{};
    while (it != end && is_numeric(*it)) {
      if (i > 254) [[unlikely]]
        throw std::runtime_error("Number is too long");
      buffer[i] = *it++;
      ++i;
    }
    auto [p, ec] = fast_float::from_chars(buffer, buffer + i, num);
    if (ec != std::errc{}) [[unlikely]]
      throw std::runtime_error("Failed to parse number");
    value = static_cast<T>(num);
  }
}

template <enum_type_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  parse_item((int &)value, it, end);
}

template <str_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, bool skip = false) {
  if (!skip) {
    skip_ws(it, end);
    match<'"'>(it, end);
  }

  if constexpr (!std::contiguous_iterator<std::decay_t<It>>) {
    const auto cend = value.cend();
    for (auto c = value.begin(); c < cend; ++c, ++it) {
      if (it == end) [[unlikely]]
        throw std::runtime_error(R"(Expected ")");
      switch (*it) {
        [[unlikely]] case '\\' : {
          if (++it == end) [[unlikely]]
            throw std::runtime_error(R"(Expected ")");
          else [[likely]] {
            *c = *it;
          }
          break;
        }
        [[unlikely]] case '"' : {
          ++it;
          value.resize(std::distance(value.begin(), c));
          return;
        }
        [[unlikely]] case 'u' : {
          ++it;
          auto start = it;
          auto code_point = parse_unicode_hex4(it);
          std::string str;
          encode_utf8(str, code_point);
          std::memcpy(value.data(), str.data(), str.size());
          --it;
          c += std::distance(start, it) - 1;

          break;
        }
        [[likely]] default : *c = *it;
      }
    }
  }

  // growth portion
  if constexpr (std::contiguous_iterator<std::decay_t<It>>) {
    value.clear(); // Single append on unescaped strings so overwrite opt isnt
                   // as important
    auto start = it;
    while (it < end) {
      skip_till_escape_or_qoute(it, end);
      if (*it == '"') {
        value.append(&*start, static_cast<size_t>(std::distance(start, it)));
        ++it;
        return;
      } else {
        // Must be an escape
        // TODO propperly handle this
        value.append(&*start, static_cast<size_t>(std::distance(start, it)));
        ++it; // skip first escape
        if (*it == 'u') {
          ++it;
          auto code_point = parse_unicode_hex4(it);
          encode_utf8(value, code_point);
          start = it;
        } else {
          value.push_back(*it); // add the escaped character
          ++it;
          start = it;
        }
      }
    }
  } else {
    while (it != end) {
      switch (*it) {
        [[unlikely]] case '\\' : {
          if (++it == end) [[unlikely]]
            throw std::runtime_error(R"(Expected ")");
          else [[likely]] {
            value.push_back(*it);
          }
          break;
        }
        [[unlikely]] case ']' : { return; }
        [[unlikely]] case '"' : {
          ++it;
          return;
        }
        [[unlikely]] case 'u' : {
          ++it;
          auto code_point = parse_unicode_hex4(it);
          encode_utf8(value, code_point);
          break;
        }
        [[likely]] default : value.push_back(*it);
      }
      ++it;
    }
  }
}

template <fixed_array U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  using T = std::remove_reference_t<U>;
  skip_ws(it, end);

  match<'['>(it, end);
  skip_ws(it, end);
  if (it == end) {
    throw std::runtime_error("Unexpected end");
  }

  if (*it == ']') [[unlikely]] {
    ++it;
    return;
  }

  constexpr auto n = sizeof(T) / sizeof(decltype(std::declval<T>()[0]));

  auto value_it = std::begin(value);

  for (size_t i = 0; i < n; ++i) {
    parse_item(*value_it++, it, end);
    skip_ws(it, end);
    if (it == end) {
      throw std::runtime_error("Unexpected end");
    }
    if (*it == ',') [[likely]] {
      ++it;
      skip_ws(it, end);
    } else if (*it == ']') {
      ++it;
      return;
    } else [[unlikely]] {
      throw std::runtime_error("Expected ]");
    }
  }
}

template <sequence_container U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  value.clear();
  skip_ws(it, end);

  match<'['>(it, end);
  skip_ws(it, end);
  for (size_t i = 0; it != end; ++i) {
    if (*it == ']') [[unlikely]] {
      ++it;
      return;
    }
    if (i > 0) [[likely]] {
      match<','>(it, end);
    }

    using value_type = typename std::remove_cv_t<U>::value_type;
    if constexpr (refletable<value_type>) {
      from_json(value.emplace_back(), it, end);
    } else {
      parse_item(value.emplace_back(), it, end);
    }

    skip_ws(it, end);
  }
  throw std::runtime_error("Expected ]");
}

template <map_container U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  using T = std::remove_reference_t<U>;
  skip_ws(it, end);

  match<'{'>(it, end);
  skip_ws(it, end);
  bool first = true;
  while (it != end) {
    if (*it == '}') [[unlikely]] {
      ++it;
      return;
    } else if (first) [[unlikely]]
      first = false;
    else [[likely]] {
      match<','>(it, end);
    }

    static thread_local std::string key{};
    parse_item(key, it, end);

    skip_ws(it, end);
    match<':'>(it, end);

    if constexpr (std::is_same_v<typename T::key_type, std::string>) {
      parse_item(value[key], it, end);
    } else {
      static thread_local typename T::key_type key_value{};
      parse_item(key_value, key.begin(), key.end());
      parse_item(value[key_value], it, end);
    }
    skip_ws(it, end);
  }
}

template <tuple U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  skip_ws(it, end);
  match<'['>(it, end);
  skip_ws(it, end);

  for_each(value, [&](auto &v, auto i) IGUANA__INLINE_LAMBDA {
    constexpr auto I = decltype(i)::value;
    if (it == end || *it == ']') {
      return;
    }
    if constexpr (I != 0) {
      match<','>(it, end);
      skip_ws(it, end);
    }
    parse_item(v, it, end);
    skip_ws(it, end);
  });

  match<']'>(it, end);
}

template <bool_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  skip_ws(it, end);

  if (it < end) [[likely]] {
    switch (*it) {
    case 't': {
      ++it;
      match<"rue">(it, end);
      value = true;
      break;
    }
    case 'f': {
      ++it;
      match<"alse">(it, end);
      value = false;
      break;
    }
      [[unlikely]] default : throw std::runtime_error("Expected true or false");
    }
  } else [[unlikely]] {
    throw std::runtime_error("Expected true or false");
  }
}

template <optional U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  skip_ws(it, end);
  if (it < end && *it == '"') {
    ++it;
  }
  using T = std::remove_reference_t<U>;
  if (it == end) {
    throw std::runtime_error("Unexexpected eof");
  }
  if (*it == 'n') {
    ++it;
    match<"ull">(it, end);
    if constexpr (!std::is_pointer_v<T>) {
      value.reset();
      if (it < end && *it == '"') {
        ++it;
      }
    }
  } else {
    using value_type = typename T::value_type;
    value_type t;
    if constexpr (str_t<value_type>) {
      parse_item(t, it, end, true);
    } else {
      parse_item(t, it, end);
    }

    value = std::move(t);
  }
}

template <char_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  // TODO: this does not handle escaped chars
  skip_ws(it, end);
  match<'"'>(it, end);
  if (it == end) [[unlikely]]
    throw std::runtime_error("Unxpected end of buffer");
  if (*it == '\\') [[unlikely]]
    if (++it == end) [[unlikely]]
      throw std::runtime_error("Unxpected end of buffer");
  value = *it++;
  match<'"'>(it, end);
}

inline void skip_object_value(auto &&it, auto &&end) {
  skip_ws(it, end);
  while (it != end) {
    switch (*it) {
    case '{':
      skip_until_closed<'{', '}'>(it, end);
      break;
    case '[':
      skip_until_closed<'[', ']'>(it, end);
      break;
    case '"':
      skip_string(it, end);
      break;
    case '/':
      skip_comment(it, end);
      continue;
    case ',':
    case '}':
    case ']':
      break;
    default: {
      ++it;
      continue;
    }
    }

    break;
  }
}
} // namespace detail

template <refletable T, typename It>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end) {
  skip_ws(it, end);

  match<'{'>(it, end);
  skip_ws(it, end);
  bool first = true;
  while (it != end) {
    if (*it == '}') [[unlikely]] {
      ++it;
      return;
    } else if (first) [[unlikely]]
      first = false;
    else [[likely]] {
      match<','>(it, end);
    }

    if constexpr (refletable<T>) {
      std::string_view key;
      if constexpr (std::contiguous_iterator<std::decay_t<It>>) {
        // skip white space and escape characters and find the string
        skip_ws(it, end);
        match<'"'>(it, end);
        auto start = it;
        skip_till_escape_or_qoute(it, end);
        if (*it == '\\') [[unlikely]] {
          // we dont' optimize this currently because it would increase binary
          // size significantly with the complexity of generating escaped
          // compile time versions of keys
          it = start;
          static thread_local std::string static_key{};
          detail::parse_item(static_key, it, end, true);
          key = static_key;
        } else [[likely]] {
          key = std::string_view{&*start,
                                 static_cast<size_t>(std::distance(start, it))};
          if (key[0] == '@') [[unlikely]] {
            key = key.substr(1);
          }
          ++it;
        }
      } else {
        static thread_local std::string static_key{};
        detail::parse_item(static_key, it, end, true);
        key = static_key;
      }

      skip_ws(it, end);
      match<':'>(it, end);

      static constexpr auto frozen_map = get_iguana_struct_map<T>();
      if constexpr (frozen_map.size() > 0) {
        const auto &member_it = frozen_map.find(key);
        if (member_it != frozen_map.end()) {
          std::visit(
              [&](auto &&member_ptr) IGUANA__INLINE_LAMBDA {
                using V = std::decay_t<decltype(member_ptr)>;
                if constexpr (std::is_member_pointer_v<V>) {
                  detail::parse_item(value.*member_ptr, it, end);
                } else {
                  static_assert(!sizeof(V), "type not supported");
                }
              },
              member_it->second);
        } else [[unlikely]] {
#ifdef THROW_UNKNOWN_KEY
          throw std::runtime_error("Unknown key: " + std::string(key));
#else
          detail::skip_object_value(it, end);
#endif
        }
      }
    }
    skip_ws(it, end);
  }
}

template <non_refletable T, typename It>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end) {
  detail::parse_item(value, it, end);
}

template <typename T, typename It>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end,
                             std::error_code &ec) noexcept {
  try {
    from_json(value, it, end);
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
}

template <typename T, json_view View>
IGUANA_INLINE void from_json(T &value, const View &view) {
  from_json(value, std::begin(view), std::end(view));
}

template <typename T, json_view View>
IGUANA_INLINE void from_json(T &value, const View &view,
                             std::error_code &ec) noexcept {
  try {
    from_json(value, view);
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
}

template <typename T, json_byte Byte>
IGUANA_INLINE void from_json(T &value, const Byte *data, size_t size) {
  std::string_view buffer(data, size);
  from_json(value, buffer);
}

template <typename T, json_byte Byte>
IGUANA_INLINE void from_json(T &value, const Byte *data, size_t size,
                             std::error_code &ec) noexcept {
  try {
    from_json(value, data, size);
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
}

template <typename T, typename It>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end) {
  static_assert(!sizeof(T), "The type is not support, please check if you have "
                            "defined REFLECTION for the type, otherwise the "
                            "type is not supported now!");
}

IGUANA_INLINE std::string json_file_content(const std::string &filename) {
  std::error_code ec;
  uint64_t size = std::filesystem::file_size(filename, ec);
  if (ec) {
    throw std::runtime_error("file size error " + ec.message());
  }

  if (size == 0) {
    throw std::runtime_error("empty file");
  }

  std::string content;
  content.resize(size);

  std::ifstream file(filename, std::ios::binary);
  file.read(content.data(), content.size());

  return content;
}

template <typename T>
IGUANA_INLINE void from_json_file(T &value, const std::string &filename) {
  std::string content = json_file_content(filename);
  from_json(value, content.begin(), content.end());
}

template <typename T>
IGUANA_INLINE void from_json_file(T &value, const std::string &filename,
                                  std::error_code &ec) noexcept {
  try {
    from_json_file(value, filename);
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
}

} // namespace iguana
