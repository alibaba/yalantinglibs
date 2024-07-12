#pragma once
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <variant>

#include "template_string.hpp"

#if __has_include(<concepts>) || defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 10)
#include "member_names.hpp"

namespace ylt::reflection {

namespace internal {
template <typename Member, typename T>
inline void set_member_ptr(Member& member, T t) {
  if constexpr (std::is_constructible_v<Member, T>) {
    member = t;
  }
  else {
    std::string str = "given type: ";
    str.append(iguana::type_string<std::remove_pointer_t<Member>>());
    str.append(" is not equal with real type: ")
        .append(iguana::type_string<std::remove_pointer_t<T>>());
    throw std::invalid_argument(str);
  }
}

template <typename T>
struct is_variant : std::false_type {};

template <typename... T>
struct is_variant<std::variant<T...>> : std::true_type {};

template <typename Member, class Tuple, std::size_t... Is>
inline bool tuple_switch(Member& member, std::size_t i, Tuple& t,
                         std::index_sequence<Is...>) {
  if constexpr (is_variant<Member>::value) {
    return (((i == Is) &&
             ((member = Member{std::in_place_index<Is>, &std::get<Is>(t)}),
              true)) ||
            ...);
  }
  else {
    return (((i == Is) && ((set_member_ptr(member, &std::get<Is>(t))), true)) ||
            ...);
  }
}
}  // namespace internal

template <typename Member, typename T>
inline Member& get(T& t, size_t index) {
  auto ref_tp = object_to_tuple(t);
  constexpr size_t tuple_size = std::tuple_size_v<decltype(ref_tp)>;

  if (index >= tuple_size) {
    std::string str = "index out of range, ";
    str.append("index: ")
        .append(std::to_string(index))
        .append(" is greater equal than member count ")
        .append(std::to_string(tuple_size));
    throw std::out_of_range(str);
  }
  Member* member_ptr = nullptr;
  internal::tuple_switch(member_ptr, index, ref_tp,
                         std::make_index_sequence<tuple_size>{});
  if (member_ptr == nullptr) {
    throw std::invalid_argument(
        "given member type is not match the real member type");
  }
  return *member_ptr;
}

template <typename Member, typename T>
inline Member& get(T& t, std::string_view name) {
  static constexpr auto map = get_member_names_map<T>();
  size_t index = map.at(name);  // may throw out_of_range: unknown key.
  auto ref_tp = object_to_tuple(t);

  Member* member_ptr = nullptr;
  internal::tuple_switch(member_ptr, index, ref_tp,
                         std::make_index_sequence<map.size()>{});
  if (member_ptr == nullptr) {
    throw std::invalid_argument(
        "given member type is not match the real member type");
  }
  return *member_ptr;
}

template <typename T>
inline auto get(T& t, size_t index) {
  auto ref_tp = object_to_tuple(t);
  constexpr size_t tuple_size = std::tuple_size_v<decltype(ref_tp)>;
  if (index >= tuple_size) {
    std::string str = "index out of range, ";
    str.append("index: ")
        .append(std::to_string(index))
        .append(" is greater equal than member count ")
        .append(std::to_string(tuple_size));
    throw std::out_of_range(str);
  }

  using variant = decltype(tuple_to_variant(ref_tp));
  variant member_ptr;
  internal::tuple_switch(member_ptr, index, ref_tp,
                         std::make_index_sequence<tuple_size>{});
  return member_ptr;
}

template <typename T>
inline auto get(T& t, std::string_view name) {
  static constexpr auto map = get_member_names_map<T>();
  size_t index = map.at(name);  // may throw out_of_range: unknown key.
  return get(t, index);
}

template <size_t index, typename T>
inline auto& get(T& t) {
  auto ref_tp = object_to_tuple(t);

  static_assert(index < std::tuple_size_v<decltype(ref_tp)>,
                "index out of range");

  return std::get<index>(ref_tp);
}

template <FixedString name, typename T>
inline auto& get(T& t) {
  constexpr size_t index = index_of<T, name>();
  return get<index>(t);
}

template <size_t I, typename T, typename U>
inline bool check_value(T value, U field_value) {
  if constexpr (std::is_same_v<T, U>) {
    return value == field_value;
  }
  else {
    return false;
  }
}

template <typename T, typename Field>
inline size_t index_of(T& t, Field& value) {
  const auto& offset_arr = get_member_offset_arr<T>();
  size_t cur_offset = (const char*)(&value) - (const char*)(&t);
  auto it = std::lower_bound(offset_arr.begin(), offset_arr.end(), cur_offset);
  if (it == offset_arr.end()) {
    return offset_arr.size();
  }

  return std::distance(offset_arr.begin(), it);
}

template <typename T, typename Field>
inline std::string_view name_of(T& t, Field& value) {
  size_t index = index_of(t, value);
  constexpr auto arr = get_member_names<T>();
  if (index == arr.size()) {
    return "";
  }

  return arr[index];
}

template <typename T, typename Visit>
inline void for_each(T& t, Visit func) {
  auto ref_tp = object_to_tuple(t);
  constexpr size_t tuple_size = std::tuple_size_v<decltype(ref_tp)>;
  constexpr auto arr = get_member_names<T>();
  [&]<size_t... Is>(std::index_sequence<Is...>) mutable {
    if constexpr (std::is_invocable_v<Visit, decltype(std::get<0>(ref_tp))>) {
      (func(std::get<Is>(ref_tp)), ...);
    }
    else if constexpr (std::is_invocable_v<Visit, decltype(std::get<0>(ref_tp)),
                                           std::string_view>) {
      (func(std::get<Is>(ref_tp), arr[Is]), ...);
    }
    else if constexpr (std::is_invocable_v<Visit, decltype(std::get<0>(ref_tp)),
                                           std::string_view, size_t>) {
      (func(std::get<Is>(ref_tp), arr[Is], Is), ...);
    }
    else {
      static_assert(sizeof(Visit) < 0,
                    "invalid arguments, full arguments: [field_value&, "
                    "std::string_view, size_t], at least has field_value and "
                    "make sure keep the order of arguments");
    }
  }
  (std::make_index_sequence<tuple_size>{});
}

}  // namespace ylt::reflection

template <ylt::reflection::FixedString s>
inline constexpr auto operator""_ylts() {
  return s;
}
#endif
