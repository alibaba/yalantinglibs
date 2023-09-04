#pragma once
#include "detail/utf.hpp"
#include "error_code.h"
#include "json_util.hpp"
namespace iguana {

template <typename T, typename It, std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end);

namespace detail {

template <typename U, typename It,
          std::enable_if_t<sequence_container_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end);

template <typename U, typename It, std::enable_if_t<smart_ptr_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end);

template <typename U, typename It, std::enable_if_t<refletable_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  from_json(value, it, end);
}

template <typename U, typename It, std::enable_if_t<string_v<U>, int> = 0>
IGUANA_INLINE void parse_escape(U &value, It &&it, It &&end) {
  if (it == end)
    IGUANA_UNLIKELY { throw std::runtime_error(R"(Expected ")"); }
  if (*it == 'u') {
    ++it;
    if (std::distance(it, end) <= 4)
      IGUANA_UNLIKELY {
        throw std::runtime_error(R"(Expected 4 hexadecimal digits)");
      }
    auto code_point = parse_unicode_hex4(it);
    encode_utf8(value, code_point);
  }
  else if (*it == 'n') {
    ++it;
    value.push_back('\n');
  }
  else if (*it == 't') {
    ++it;
    value.push_back('\t');
  }
  else if (*it == 'r') {
    ++it;
    value.push_back('\r');
  }
  else if (*it == 'b') {
    ++it;
    value.push_back('\b');
  }
  else if (*it == 'f') {
    ++it;
    value.push_back('\f');
  }
  else {
    value.push_back(*it);  // add the escaped character
    ++it;
  }
}

template <typename U, typename It, std::enable_if_t<num_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  skip_ws(it, end);
  if constexpr (contiguous_iterator<std::decay_t<It>>) {
    const auto size = std::distance(it, end);
    if (size == 0)
      IGUANA_UNLIKELY { throw std::runtime_error("Failed to parse number"); }
    const auto start = &*it;
    auto [p, ec] = detail::from_chars(start, start + size, value);
    if (ec != std::errc{})
      IGUANA_UNLIKELY { throw std::runtime_error("Failed to parse number"); }
    it += (p - &*it);
  }
  else {
    char buffer[256];
    size_t i{};
    while (it != end && is_numeric(*it)) {
      if (i > 254)
        IGUANA_UNLIKELY { throw std::runtime_error("Number is too long"); }
      buffer[i] = *it++;
      ++i;
    }
    auto [p, ec] = detail::from_chars(buffer, buffer + i, value);
    if (ec != std::errc{})
      IGUANA_UNLIKELY { throw std::runtime_error("Failed to parse number"); }
  }
}

template <typename U, typename It, std::enable_if_t<numeric_str_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  skip_ws(it, end);
  auto start = it;
  while (it != end && is_numeric(*it)) {
    ++it;
  }
  value.value() =
      std::string_view(&*start, static_cast<size_t>(std::distance(start, it)));
}

template <bool skip = false, typename U, typename It,
          std::enable_if_t<char_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  if constexpr (!skip) {
    skip_ws(it, end);
    match<'"'>(it, end);
  }
  if (it == end)
    IGUANA_UNLIKELY { throw std::runtime_error("Unxpected end of buffer"); }
  if (*it == '\\')
    IGUANA_UNLIKELY {
      if (++it == end)
        IGUANA_UNLIKELY { throw std::runtime_error("Unxpected end of buffer"); }
      else if (*it == 'n') {
        value = '\n';
      }
      else if (*it == 't') {
        value = '\t';
      }
      else if (*it == 'r') {
        value = '\r';
      }
      else if (*it == 'b') {
        value = '\b';
      }
      else if (*it == 'f') {
        value = '\f';
      }
      else
        IGUANA_UNLIKELY { value = *it; }
    }
  else {
    value = *it;
  }
  ++it;
  if constexpr (!skip) {
    match<'"'>(it, end);
  }
}

template <typename U, typename It, std::enable_if_t<bool_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &&value, It &&it, It &&end) {
  skip_ws(it, end);

  if (it < end)
    IGUANA_LIKELY {
      switch (*it) {
        case 't':
          ++it;
          match<'r', 'u', 'e'>(it, end);
          value = true;
          break;
        case 'f':
          ++it;
          match<'a', 'l', 's', 'e'>(it, end);
          value = false;
          break;
          IGUANA_UNLIKELY default
              : throw std::runtime_error("Expected true or false");
      }
    }
  else
    IGUANA_UNLIKELY { throw std::runtime_error("Expected true or false"); }
}

template <bool skip = false, typename U, typename It,
          std::enable_if_t<string_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  if constexpr (!skip) {
    skip_ws(it, end);
    match<'"'>(it, end);
  }
  value.clear();
  if constexpr (contiguous_iterator<std::decay_t<It>>) {
    auto start = it;
    while (it < end) {
      skip_till_escape_or_qoute(it, end);
      if (*it == '"') {
        value.append(&*start, static_cast<size_t>(std::distance(start, it)));
        ++it;
        return;
      }
      else {
        // Must be an escape
        value.append(&*start, static_cast<size_t>(std::distance(start, it)));
        ++it;  // skip first escape
        parse_escape(value, it, end);
        start = it;
      }
    }
  }
  else {
    while (it != end) {
      switch (*it) {
        IGUANA_UNLIKELY case '\\' : ++it;
        parse_escape(value, it, end);
        break;
        // IGUANA_UNLIKELY case ']' : return;
        IGUANA_UNLIKELY case '"' : ++it;
        return;
        IGUANA_LIKELY default : value.push_back(*it);
        ++it;
      }
    }
  }
}

template <bool skip = false, typename U, typename It,
          std::enable_if_t<string_view_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  static_assert(contiguous_iterator<std::decay_t<It>>, "must be contiguous");
  if constexpr (!skip) {
    skip_ws(it, end);
    match<'"'>(it, end);
  }
  using T = std::decay_t<U>;
  auto start = it;
  while (it != end) {
    skip_till_qoute(it, end);
    if (*(it - 1) != '\\') {
      value = T(&*start, static_cast<size_t>(std::distance(start, it)));
      ++it;
      return;
    }
    ++it;
  }
  throw std::runtime_error("Expected \"");
}

template <typename U, typename It, std::enable_if_t<enum_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  static constexpr auto str_to_enum = get_enum_map<true, std::decay_t<U>>();
  if constexpr (bool_v<decltype(str_to_enum)>) {
    // not defined a specialization template
    using T = std::underlying_type_t<std::decay_t<U>>;
    parse_item(reinterpret_cast<T &>(value), it, end);
  }
  else {
    std::string_view enum_names;
    parse_item(enum_names, it, end);
    auto it = str_to_enum.find(enum_names);
    if (it != str_to_enum.end())
      IGUANA_LIKELY { value = it->second; }
    else {
      throw std::runtime_error(std::string(enum_names) +
                               " missing corresponding value in enum_value");
    }
  }
}

template <typename U, typename It, std::enable_if_t<fixed_array_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  using T = std::remove_reference_t<U>;
  constexpr auto n = sizeof(T) / sizeof(decltype(std::declval<T>()[0]));
  skip_ws(it, end);

  if constexpr (std::is_same_v<char, std::remove_reference_t<
                                         decltype(std::declval<T>()[0])>>) {
    if (*it == '"') {
      match<'"'>(it, end);
      auto value_it = std::begin(value);
      for (size_t i = 0; i < n; ++i) {
        if (*it != '"')
          IGUANA_LIKELY { parse_item<true>(*value_it++, it, end); }
      }
      match<'"'>(it, end);
      return;
    }
  }
  match<'['>(it, end);
  skip_ws(it, end);
  if (it == end)
    IGUANA_UNLIKELY { throw std::runtime_error("Unexpected end"); }

  if (*it == ']')
    IGUANA_UNLIKELY {
      ++it;
      return;
    }
  auto value_it = std::begin(value);
  for (size_t i = 0; i < n; ++i) {
    parse_item(*value_it++, it, end);
    skip_ws(it, end);
    if (it == end) {
      throw std::runtime_error("Unexpected end");
    }
    if (*it == ',')
      IGUANA_LIKELY {
        ++it;
        skip_ws(it, end);
      }
    else if (*it == ']') {
      ++it;
      return;
    }
    else
      IGUANA_UNLIKELY { throw std::runtime_error("Expected ]"); }
  }
}

template <typename U, typename It,
          std::enable_if_t<sequence_container_v<U>, int>>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  value.clear();
  skip_ws(it, end);

  match<'['>(it, end);
  skip_ws(it, end);
  for (size_t i = 0; it != end; ++i) {
    if (*it == ']')
      IGUANA_UNLIKELY {
        ++it;
        return;
      }
    if (i > 0)
      IGUANA_LIKELY { match<','>(it, end); }

    parse_item(value.emplace_back(), it, end);
    skip_ws(it, end);
  }
  throw std::runtime_error("Expected ]");
}

template <typename It>
IGUANA_INLINE auto get_key(It &&it, It &&end) {
  if constexpr (contiguous_iterator<std::decay_t<It>>) {
    // skip white space and escape characters and find the string
    skip_ws(it, end);
    match<'"'>(it, end);
    auto start = it;
    skip_till_escape_or_qoute(it, end);
    if (*it == '\\')
      IGUANA_UNLIKELY {
        // we dont' optimize this currently because it would increase binary
        // size significantly with the complexity of generating escaped
        // compile time versions of keys
        it = start;
        static thread_local std::string static_key{};
        detail::parse_item<true>(static_key, it, end);
        return std::string_view(static_key);
      }
    else
      IGUANA_LIKELY {
        auto key = std::string_view{
            &*start, static_cast<size_t>(std::distance(start, it))};
        ++it;
        if (key[0] == '@')
          IGUANA_UNLIKELY { return key.substr(1); }
        return key;
      }
  }
  else {
    static thread_local std::string static_key{};
    detail::parse_item(static_key, it, end);
    return std::string_view(static_key);
  }
}

template <typename U, typename It,
          std::enable_if_t<map_container_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  using T = std::remove_reference_t<U>;
  using key_type = typename T::key_type;
  skip_ws(it, end);

  match<'{'>(it, end);
  skip_ws(it, end);
  bool first = true;
  while (it != end) {
    if (*it == '}')
      IGUANA_UNLIKELY {
        ++it;
        return;
      }
    else if (first)
      IGUANA_UNLIKELY { first = false; }
    else
      IGUANA_LIKELY { match<','>(it, end); }

    std::string_view key = get_key(it, end);

    skip_ws(it, end);
    match<':'>(it, end);

    if constexpr (string_v<key_type> || string_view_v<key_type>) {
      parse_item(value[key_type(key)], it, end);
    }
    else {
      static thread_local key_type key_value{};
      parse_item(key_value, key.begin(), key.end());
      parse_item(value[key_value], it, end);
    }
    skip_ws(it, end);
  }
}

template <typename U, typename It, std::enable_if_t<tuple_v<U>, int> = 0>
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

template <typename U, typename It, std::enable_if_t<optional_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  skip_ws(it, end);
  if (it < end && *it == '"')
    IGUANA_LIKELY { ++it; }
  using T = std::remove_reference_t<U>;
  if (it == end)
    IGUANA_UNLIKELY { throw std::runtime_error("Unexexpected eof"); }
  if (*it == 'n') {
    ++it;
    match<'u', 'l', 'l'>(it, end);
    if constexpr (!std::is_pointer_v<T>) {
      value.reset();
      if (it < end && *it == '"') {
        ++it;
      }
    }
  }
  else {
    using value_type = typename T::value_type;
    value_type t;
    if constexpr (string_v<value_type> || string_view_v<value_type>) {
      parse_item<true>(t, it, end);
    }
    else {
      parse_item(t, it, end);
    }
    value = std::move(t);
  }
}

template <typename U, typename It, std::enable_if_t<smart_ptr_v<U>, int>>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end) {
  skip_ws(it, end);
  if (it == end)
    IGUANA_UNLIKELY { throw std::runtime_error("Unexexpected eof"); }
  if (*it == 'n') {
    ++it;
    match<'u', 'l', 'l'>(it, end);
  }
  else {
    using value_type = typename std::remove_reference_t<U>::element_type;
    if constexpr (unique_ptr_v<U>) {
      value = std::make_unique<value_type>();
    }
    else {
      value = std::make_shared<value_type>();
    }
    parse_item(*value, it, end);
  }
}

template <typename It>
IGUANA_INLINE void skip_object_value(It &&it, It &&end) {
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

}  // namespace detail

template <typename T, typename It, std::enable_if_t<refletable_v<T>, int>>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end) {
  skip_ws(it, end);
  match<'{'>(it, end);

  skip_ws(it, end);
  if (*it == '}')
    IGUANA_UNLIKELY {
      ++it;
      return;
    }
  std::string_view key = detail::get_key(it, end);
#ifdef SEQUENTIAL_PARSE
  bool parse_done = false;
  for_each(value, [&](const auto member_ptr, auto i) IGUANA__INLINE_LAMBDA {
    constexpr auto mkey = iguana::get_name<T, decltype(i)::value>();
    constexpr std::string_view st_key(mkey.data(), mkey.size());
    if (parse_done || (key != st_key))
      IGUANA_UNLIKELY { return; }
    skip_ws(it, end);
    match<':'>(it, end);
    detail::parse_item(value.*member_ptr, it, end);

    skip_ws(it, end);
    if (*it == '}')
      IGUANA_UNLIKELY {
        ++it;
        parse_done = true;
        return;
      }
    else
      IGUANA_LIKELY { match<','>(it, end); }
    key = detail::get_key(it, end);
  });
  if (parse_done) [[unlikely]] {
    return;
  }
#endif
  while (it != end) {
    static constexpr auto frozen_map = get_iguana_struct_map<T>();
    if constexpr (frozen_map.size() > 0) {
      const auto &member_it = frozen_map.find(key);
      skip_ws(it, end);
      match<':'>(it, end);
      if (member_it != frozen_map.end())
        IGUANA_LIKELY {
          std::visit(
              [&](auto &&member_ptr) IGUANA__INLINE_LAMBDA {
                using V = std::decay_t<decltype(member_ptr)>;
                if constexpr (std::is_member_pointer_v<V>) {
                  detail::parse_item(value.*member_ptr, it, end);
                }
                else {
                  static_assert(!sizeof(V), "type not supported");
                }
              },
              member_it->second);
        }
      else
        IGUANA_UNLIKELY {
#ifdef THROW_UNKNOWN_KEY
          throw std::runtime_error("Unknown key: " + std::string(key));
#else
          detail::skip_object_value(it, end);
#endif
        }
    }
    skip_ws(it, end);
    if (*it == '}')
      IGUANA_UNLIKELY {
        ++it;
        return;
      }
    else
      IGUANA_LIKELY { match<','>(it, end); }
    key = detail::get_key(it, end);
  }
}

template <typename T, typename It,
          std::enable_if_t<non_refletable_v<T>, int> = 0>
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

template <typename T, typename View,
          std::enable_if_t<json_view_v<View>, int> = 0>
IGUANA_INLINE void from_json(T &value, const View &view) {
  from_json(value, std::begin(view), std::end(view));
}

template <typename T, typename View,
          std::enable_if_t<json_view_v<View>, int> = 0>
IGUANA_INLINE void from_json(T &value, const View &view,
                             std::error_code &ec) noexcept {
  try {
    from_json(value, view);
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
}

template <typename T, typename Byte,
          std::enable_if_t<json_byte_v<Byte>, int> = 0>
IGUANA_INLINE void from_json(T &value, const Byte *data, size_t size) {
  std::string_view buffer(data, size);
  from_json(value, buffer);
}

template <typename T, typename Byte,
          std::enable_if_t<json_byte_v<Byte>, int> = 0>
IGUANA_INLINE void from_json(T &value, const Byte *data, size_t size,
                             std::error_code &ec) noexcept {
  try {
    from_json(value, data, size);
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
}

template <bool Is_view = false, typename It>
void parse(jvalue &result, It &&it, It &&end);

template <bool Is_view = false, typename It>
inline void parse(jarray &result, It &&it, It &&end) {
  skip_ws(it, end);
  match<'['>(it, end);
  if (*it == ']')
    IGUANA_UNLIKELY {
      ++it;
      return;
    }
  while (true) {
    if (it == end) {
      break;
    }
    result.emplace_back();

    parse<Is_view>(result.back(), it, end);

    if (*it == ']')
      IGUANA_UNLIKELY {
        ++it;
        return;
      }

    match<','>(it, end);
  }
  throw std::runtime_error("Expected ]");
}

template <bool Is_view = false, typename It>
inline void parse(jobject &result, It &&it, It &&end) {
  skip_ws(it, end);
  match<'{'>(it, end);
  if (*it == '}')
    IGUANA_UNLIKELY {
      ++it;
      return;
    }

  skip_ws(it, end);

  while (true) {
    if (it == end)
      IGUANA_UNLIKELY { throw std::runtime_error("Expected }"); }
    std::string key;
    detail::parse_item(key, it, end);

    auto emplaced = result.try_emplace(key);
    if (!emplaced.second)
      throw std::runtime_error("duplicated key " + key);

    match<':'>(it, end);

    parse<Is_view>(emplaced.first->second, it, end);

    if (*it == '}')
      IGUANA_UNLIKELY {
        ++it;
        return;
      }

    match<','>(it, end);
  }
}

template <bool Is_view, typename It>
inline void parse(jvalue &result, It &&it, It &&end) {
  skip_ws(it, end);
  switch (*it) {
    case 'n':
      ++it;
      match<'u', 'l', 'l'>(it, end);
      result.template emplace<std::nullptr_t>();
      break;

    case 'f':
    case 't':
      detail::parse_item(result.template emplace<bool>(), it, end);
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-': {
      double d{};
      detail::parse_item(d, it, end);
      if (static_cast<int>(d) == d)
        result.emplace<int>(d);
      else
        result.emplace<double>(d);
      break;
    }
    case '"':
      if constexpr (Is_view) {
        result.template emplace<std::string_view>();
        detail::parse_item(std::get<std::string_view>(result), it, end);
      }
      else {
        result.template emplace<std::string>();
        detail::parse_item(std::get<std::string>(result), it, end);
      }
      break;
    case '[':
      result.template emplace<jarray>();
      parse<Is_view>(std::get<jarray>(result), it, end);
      break;
    case '{': {
      result.template emplace<jobject>();
      parse<Is_view>(std::get<jobject>(result), it, end);
      break;
    }
    default:
      throw std::runtime_error("parse failed");
  }

  skip_ws(it, end);
}

// when Is_view is true, parse str as string_view
template <bool Is_view = false, typename It>
inline void parse(jvalue &result, It &&it, It &&end, std::error_code &ec) {
  try {
    parse<Is_view>(result, it, end);
    ec = {};
  } catch (const std::runtime_error &e) {
    result.template emplace<std::nullptr_t>();
    ec = iguana::make_error_code(e.what());
  }
}

template <bool Is_view = false, typename T, typename View,
          std::enable_if_t<json_view_v<View>, int> = 0>
inline void parse(T &result, const View &view) {
  parse<Is_view>(result, std::begin(view), std::end(view));
}

template <bool Is_view = false, typename T, typename View,
          std::enable_if_t<json_view_v<View>, int> = 0>
inline void parse(T &result, const View &view, std::error_code &ec) noexcept {
  try {
    parse<Is_view>(result, view);
    ec = {};
  } catch (std::runtime_error &e) {
    ec = iguana::make_error_code(e.what());
  }
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

}  // namespace iguana
