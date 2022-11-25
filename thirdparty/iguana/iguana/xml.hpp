//
// Created by qiyu on 17-6-6.
//

#ifndef IGUANA_XML17_HPP
#define IGUANA_XML17_HPP
#include "reflection.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <string.h>

#define IGUANA_XML_READER_CHECK_FORWARD                                        \
  if (l > length)                                                              \
    return 0;                                                                  \
  work_ptr += l;                                                               \
  length -= l
#define IGUANA_XML_READER_CHECK_FORWARD_CHAR                                   \
  if (length < 1)                                                              \
    return 0;                                                                  \
  work_ptr += 1;                                                               \
  length -= 1
#define IGUANA_XML_HEADER "<?xml version = \"1.0\" encoding=\"UTF-8\">"

namespace iguana::xml {
namespace detail {
struct char_const {
  static constexpr char angle_bracket = 0x3c;      // '<'
  static constexpr char anti_angle_bracket = 0x3e; // '>'
  static constexpr char slash = 0x2f;              // '/'
  static constexpr char space = 0x20;              // ' '
  static constexpr char horizental_tab = 0x09;     // '/t'
  static constexpr char line_feed = 0x0a;          // '/n'
  static constexpr char enter = 0x0d;              // '/r'
  static constexpr char quote = 0x22;              // '"'
  static constexpr char underline = 0x5f;          // '_'
  static constexpr char question_mark = 0x3f;      // '?'
};

inline bool expected_char(char const *str, char character) {
  if (*str == character)
    return true;
  return false;
}

template <typename Pred>
inline size_t forward(char const *begin, size_t length, Pred p) {
  auto work_ptr = begin;
  for (size_t traversed = 0; traversed < length; traversed++) {
    char work = *work_ptr;
    if (!p(work)) {
      ++work_ptr;
    } else {
      return traversed;
    }
  }

  return length;
}

inline size_t ignore_blank_ctrl(char const *begin, size_t length) {
  return forward(begin, length,
                 [](auto c) { return !std::isblank(c) && !std::iscntrl(c); });
}

inline size_t get_token(char const *str, size_t length) {
  if (!(std::isalpha(*str) || expected_char(str++, char_const::underline)))
    return 0;

  return forward(str, length - 1, [](auto c) {
    return !std::isalnum(c) && char_const::underline != c;
  });
}

inline bool expected_token(char const *str, size_t length, char const *expected,
                           size_t expected_length) {
  if (expected_length != length)
    return false;

  if (std::strncmp(str, expected, length) == 0)
    return true;
  return false;
}

inline size_t forward_until(char const *str, size_t length, char until_c) {
  return forward(str, length, [until_c](auto c) { return until_c == c; });
}

inline size_t forward_after(char const *str, size_t length, char after_c) {
  auto l = forward_until(str, length, after_c);
  if (l < length)
    l++;
  return l;
}

template <typename T>
constexpr bool is_64_v =
    std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint64_t>;

template <typename T>
inline auto get_value(char const *str, size_t length, T &value)
    -> std::enable_if_t<std::is_arithmetic<T>::value> {
  using U = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (std::is_integral_v<U> && !detail::is_64_v<U>) {
    value = std::atoi(str);
  } else if constexpr (std::is_floating_point_v<U>) {
    value = std::atof(str);
  } else if constexpr (detail::is_64_v<U>) {
    value = std::atoll(str);
  } else {
    std::cout << "don't support the type now" << std::endl;
    throw(std::invalid_argument("don't support the type now"));
  }
}

void get_value(char const *str, size_t length, std::string &value) {
  value.assign(str, length);
}

template <typename T> struct array_size { static constexpr size_t value = 0; };

template <typename T, size_t Size> struct array_size<T const (&)[Size]> {
  static constexpr size_t value = Size;
};
} // namespace detail

class xml_reader_t {
  enum object_status {
    EMPTY = -1,
    ILLEGAL = 0,
    NORMAL = 1,
  };

public:
  xml_reader_t(char const *buffer, size_t length)
      : buffer_(buffer), length_(length) {}

  bool get_root() {
    auto work_ptr = buffer_;
    auto length = length_;

    auto l = detail::ignore_blank_ctrl(work_ptr, length);
    IGUANA_XML_READER_CHECK_FORWARD;

    if (!detail::expected_char(work_ptr, detail::char_const::angle_bracket))
      return false;
    IGUANA_XML_READER_CHECK_FORWARD_CHAR;

    if (!detail::expected_char(work_ptr, detail::char_const::question_mark))
      return false;
    IGUANA_XML_READER_CHECK_FORWARD_CHAR;

    l = detail::get_token(work_ptr, length);
    if (!detail::expected_token(work_ptr, l, "xml", 3))
      return false;
    IGUANA_XML_READER_CHECK_FORWARD;

    l = detail::forward_after(work_ptr, length,
                              detail::char_const::anti_angle_bracket);
    IGUANA_XML_READER_CHECK_FORWARD;

    buffer_ = work_ptr;
    length_ = length;

    return true;
  }

  int begin_object(char const *expected) {
    auto work_ptr = buffer_;
    auto length = length_;

    auto l = detail::ignore_blank_ctrl(work_ptr, length);
    IGUANA_XML_READER_CHECK_FORWARD;

    if (!detail::expected_char(work_ptr, detail::char_const::angle_bracket))
      return object_status::ILLEGAL;
    IGUANA_XML_READER_CHECK_FORWARD_CHAR;

    l = detail::get_token(work_ptr, length);
    if (!detail::expected_token(work_ptr, l, expected, std::strlen(expected)))
      return false;
    IGUANA_XML_READER_CHECK_FORWARD;

    l = detail::forward_after(work_ptr, length,
                              detail::char_const::anti_angle_bracket);
    IGUANA_XML_READER_CHECK_FORWARD;

    buffer_ = work_ptr;
    length_ = length;

    if (detail::expected_char(work_ptr - 1, detail::char_const::slash))
      return object_status::EMPTY;
    return object_status::NORMAL;
  }

  template <typename T> bool get_value(T &t) {
    auto work_ptr = buffer_;
    auto length = length_;

    auto l = detail::ignore_blank_ctrl(work_ptr, length);
    IGUANA_XML_READER_CHECK_FORWARD;

    l = detail::forward_until(work_ptr, length,
                              detail::char_const::angle_bracket);
    try {
      detail::get_value(work_ptr, l, t);
    } catch (...) {
      return false;
    }
    IGUANA_XML_READER_CHECK_FORWARD;

    buffer_ = work_ptr;
    length_ = length;

    return true;
  }

  bool end_object(char const *expected) {
    auto work_ptr = buffer_;
    auto length = length_;

    auto l = detail::ignore_blank_ctrl(work_ptr, length);
    IGUANA_XML_READER_CHECK_FORWARD;

    if (!detail::expected_char(work_ptr, detail::char_const::angle_bracket))
      return false;
    IGUANA_XML_READER_CHECK_FORWARD_CHAR;

    if (!detail::expected_char(work_ptr, detail::char_const::slash))
      return false;
    IGUANA_XML_READER_CHECK_FORWARD_CHAR;

    l = detail::get_token(work_ptr, length);
    if (!detail::expected_token(work_ptr, l, expected, std::strlen(expected)))
      return false;
    IGUANA_XML_READER_CHECK_FORWARD;

    if (!detail::expected_char(work_ptr,
                               detail::char_const::anti_angle_bracket))
      return false;
    IGUANA_XML_READER_CHECK_FORWARD_CHAR;

    buffer_ = work_ptr;
    length_ = length;
    return true;
  }

  static const size_t xml_header_length =
      detail::array_size<decltype(IGUANA_XML_HEADER)>::value - 1;

private:
  char const *buffer_;
  size_t length_;
};

// to xml
template <typename Stream, typename T>
std::enable_if_t<!std::is_floating_point<T>::value &&
                 (std::is_integral<T>::value || std::is_unsigned<T>::value ||
                  std::is_signed<T>::value)>
render_xml_value(Stream &ss, T value) {
  char temp[20];
  auto p = itoa_fwd(value, temp);
  ss.append(temp, p - temp);
}

template <typename Stream, typename T>
std::enable_if_t<std::is_floating_point<T>::value> render_xml_value(Stream &ss,
                                                                    T value) {
  char temp[20];
  sprintf(temp, "%f", value);
  ss.append(temp);
}

template <typename Stream>
void render_xml_value(Stream &ss, const std::string &s) {
  ss.append(s.c_str(), s.size());
}

template <typename Stream> void render_xml_value(Stream &ss, const char *s) {
  ss.append(s, strlen(s));
}

template <typename Stream, typename T>
std::enable_if_t<std::is_arithmetic<T>::value> render_key(Stream &ss, T t) {
  ss.push_back('<');
  render_xml_value(ss, t);
  ss.push_back('>');
}

template <typename Stream> void render_key(Stream &ss, const std::string &s) {
  render_xml_value(ss, s);
}

template <typename Stream> void render_key(Stream &ss, const char *s) {
  render_xml_value(ss, s);
}

template <typename Stream> void render_tail(Stream &ss, const char *s) {
  ss.push_back('<');
  ss.push_back('/');
  ss.append(s, strlen(s));
  ss.push_back('>');
}

template <typename Stream> void render_head(Stream &ss, const char *s) {
  ss.push_back('<');
  ss.append(s, strlen(s));
  ss.push_back('>');
}

template <typename Stream, typename T,
          typename = std::enable_if_t<is_reflection<T>::value>>
void to_xml_impl(Stream &s, T &&t) {
  for_each(std::forward<T>(t), [&t, &s](const auto v, auto i) {
    using M = decltype(iguana_reflect_members(std::forward<T>(t)));
    constexpr auto Idx = decltype(i)::value;
    constexpr auto Count = M::value();
    static_assert(Idx < Count);

    using type_v = decltype(std::declval<T>().*std::declval<decltype(v)>());
    if constexpr (!is_reflection<type_v>::value) {
      render_head(s, get_name<T, Idx>().data());
      render_xml_value(s, t.*v);
      render_tail(s, get_name<T, Idx>().data());
    } else {
      render_head(s, get_name<T, Idx>().data());
      to_xml_impl(s, t.*v);
      render_tail(s, get_name<T, Idx>().data());
    }
  });
}

template <typename Stream, typename T,
          typename = std::enable_if_t<is_reflection<T>::value>>
void to_xml(Stream &s, T &&t) {
  // render_head(s, "xml");
  s.append(IGUANA_XML_HEADER, xml_reader_t::xml_header_length);
  to_xml_impl(s, std::forward<T>(t));
}

template <typename T, typename = std::enable_if_t<is_reflection<T>::value>>
constexpr void do_read(xml_reader_t &rd, T &&t) {
  for_each(std::forward<T>(t), [&t, &rd](const auto v, auto i) {
    using M = decltype(iguana_reflect_members(std::forward<T>(t)));
    constexpr auto Idx = decltype(i)::value;
    constexpr auto Count = M::value();
    static_assert(Idx < Count);

    using type_v = decltype(std::declval<T>().*std::declval<decltype(v)>());
    if constexpr (!is_reflection<type_v>::value) {
      if (rd.begin_object(get_name<T, Idx>().data()) == 1) {
        // read value
        rd.get_value(t.*v);
        rd.end_object(get_name<T, Idx>().data());
      }
    } else {
      if (rd.begin_object(get_name<T, Idx>().data()) == 1) {
        do_read(rd, t.*v);
        rd.end_object(get_name<T, Idx>().data());
      }
    }
  });
}

template <typename T, typename = std::enable_if_t<is_reflection<T>::value>>
void from_xml(T &&t, const char *buf, size_t len = -1) {
  xml_reader_t rd(buf, len);
  if (rd.get_root()) {
    do_read(rd, t);
  }
}
} // namespace iguana::xml
#endif // IGUANA_XML17_HPP
