//
// Created by Qiyu on 17-6-5.
//

#ifndef SERIALIZE_JSON_HPP
#define SERIALIZE_JSON_HPP
#include "json_util.hpp"

namespace iguana {

template <typename Stream, typename T,
          std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE void to_json(T &&t, Stream &s);

template <typename Stream, typename T>
IGUANA_INLINE void render_json_value(Stream &ss, std::optional<T> &val);

template <typename Stream, typename T,
          std::enable_if_t<fixed_array_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, const T &t);

template <typename Stream, typename T,
          std::enable_if_t<sequence_container_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, const T &v);

template <typename Stream, typename T,
          std::enable_if_t<smart_ptr_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, const T &v);

template <typename Stream, typename T,
          std::enable_if_t<map_container_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, const T &o);

template <typename Stream, typename T, std::enable_if_t<tuple_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &s, T &&t);

template <typename Stream, typename T, std::enable_if_t<variant_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &s, T &&t);

template <typename Stream, typename InputIt, typename T, typename F>
IGUANA_INLINE void join(Stream &ss, InputIt first, InputIt last, const T &delim,
                        const F &f) {
  if (first == last)
    return;

  f(*first++);
  while (first != last) {
    ss.push_back(delim);
    f(*first++);
  }
}

template <typename Stream>
IGUANA_INLINE void render_json_value(Stream &ss, std::nullptr_t) {
  ss.append("null");
}

template <typename Stream>
IGUANA_INLINE void render_json_value(Stream &ss, bool b) {
  ss.append(b ? "true" : "false");
};

template <typename Stream>
IGUANA_INLINE void render_json_value(Stream &ss, char value) {
  ss.append("\"");
  ss.push_back(value);
  ss.append("\"");
}

template <typename Stream, typename T, std::enable_if_t<num_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, T value) {
  char temp[65];
  auto p = detail::to_chars(temp, value);
  ss.append(temp, p - temp);
}

template <typename Stream, typename T,
          std::enable_if_t<numeric_str_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, T v) {
  ss.append(v.value().data(), v.value().size());
}

template <typename Stream, typename T,
          std::enable_if_t<string_container_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, T &&t) {
  ss.push_back('"');
  ss.append(t.data(), t.size());
  ss.push_back('"');
}

template <typename Stream, typename T, std::enable_if_t<num_v<T>, int> = 0>
IGUANA_INLINE void render_key(Stream &ss, T &t) {
  ss.push_back('"');
  render_json_value(ss, t);
  ss.push_back('"');
}

template <typename Stream, typename T,
          std::enable_if_t<string_container_v<T>, int> = 0>
IGUANA_INLINE void render_key(Stream &ss, T &&t) {
  render_json_value(ss, std::forward<T>(t));
}

template <typename Stream, typename T,
          std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, T &&t) {
  to_json(std::forward<T>(t), ss);
}

template <typename Stream, typename T, std::enable_if_t<enum_v<T>, int> = 0>
IGUANA_INLINE void render_json_value(Stream &ss, T val) {
  static constexpr auto enum_to_str = get_enum_map<false, std::decay_t<T>>();
  if constexpr (bool_v<decltype(enum_to_str)>) {
    render_json_value(ss, static_cast<std::underlying_type_t<T>>(val));
  }
  else {
    auto it = enum_to_str.find(val);
    if (it != enum_to_str.end())
      IGUANA_LIKELY {
        auto str = it->second;
        render_json_value(ss, std::string_view(str.data(), str.size()));
      }
    else {
      throw std::runtime_error(
          std::to_string(static_cast<std::underlying_type_t<T>>(val)) +
          " is a missing value in enum_value");
    }
  }
}

template <typename Stream, typename T>
IGUANA_INLINE void render_json_value(Stream &ss, std::optional<T> &val) {
  if (!val) {
    ss.append("null");
  }
  else {
    render_json_value(ss, *val);
  }
}

template <typename Stream, typename T>
IGUANA_INLINE void render_array(Stream &ss, const T &v) {
  ss.push_back('[');
  join(ss, std::begin(v), std::end(v), ',',
       [&ss](const auto &jsv) IGUANA__INLINE_LAMBDA {
         render_json_value(ss, jsv);
       });
  ss.push_back(']');
}

template <typename Stream, typename T, std::enable_if_t<fixed_array_v<T>, int>>
IGUANA_INLINE void render_json_value(Stream &ss, const T &t) {
  if constexpr (std::is_same_v<char, std::remove_reference_t<
                                         decltype(std::declval<T>()[0])>>) {
    constexpr size_t n = sizeof(T) / sizeof(decltype(std::declval<T>()[0]));
    ss.push_back('"');
    auto get_length = [&t](int n) constexpr {
      for (int i = 0; i < n; ++i) {
        if (t[i] == '\0')
          return i;
      }
      return n;
    };
    size_t len = get_length(n);
    ss.append(std::begin(t), len);
    ss.push_back('"');
  }
  else {
    render_array(ss, t);
  }
}

template <typename Stream, typename T,
          std::enable_if_t<map_container_v<T>, int>>
IGUANA_INLINE void render_json_value(Stream &ss, const T &o) {
  ss.push_back('{');
  join(ss, o.cbegin(), o.cend(), ',',
       [&ss](const auto &jsv) IGUANA__INLINE_LAMBDA {
         render_key(ss, jsv.first);
         ss.push_back(':');
         render_json_value(ss, jsv.second);
       });
  ss.push_back('}');
}

template <typename Stream, typename T,
          std::enable_if_t<sequence_container_v<T>, int>>
IGUANA_INLINE void render_json_value(Stream &ss, const T &v) {
  ss.push_back('[');
  join(ss, v.cbegin(), v.cend(), ',',
       [&ss](const auto &jsv) IGUANA__INLINE_LAMBDA {
         render_json_value(ss, jsv);
       });
  ss.push_back(']');
}

constexpr auto write_json_key = [](auto &s, auto i,
                                   auto &t) IGUANA__INLINE_LAMBDA {
  s.push_back('"');
  // will be replaced by string_view later
  constexpr auto name = get_name<decltype(t), decltype(i)::value>();
  s.append(name.data(), name.size());
  s.push_back('"');
};

template <typename Stream, typename T, std::enable_if_t<smart_ptr_v<T>, int>>
IGUANA_INLINE void render_json_value(Stream &ss, const T &v) {
  if (v) {
    render_json_value(ss, *v);
  }
  else {
    ss.append("null");
  }
}

template <typename Stream, typename T, std::enable_if_t<tuple_v<T>, int>>
IGUANA_INLINE void render_json_value(Stream &s, T &&t) {
  using U = typename std::decay_t<T>;
  s.push_back('[');
  constexpr size_t size = std::tuple_size_v<U>;
  for_each(std::forward<T>(t),
           [&s, size](auto &v, auto i) IGUANA__INLINE_LAMBDA {
             render_json_value(s, v);

             if (i != size - 1)
               IGUANA_LIKELY { s.push_back(','); }
           });
  s.push_back(']');
}

template <typename Stream, typename T, std::enable_if_t<variant_v<T>, int>>
IGUANA_INLINE void render_json_value(Stream &s, T &&t) {
  std::visit(
      [&s](auto value) {
        render_json_value(s, value);
      },
      t);
}

template <typename Stream, typename T, std::enable_if_t<refletable_v<T>, int>>
IGUANA_INLINE void to_json(T &&t, Stream &s) {
  s.push_back('{');
  for_each(std::forward<T>(t),
           [&t, &s](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_type(std::forward<T>(t)));
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             static_assert(Idx < Count);

             write_json_key(s, i, t);
             s.push_back(':');
             render_json_value(s, t.*v);
             if (Idx < Count - 1)
               IGUANA_LIKELY { s.push_back(','); }
           });
  s.push_back('}');
}

template <typename Stream, typename T,
          std::enable_if_t<non_refletable_v<T>, int> = 0>
IGUANA_INLINE void to_json(T &&t, Stream &s) {
  render_json_value(s, t);
}

}  // namespace iguana
#endif  // SERIALIZE_JSON_HPP
