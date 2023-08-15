#pragma once

#include "detail/charconv.h"
#include "reflection.hpp"
#include "xml_util.hpp"

namespace iguana {

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<sequence_container_v<T>, int> = 0>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name);

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE void render_xml_value(Stream &ss, T &&t, std::string_view name);

template <bool pretty, size_t spaces, typename Stream>
IGUANA_INLINE void render_tail(Stream &ss, std::string_view str) {
  if constexpr (pretty) {
    ss.append(spaces, '\t');
  }
  ss.push_back('<');
  ss.push_back('/');
  ss.append(str.data(), str.size());
  ss.push_back('>');
  if constexpr (pretty) {
    ss.push_back('\n');
  }
}

template <bool pretty, size_t spaces, typename Stream>
IGUANA_INLINE void render_head(Stream &ss, std::string_view str) {
  if constexpr (pretty) {
    ss.append(spaces, '\t');
  }
  ss.push_back('<');
  ss.append(str.data(), str.size());
  ss.push_back('>');
}

template <typename Stream, typename T, std::enable_if_t<plain_v<T>, int> = 0>
IGUANA_INLINE void render_value(Stream &ss, const T &value) {
  if constexpr (string_container_v<T>) {
    ss.append(value.data(), value.size());
  }
  else if constexpr (num_v<T>) {
    char temp[65];
    auto p = detail::to_chars(temp, value);
    ss.append(temp, p - temp);
  }
  else if constexpr (char_v<T>) {
    ss.push_back(value);
  }
  else if constexpr (bool_v<T>) {
    ss.append(value ? "true" : "false");
  }
  else if constexpr (enum_v<T>) {
    static constexpr auto enum_to_str = get_enum_map<false, std::decay_t<T>>();
    if constexpr (bool_v<decltype(enum_to_str)>) {
      render_value(ss, static_cast<std::underlying_type_t<T>>(value));
    }
    else {
      auto it = enum_to_str.find(value);
      if (it != enum_to_str.end())
        IGUANA_LIKELY {
          auto str = it->second;
          render_value(ss, std::string_view(str.data(), str.size()));
        }
      else {
        throw std::runtime_error(
            std::to_string(static_cast<std::underlying_type_t<T>>(value)) +
            " is a missing value in enum_value");
      }
    }
  }
  else {
    static_assert(!sizeof(T), "type is not supported");
  }
}

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<map_container_v<T>, int> = 0>
inline void render_xml_attr(Stream &ss, const T &value, std::string_view name) {
  if constexpr (pretty) {
    ss.append(spaces, '\t');
  }
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

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<plain_v<T>, int> = 0>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_value(ss, value);
  render_tail<pretty, 0>(ss, name);
}

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<optional_v<T>, int> = 0>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  if (value) {
    render_xml_value<pretty, spaces>(ss, *value, name);
  }
  else {
    render_tail<pretty, 0>(ss, name);
  }
}

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<smart_ptr_v<T>, int> = 0>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  if (value) {
    render_xml_value<pretty, spaces>(ss, *value, name);
  }
  else {
    render_tail<pretty, 0>(ss, name);
  }
}

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<attr_v<T>, int> = 0>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_xml_attr<pretty, spaces>(ss, value.attr(), name);
  render_xml_value<pretty, spaces>(ss, value.value(), name);
}

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<sequence_container_v<T>, int>>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  using value_type =
      underline_type_t<typename std::remove_cvref_t<T>::value_type>;
  for (const auto &v : value) {
    if constexpr (attr_v<value_type>) {
      render_xml_value<pretty, spaces>(ss, v, name);
    }
    else {
      render_head<pretty, spaces>(ss, name);
      render_xml_value<pretty, spaces>(ss, v, name);
    }
  }
}

template <bool pretty, size_t spaces, typename Stream, typename T,
          std::enable_if_t<refletable_v<T>, int>>
IGUANA_INLINE void render_xml_value(Stream &ss, T &&t, std::string_view name) {
  if constexpr (pretty) {
    ss.push_back('\n');
  }
  for_each(std::forward<T>(t),
           [&](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_type(std::forward<T>(t)));
             using value_type = underline_type_t<decltype(t.*v)>;
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             constexpr std::string_view tag_name =
                 std::string_view(get_name<std::decay_t<T>, Idx>().data(),
                                  get_name<std::decay_t<T>, Idx>().size());
             static_assert(Idx < Count);
             if constexpr (sequence_container_v<value_type>) {
               render_xml_value<pretty, spaces + 1>(ss, t.*v, tag_name);
             }
             else if constexpr (attr_v<value_type>) {
               render_xml_value<pretty, spaces + 1>(ss, t.*v, tag_name);
             }
             else if constexpr (cdata_v<value_type>) {
               if constexpr (pretty) {
                 ss.append(spaces + 1, '\t');
                 ss.append("<![CDATA[").append((t.*v).value()).append("]]>\n");
               }
               else {
                 ss.append("<![CDATA[").append((t.*v).value()).append("]]>");
               }
             }
             else {
               render_head<pretty, spaces + 1>(ss, tag_name);
               render_xml_value<pretty, spaces + 1>(ss, t.*v, tag_name);
             }
           });
  render_tail<pretty, spaces>(ss, name);
}

template <bool pretty = false, typename Stream, typename T,
          std::enable_if_t<attr_v<T>, int> = 0>
IGUANA_INLINE void to_xml(T &&t, Stream &s) {
  using value_type = typename std::decay_t<T>::value_type;
  static_assert(refletable_v<value_type>, "value_type must be refletable");
  constexpr std::string_view root_name = std::string_view(
      get_name<value_type>().data(), get_name<value_type>().size());
  render_xml_value<pretty, 0>(s, std::forward<T>(t), root_name);
}

template <bool pretty = false, typename Stream, typename T,
          std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE void to_xml(T &&t, Stream &s) {
  constexpr std::string_view root_name = std::string_view(
      get_name<std::decay_t<T>>().data(), get_name<std::decay_t<T>>().size());
  render_head<pretty, 0>(s, root_name);
  render_xml_value<pretty, 0>(s, std::forward<T>(t), root_name);
}

}  // namespace iguana
