#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace iguana {
template <class, class = void> struct is_container : std::false_type {};

template <class T>
struct is_container<
    T, std::void_t<decltype(std::declval<T>().size(), std::declval<T>().begin(),
                            std::declval<T>().end())>> : std::true_type {};

template <class, class = void> struct is_map_container : std::false_type {};

template <class T>
struct is_map_container<
    T, std::void_t<decltype(std::declval<typename T::mapped_type>())>>
    : public is_container<T> {};

template <typename T> constexpr inline bool is_std_optinal_v = false;

template <typename T>
constexpr inline bool is_std_optinal_v<std::optional<T>> = true;

template <typename T>
constexpr inline bool is_str_v =
    std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>;

template <typename T> constexpr inline bool is_std_pair_v = false;

template <typename T, typename K>
constexpr inline bool is_std_pair_v<std::pair<T, K>> = true;

template <typename T> class namespace_t;
template <typename T> constexpr inline bool is_namespace_v = false;

template <typename T>
constexpr inline bool is_namespace_v<namespace_t<T>> = true;

} // namespace iguana
