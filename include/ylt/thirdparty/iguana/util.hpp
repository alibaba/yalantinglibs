#pragma once
#include <string_view>

namespace iguana {
template <typename T>
constexpr std::string_view get_raw_name() {
#ifdef _MSC_VER
  return __FUNCSIG__;
#else
  return __PRETTY_FUNCTION__;
#endif
}

template <auto T>
constexpr std::string_view get_raw_name() {
#ifdef _MSC_VER
  return __FUNCSIG__;
#else
  return __PRETTY_FUNCTION__;
#endif
}

template <typename T>
inline constexpr std::string_view type_string() {
  constexpr std::string_view sample = get_raw_name<int>();
  constexpr size_t pos = sample.find("int");
  constexpr std::string_view str = get_raw_name<T>();
  constexpr auto next1 = str.rfind(sample[pos + 3]);
#if defined(_MSC_VER)
  constexpr auto s1 = str.substr(pos + 6, next1 - pos - 6);
#else
  constexpr auto s1 = str.substr(pos, next1 - pos);
#endif
  return s1;
}

template <auto T>
inline constexpr std::string_view enum_string() {
  constexpr std::string_view sample = get_raw_name<int>();
  constexpr size_t pos = sample.find("int");
  constexpr std::string_view str = get_raw_name<T>();
  constexpr auto next1 = str.rfind(sample[pos + 3]);
#if defined(__clang__) || defined(_MSC_VER)
  constexpr auto s1 = str.substr(pos, next1 - pos);
#else
  constexpr auto s1 = str.substr(pos + 5, next1 - pos - 5);
#endif
  return s1;
}
}  // namespace iguana