#pragma once

#include "detail/charconv.h"
#include "yaml_util.hpp"

namespace iguana {

template <typename Stream, typename T,
          std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE void to_yaml(T &&t, Stream &s, size_t min_spaces = 0);

template <typename Stream, typename T,
          std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T &&t, size_t min_spaces) {
  ss.push_back('\n');
  to_yaml(std::forward<T>(t), ss, min_spaces);
}

// TODO: support more string style, support escape
template <bool appendLf = true, typename Stream, typename T,
          std::enable_if_t<string_container_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T &&t, size_t min_spaces) {
  ss.append(t.data(), t.size());
  if constexpr (appendLf)
    ss.push_back('\n');
}

template <bool appendLf = true, typename Stream, typename T,
          std::enable_if_t<num_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T value, size_t min_spaces) {
  char temp[65];
  auto p = detail::to_chars(temp, value);
  ss.append(temp, p - temp);
  if constexpr (appendLf)
    ss.push_back('\n');
}

template <bool appendLf = true, typename Stream>
IGUANA_INLINE void render_yaml_value(Stream &ss, char value,
                                     size_t min_spaces) {
  ss.push_back(value);
  if constexpr (appendLf)
    ss.push_back('\n');
}

template <bool appendLf = true, typename Stream>
IGUANA_INLINE void render_yaml_value(Stream &ss, bool value,
                                     size_t min_spaces) {
  ss.append(value ? "true" : "false");
  if constexpr (appendLf)
    ss.push_back('\n');
}

template <bool appendLf = true, typename Stream, typename T,
          std::enable_if_t<enum_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T value, size_t min_spaces) {
  static constexpr auto enum_to_str = get_enum_map<false, std::decay_t<T>>();
  if constexpr (bool_v<decltype(enum_to_str)>) {
    render_yaml_value(ss, static_cast<std::underlying_type_t<T>>(value),
                      min_spaces);
  }
  else {
    auto it = enum_to_str.find(value);
    if (it != enum_to_str.end())
      IGUANA_LIKELY {
        auto str = it->second;
        render_yaml_value(ss, std::string_view(str.data(), str.size()),
                          min_spaces);
      }
    else {
      throw std::runtime_error(
          std::to_string(static_cast<std::underlying_type_t<T>>(value)) +
          " is a missing value in enum_value");
    }
  }
}

template <typename Stream, typename T, std::enable_if_t<optional_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &val,
                                     size_t min_spaces);

template <typename Stream, typename T,
          std::enable_if_t<smart_ptr_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &val,
                                     size_t min_spaces);

template <typename Stream, typename T,
          std::enable_if_t<sequence_container_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &t,
                                     size_t min_spaces) {
  ss.push_back('\n');
  for (const auto &v : t) {
    ss.append(min_spaces, ' ');
    ss.append("- ");
    render_yaml_value(ss, v, min_spaces + 1);
  }
}

template <typename Stream, typename T, std::enable_if_t<tuple_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T &&t, size_t min_spaces) {
  ss.push_back('\n');
  for_each(std::forward<T>(t),
           [&ss, min_spaces](auto &v, auto i) IGUANA__INLINE_LAMBDA {
             ss.append(min_spaces, ' ');
             ss.append("- ");
             render_yaml_value(ss, v, min_spaces + 1);
           });
}

template <typename Stream, typename T,
          std::enable_if_t<map_container_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &t,
                                     size_t min_spaces) {
  ss.push_back('\n');
  for (const auto &[k, v] : t) {
    ss.append(min_spaces, ' ');
    render_yaml_value<false>(ss, k, 0);  // key must be plaint type
    ss.append(": ");
    render_yaml_value(ss, v, min_spaces + 1);
  }
}

template <typename Stream, typename T, std::enable_if_t<optional_v<T>, int>>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &val,
                                     size_t min_spaces) {
  if (!val) {
    ss.append("null");
    ss.push_back('\n');
  }
  else {
    render_yaml_value(ss, *val, min_spaces);
  }
}

template <typename Stream, typename T, std::enable_if_t<smart_ptr_v<T>, int>>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &val,
                                     size_t min_spaces) {
  if (!val) {
    ss.push_back('\n');
  }
  else {
    render_yaml_value(ss, *val, min_spaces);
  }
}

constexpr auto write_yaml_key = [](auto &s, auto i,
                                   auto &t) IGUANA__INLINE_LAMBDA {
  constexpr auto name = get_name<decltype(t), decltype(i)::value>();
  s.append(name.data(), name.size());
};

template <typename Stream, typename T, std::enable_if_t<refletable_v<T>, int>>
IGUANA_INLINE void to_yaml(T &&t, Stream &s, size_t min_spaces) {
  for_each(std::forward<T>(t),
           [&t, &s, min_spaces](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_type(std::forward<T>(t)));
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             static_assert(Idx < Count);
             s.append(min_spaces, ' ');
             write_yaml_key(s, i, t);
             s.append(": ");
             if constexpr (!is_reflection<std::decay_t<decltype(v)>>::value) {
               render_yaml_value(s, t.*v, min_spaces + 1);
             }
             else {
               s.push_back('\n');
               to_yaml(t.*v, s, min_spaces + 1);
             }
           });
}

template <typename Stream, typename T,
          std::enable_if_t<non_refletable_v<T>, int> = 0>
IGUANA_INLINE void to_yaml(T &&t, Stream &s) {
  if constexpr (tuple_v<T> || map_container_v<T> || sequence_container_v<T> ||
                optional_v<T> || smart_ptr_v<T>)
    render_yaml_value(s, std::forward<T>(t), 0);
  else
    static_assert(!sizeof(T), "don't suppport this type");
}

}  // namespace iguana
