#pragma once
#include <charconv>

#include "detail/charconv.h"
#include "detail/utf.hpp"
#include "xml_util.hpp"

namespace iguana {
namespace detail {

template <typename U, typename It, std::enable_if_t<optional_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name);

template <typename U, typename It, std::enable_if_t<smart_ptr_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name);

template <typename U, typename It, std::enable_if_t<refletable_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name);

template <typename U, typename It, std::enable_if_t<attr_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name);

template <typename U, typename It, std::enable_if_t<plain_v<U>, int> = 0>
IGUANA_INLINE void parse_value(U &&value, It &&begin, It &&end) {
  using T = std::decay_t<U>;
  if constexpr (string_container_v<T>) {
    value = T(&*begin, static_cast<size_t>(std::distance(begin, end)));
  }
  else if constexpr (num_v<T>) {
    auto size = std::distance(begin, end);
    const auto start = &*begin;
    auto [p, ec] = detail::from_chars(start, start + size, value);
    if (ec != std::errc{})
      IGUANA_UNLIKELY
    throw std::runtime_error("Failed to parse number");
  }
  else if constexpr (char_v<T>) {
    if (static_cast<size_t>(std::distance(begin, end)) != 1)
      IGUANA_UNLIKELY { throw std::runtime_error("Expected one character"); }
    value = *begin;
  }
  else if constexpr (bool_v<T>) {
    auto bool_v = std::string_view(
        &*begin, static_cast<size_t>(std::distance(begin, end)));
    if (bool_v == "true") {
      value = true;
    }
    else if (bool_v == "false") {
      value = false;
    }
    else
      IGUANA_UNLIKELY { throw std::runtime_error("Expected true or false"); }
  }
  else if constexpr (enum_v<T>) {
    static constexpr auto str_to_enum = get_enum_map<true, T>();
    if constexpr (bool_v<decltype(str_to_enum)>) {
      // not defined a specialization template
      parse_value(reinterpret_cast<std::underlying_type_t<T> &>(value), begin,
                  end);
    }
    else {
      auto enum_names = std::string_view(
          &*begin, static_cast<size_t>(std::distance(begin, end)));
      auto it = str_to_enum.find(enum_names);
      if (it != str_to_enum.end())
        IGUANA_LIKELY { value = it->second; }
      else {
        throw std::runtime_error(std::string(enum_names) +
                                 " missing corresponding value in enum_value");
      }
    }
  }
}
template <typename U, typename It,
          std::enable_if_t<map_container_v<U>, int> = 0>
IGUANA_INLINE void parse_attr(U &&value, It &&it, It &&end) {
  using key_type = typename std::remove_cvref_t<U>::key_type;
  using value_type = typename std::remove_cvref_t<U>::mapped_type;
  while (it != end) {
    skip_sapces_and_newline(it, end);
    if (*it == '>' || *it == '/')
      IGUANA_UNLIKELY { return; }
    auto key_begin = it;
    auto key_end = skip_pass<'='>(it, end);
    key_type key;
    parse_value(key, key_begin, key_end);

    skip_sapces_and_newline(it, end);
    match<'"'>(it, end);
    auto value_begin = it;
    auto value_end = skip_pass<'"'>(it, end);
    value_type v;
    parse_value(v, value_begin, value_end);
    value.emplace(std::move(key), std::move(v));
  }
}

template <typename U, typename It, std::enable_if_t<plain_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  skip_till<'>'>(it, end);
  ++it;
  skip_sapces_and_newline(it, end);
  auto value_begin = it;
  auto value_end = skip_pass<'<'>(it, end);
  parse_value(value, value_begin, value_end);
  match_close_tag(it, end, name);
}

template <typename U, typename It,
          std::enable_if_t<sequence_container_v<U>, int> = 0>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  parse_item(value.emplace_back(), it, end, name);
  skip_sapces_and_newline(it, end);
  while (it != end) {
    match<'<'>(it, end);
    if (*it == '?' || *it == '!')
      IGUANA_UNLIKELY {
        // skip <?
        if (*(it + 1) == '[') {
          --it;
          return;
        }
        else {
          skip_till<'>'>(it, end);
          ++it;
          skip_sapces_and_newline(it, end);
          continue;
        }
      }
    auto start = it;
    skip_till_greater_or_space(it, end);
    std::string_view key = std::string_view{
        &*start, static_cast<size_t>(std::distance(start, it))};
    if (key != name)
      IGUANA_UNLIKELY {
        it = start - 1;
        return;
      }
    parse_item(value.emplace_back(), it, end, name);
    skip_sapces_and_newline(it, end);
  }
}

template <typename U, typename It, std::enable_if_t<optional_v<U>, int>>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  using value_type = typename std::remove_reference_t<U>::value_type;
  skip_till<'>'>(it, end);
  if (*(it - 1) == '/')
    IGUANA_LIKELY {
      ++it;
      return;
    }

  if constexpr (plain_v<value_type>) {
    // The following code is for option not to be emplaced
    // when parse  "...> <..."
    skip_till<'>'>(it, end);
    ++it;
    skip_sapces_and_newline(it, end);
    auto value_begin = it;
    auto value_end = skip_pass<'<'>(it, end);
    if (value_begin == value_end) {
      match_close_tag(it, end, name);
      return;
    }
    parse_value(value.emplace(), value_begin, value_end);
    match_close_tag(it, end, name);
  }
  else {
    parse_item(value.emplace(), it, end, name);
  }
}

template <typename U, typename It, std::enable_if_t<smart_ptr_v<U>, int>>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  if constexpr (unique_ptr_v<U>) {
    value = std::make_unique<typename std::remove_cvref_t<U>::element_type>();
  }
  else {
    value = std::make_shared<typename std::remove_cvref_t<U>::element_type>();
  }

  parse_item(*value, it, end, name);
}

template <typename U, typename It, std::enable_if_t<attr_v<U>, int>>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  parse_attr(value.attr(), it, end);
  parse_item(value.value(), it, end, name);
}

//  /> or skip <?>、 <!> and <tag></tag> until the </name>
//  loose inspection here
template <typename It>
IGUANA_INLINE void skip_object_value(It &&it, It &&end, std::string_view name) {
  skip_till<'>'>(it, end);
  ++it;
  if (*(it - 2) == '/') {
    // .../>
    return;
  }
  // </name>
  size_t size = name.size();
  while (it != end) {
    skip_till<'>'>(it, end);
    if (*(it - size - 2) == '<' && *(it - size - 1) == '/' &&
        (std::string_view{&*(it - size), size} == name)) {
      ++it;
      return;
    }
    ++it;
    skip_sapces_and_newline(it, end);
  }
  throw std::runtime_error("unclosed tag: " + std::string(name));
}

// return true means reach the close tag
template <size_t cdata_idx, typename T, typename It,
          std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE auto skip_till_key(T &value, It &&it, It &&end) {
  skip_sapces_and_newline(it, end);
  while (true) {
    match<'<'>(it, end);
    if (*it == '/')
      IGUANA_UNLIKELY {
        // </tag>
        return true;  // reach the close tag
      }
    else if (*it == '?')
      IGUANA_UNLIKELY {
        // <? ... ?>
        skip_till<'>'>(it, end);
        ++it;
        skip_sapces_and_newline(it, end);
        continue;
      }
    else if (*it == '!')
      IGUANA_UNLIKELY {
        ++it;
        if (*it == '[') {
          // <![
          if constexpr (cdata_idx == iguana::get_value<std::decay_t<T>>()) {
            ++it;
            skip_till<']'>(it, end);
            ++it;
            match<']', '>'>(it, end);
            skip_sapces_and_newline(it, end);
            continue;
          }
          else {
            // if parse cdata
            ++it;
            match<'C', 'D', 'A', 'T', 'A', '['>(it, end);
            skip_sapces_and_newline(it, end);
            auto &cdata_value = get<cdata_idx>(value);
            using VT = typename std::decay_t<decltype(cdata_value)>::value_type;
            auto vb = it;
            auto ve = skip_pass<']'>(it, end);
            if constexpr (string_view_v<VT>) {
              cdata_value.value() =
                  VT(&*vb, static_cast<size_t>(std::distance(vb, ve)));
            }
            else {
              cdata_value.value().append(
                  &*vb, static_cast<size_t>(std::distance(vb, ve)));
            }
            match<']', '>'>(it, end);
            skip_sapces_and_newline(it, end);
            continue;
          }
        }
        else {
          // <!-- -->
          // <!D
          skip_till<'>'>(it, end);
          ++it;
          skip_sapces_and_newline(it, end);
          continue;
        }
      }
    return false;
  }
}

template <typename T>
IGUANA_INLINE void check_required(std::string_view key_set) {
  if constexpr (iguana::has_iguana_required_arr_v<T>) {
    constexpr auto required_arr =
        iguana::iguana_required_struct<T>::requied_arr();
    for (auto &item : required_arr) {
      if (key_set.find(item) == std::string_view::npos) {
        std::string err = "required filed ";
        err.append(item).append(" not found!");
        throw std::invalid_argument(err);
      }
    }
  }
}

template <typename T, typename It, std::enable_if_t<refletable_v<T>, int>>
IGUANA_INLINE void parse_item(T &value, It &&it, It &&end,
                              std::string_view name) {
  using U = std::decay_t<T>;
  constexpr auto cdata_idx = get_type_index<is_cdata_t, U>();
  skip_till<'>'>(it, end);
  ++it;
  if (skip_till_key<cdata_idx>(value, it, end)) {
    match_close_tag(it, end, name);
    return;
  }
  auto start = it;
  skip_till_greater_or_space(it, end);
  std::string_view key =
      std::string_view{&*start, static_cast<size_t>(std::distance(start, it))};

  [[maybe_unused]] std::string key_set;
  bool parse_done = false;
  // sequential parse
  for_each(value, [&](const auto member_ptr, auto i) IGUANA__INLINE_LAMBDA {
#if defined(_MSC_VER) && _MSVC_LANG < 202002L
    // seems MVSC can't pass a constexpr value to lambda
    constexpr auto cdata_idx = get_type_index<is_cdata_t, U>();
#endif
    using item_type = std::remove_reference_t<decltype(value.*member_ptr)>;
    constexpr auto mkey = iguana::get_name<T, decltype(i)::value>();
    constexpr std::string_view st_key(mkey.data(), mkey.size());
    if constexpr (cdata_v<item_type>) {
      return;
    }
    if (parse_done || key != st_key)
      IGUANA_UNLIKELY { return; }
    if constexpr (!cdata_v<item_type>) {
      parse_item(value.*member_ptr, it, end, key);
      if constexpr (iguana::has_iguana_required_arr_v<U>) {
        key_set.append(key).append(", ");
      }
    }
    if (skip_till_key<cdata_idx>(value, it, end))
      IGUANA_UNLIKELY {
        match_close_tag(it, end, name);
        parse_done = true;
        return;
      }
    start = it;
    skip_till_greater_or_space(it, end);
    key = std::string_view{&*start,
                           static_cast<size_t>(std::distance(start, it))};
  });
  if (parse_done)
    IGUANA_UNLIKELY {
      check_required<U>(key_set);
      return;
    }
  // map parse
  while (true) {
    static constexpr auto frozen_map = get_iguana_struct_map<T>();
    const auto &member_it = frozen_map.find(key);
    if (member_it != frozen_map.end())
      IGUANA_LIKELY {
        std::visit(
            [&](auto &&member_ptr) IGUANA__INLINE_LAMBDA {
              static_assert(
                  std::is_member_pointer_v<std::decay_t<decltype(member_ptr)>>,
                  "type must be memberptr");
              using V = std::remove_reference_t<decltype(value.*member_ptr)>;
              if constexpr (!cdata_v<V>) {
                parse_item(value.*member_ptr, it, end, key);
                if constexpr (iguana::has_iguana_required_arr_v<U>) {
                  key_set.append(key).append(", ");
                }
              }
            },
            member_it->second);
      }
    else
      IGUANA_UNLIKELY {
#ifdef THROW_UNKNOWN_KEY
        throw std::runtime_error("Unknown key: " + std::string(key));
#else
        skip_object_value(it, end, key);
#endif
      }
    if (skip_till_key<cdata_idx>(value, it, end)) {
      match_close_tag(it, end, name);
      check_required<U>(key_set);
      return;
    }
    start = it;
    skip_till_greater_or_space(it, end);
    key = std::string_view{&*start,
                           static_cast<size_t>(std::distance(start, it))};
  }
}

}  // namespace detail

template <typename It, typename U, std::enable_if_t<attr_v<U>, int> = 0>
IGUANA_INLINE void from_xml(U &value, It &&it, It &&end) {
  while (it != end) {
    skip_sapces_and_newline(it, end);
    match<'<'>(it, end);
    if (*it == '?') {
      skip_till<'>'>(it, end);
      ++it;
    }
    else {
      break;
    }
  }
  auto start = it;
  skip_till_greater_or_space(it, end);
  std::string_view key =
      std::string_view{&*start, static_cast<size_t>(std::distance(start, it))};
  detail::parse_attr(value.attr(), it, end);
  detail::parse_item(value.value(), it, end, key);
}

template <typename It, typename U, std::enable_if_t<refletable_v<U>, int> = 0>
IGUANA_INLINE void from_xml(U &value, It &&it, It &&end) {
  while (it != end) {
    skip_sapces_and_newline(it, end);
    match<'<'>(it, end);
    if (*it == '?') {
      skip_till<'>'>(it, end);
      ++it;  // skip >
    }
    else {
      break;
    }
  }
  auto start = it;
  skip_till_greater_or_space(it, end);
  std::string_view key =
      std::string_view{&*start, static_cast<size_t>(std::distance(start, it))};
  detail::parse_item(value, it, end, key);
}

template <typename U, typename View,
          std::enable_if_t<string_container_v<View>, int> = 0>
IGUANA_INLINE void from_xml(U &value, const View &view) {
  from_xml(value, std::begin(view), std::end(view));
}

template <typename Num, std::enable_if_t<num_v<Num>, int> = 0>
IGUANA_INLINE Num get_number(std::string_view str) {
  Num num;
  detail::parse_value(num, str.begin(), str.end());
  return num;
}

}  // namespace iguana