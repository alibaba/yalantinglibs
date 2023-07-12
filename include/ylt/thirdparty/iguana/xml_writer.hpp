#pragma once

#include "detail/charconv.h"
#include "reflection.hpp"
#include "xml_util.hpp"

namespace iguana {

template <typename Stream, refletable T>
IGUANA_INLINE void render_xml_value(Stream &ss, T &&t, std::string_view name);

template <typename Stream>
inline void render_tail(Stream &ss, std::string_view str) {
  ss.push_back('<');
  ss.push_back('/');
  ss.append(str.data(), str.size());
  ss.push_back('>');
}

template <typename Stream>
inline void render_head(Stream &ss, std::string_view str) {
  ss.push_back('<');
  ss.append(str.data(), str.size());
  ss.push_back('>');
}

template <typename Stream, plain_t T>
IGUANA_INLINE void render_value(Stream &ss, const T &value) {
  if constexpr (string_t<T>) {
    ss.append(value.data(), value.size());
  }
  else if constexpr (num_t<T>) {
    char temp[65];
    auto p = detail::to_chars(temp, value);
    ss.append(temp, p - temp);
  }
  else if constexpr (char_t<T>) {
    ss.push_back(value);
  }
  else if constexpr (bool_t<T>) {
    ss.append(value ? "true" : "false");
  }
  else if constexpr (enum_t<T>) {
    render_value(ss, static_cast<std::underlying_type_t<T>>(value));
  }
  else {
    static_assert(!sizeof(T), "type is not supported");
  }
}

template <typename Stream, map_container T>
inline void render_xml_attr(Stream &ss, const T &value, std::string_view name) {
  ss.push_back('<');
  ss.append(name.data(), name.size());
  for (auto [k, v] : value) {
    ss.push_back(' ');
    render_value(ss, k);
    ss.push_back('=');
    ss.push_back('"');
    render_value(ss, v);
    ss.push_back('"');
  }
  ss.push_back('>');
}

template <typename Stream, attr_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_xml_attr(ss, value.attr(), name);
  render_xml_value(ss, value.value(), name);
  render_tail(ss, name);
}

template <typename Stream, plain_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_value(ss, value);
  render_tail(ss, name);
}

template <typename Stream, optional_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  if (value) {
    render_xml_value(ss, *value, name);
  }
  else {
    render_tail(ss, name);
  }
}

template <typename Stream, unique_ptr_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  if (value) {
    render_xml_value(ss, *value, name);
  }
  else {
    render_tail(ss, name);
  }
}

template <typename Stream, sequence_container_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  using value_type = typename std::remove_cvref_t<T>::value_type;
  for (const auto &v : value) {
    if constexpr (attr_t<value_type>) {
      render_xml_attr(ss, v.attr(), name);
      render_xml_value(ss, v.value(), name);
    }
    else {
      render_head(ss, name);
      render_xml_value(ss, v, name);
    }
  }
}

template <typename Stream, refletable T>
IGUANA_INLINE void render_xml_value(Stream &ss, T &&t, std::string_view name) {
  for_each(std::forward<T>(t),
           [&](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_members(std::forward<T>(t)));
             using value_type = std::remove_cvref_t<decltype(t.*v)>;
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             constexpr std::string_view tag_name =
                 std::string_view(get_name<std::decay_t<T>, Idx>().data(),
                                  get_name<std::decay_t<T>, Idx>().size());
             static_assert(Idx < Count);
             if constexpr (sequence_container_t<value_type>) {
               render_xml_value(ss, t.*v, tag_name);
             }
             else if constexpr (attr_t<value_type>) {
               render_xml_attr(ss, (t.*v).attr(), tag_name);
               render_xml_value(ss, (t.*v).value(), tag_name);
             }
             else if constexpr (cdata_t<value_type>) {
               ss.append("<![CDATA[").append((t.*v).value()).append("]]>");
             }
             else {
               render_head(ss, tag_name);
               render_xml_value(ss, t.*v, tag_name);
             }
           });
  render_tail(ss, name);
}

template <typename Stream, attr_t T>
IGUANA_INLINE void to_xml(T &&t, Stream &s) {
  using value_type = typename std::decay_t<T>::value_type;
  constexpr std::string_view root_name = std::string_view(
      get_name<value_type>().data(), get_name<value_type>().size());
  render_xml_attr(s, std::forward<T>(t).attr(), root_name);
  render_xml_value(s, std::forward<T>(t).value(), root_name);
}

template <typename Stream, refletable T>
IGUANA_INLINE void to_xml(T &&t, Stream &s) {
  constexpr std::string_view root_name = std::string_view(
      get_name<std::decay_t<T>>().data(), get_name<std::decay_t<T>>().size());
  render_head(s, root_name);
  render_xml_value(s, std::forward<T>(t), root_name);
}

}  // namespace iguana