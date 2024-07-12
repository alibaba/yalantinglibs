#pragma once
#include <string_view>
#include <variant>

#include "template_string.hpp"
#if __has_include(<concepts>) || defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 10)
#include "member_ptr.hpp"

namespace ylt::reflection {

namespace internal {

template <class T>
struct Wrapper {
  using Type = T;
  T v;
};

template <class T>
Wrapper(T) -> Wrapper<T>;

// This workaround is necessary for clang.
template <class T>
inline constexpr auto wrap(const T& arg) noexcept {
  return Wrapper{arg};
}

template <auto ptr>
inline constexpr std::string_view get_member_name() {
#if defined(_MSC_VER)
  constexpr std::string_view func_name = __FUNCSIG__;
#else
  constexpr std::string_view func_name = __PRETTY_FUNCTION__;
#endif

#if defined(__clang__)
  auto split = func_name.substr(0, func_name.size() - 2);
  return split.substr(split.find_last_of(":.") + 1);
#elif defined(__GNUC__)
  auto split = func_name.substr(0, func_name.rfind(")}"));
  return split.substr(split.find_last_of(":") + 1);
#elif defined(_MSC_VER)
  auto split = func_name.substr(0, func_name.rfind("}>"));
  return split.substr(split.rfind("->") + 2);
#else
  static_assert(false,
                "You are using an unsupported compiler. Please use GCC, Clang "
                "or MSVC or switch to the rfl::Field-syntax.");
#endif
}
}  // namespace internal

template <typename T>
inline constexpr std::array<std::string_view, members_count_v<T>>
get_member_names() {
  constexpr size_t Count = members_count_v<T>;
  constexpr auto tp = struct_to_tuple<T>();

  std::array<std::string_view, Count> arr;
  [&]<size_t... Is>(std::index_sequence<Is...>) mutable {
    ((arr[Is] = internal::get_member_name<internal::wrap(std::get<Is>(tp))>()),
     ...);
  }
  (std::make_index_sequence<Count>{});
  return arr;
}

template <typename... Args>
inline constexpr auto tuple_to_variant(std::tuple<Args...>) {
  return std::variant<std::add_pointer_t<Args>...>{};
}

template <typename T>
using struct_variant_t = decltype(tuple_to_variant(
    std::declval<decltype(struct_to_tuple<std::remove_cvref_t<T>>())>));

template <typename T>
inline const auto& get_member_offset_arr() {
  constexpr size_t Count = members_count_v<T>;
  constexpr auto tp = struct_to_tuple<T>();

  [[maybe_unused]] static std::array<size_t, Count> arr = {[&]<size_t... Is>(
      std::index_sequence<Is...>) mutable {std::array<size_t, Count> arr;
  ((arr[Is] = size_t((const char*)std::get<Is>(tp) -
                     (char*)(&internal::wrapper<T>::value))),
   ...);
  return arr;
}
(std::make_index_sequence<Count>{})
};  // namespace ylt::reflection

return arr;
}

template <typename T>
inline constexpr auto get_member_names_map() {
  constexpr auto name_arr = get_member_names<T>();
  return [&]<size_t... Is>(std::index_sequence<Is...>) mutable {
    return frozen::unordered_map<frozen::string, size_t, name_arr.size()>{
        {name_arr[Is], Is}...};
  }
  (std::make_index_sequence<name_arr.size()>{});
}

template <std::size_t N>
struct FixedString {
  char data[N];
  template <std::size_t... I>
  constexpr FixedString(const char (&s)[N], std::index_sequence<I...>)
      : data{s[I]...} {}
  constexpr FixedString(const char (&s)[N])
      : FixedString(s, std::make_index_sequence<N>()) {}
  constexpr std::string_view str() const {
    return std::string_view{data, N - 1};
  }
};

template <typename T>
inline constexpr size_t index_of(std::string_view name) {
  constexpr auto arr = get_member_names<T>();
  for (size_t i = 0; i < arr.size(); i++) {
    if (arr[i] == name) {
      return i;
    }
  }

  return arr.size();
}

template <typename T, FixedString name>
inline constexpr size_t index_of() {
  return index_of<T>(name.str());
}

template <typename T, size_t index>
inline constexpr std::string_view name_of() {
  static_assert(index < members_count_v<T>, "index out of range");
  constexpr auto arr = get_member_names<T>();
  return arr[index];
}

template <typename T>
inline constexpr std::string_view name_of(size_t index) {
  constexpr auto arr = get_member_names<T>();
  if (index >= arr.size()) {
    return "";
  }

  return arr[index];
}

template <typename T, typename Visit>
inline constexpr void for_each(Visit func) {
  constexpr auto arr = get_member_names<T>();
  [&]<size_t... Is>(std::index_sequence<Is...>) mutable {
    if constexpr (std::is_invocable_v<Visit, std::string_view, size_t>) {
      (func(arr[Is], Is), ...);
    }
    else if constexpr (std::is_invocable_v<Visit, std::string_view>) {
      (func(arr[Is]), ...);
    }
    else {
      static_assert(sizeof(Visit) < 0,
                    "invalid arguments, full arguments: [std::string_view, "
                    "size_t], at least has std::string_view and make sure keep "
                    "the order of arguments");
    }
  }
  (std::make_index_sequence<arr.size()>{});
}

}  // namespace ylt::reflection
#endif
