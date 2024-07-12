
#pragma once
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ylt::reflection {
namespace internal {
#if __cpp_concepts >= 201907L
template <typename Type>
concept tuple_size = requires(Type tuple) {
  std::tuple_size<std::remove_cvref_t<Type>>::value;
};
#else
template <typename T, typename = void>
struct tuple_size_impl : std::false_type {};

template <typename T>
struct tuple_size_impl<
    T, std::void_t<decltype(std::tuple_size<remove_cvref_t<T>>::value)>>
    : std::true_type {};

template <typename T>
constexpr bool tuple_size = tuple_size_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
template <typename Type>
concept optional = requires(Type optional) {
  optional.value();
  optional.has_value();
  optional.operator*();
  typename std::remove_cvref_t<Type>::value_type;
};
#else
template <typename T, typename = void>
struct optional_impl : std::false_type {};

template <typename T>
struct optional_impl<T, std::void_t<decltype(std::declval<T>().value()),
                                    decltype(std::declval<T>().has_value()),
                                    decltype(std::declval<T>().operator*()),
                                    typename remove_cvref_t<T>::value_type>>
    : std::true_type {};

template <typename T>
constexpr bool optional = !expected<T> && optional_impl<T>::value;
#endif

template <typename T, uint64_t version = 0>
struct compatible;

template <typename Type>
constexpr inline bool is_compatible_v = false;

template <typename Type, uint64_t version>
constexpr inline bool is_compatible_v<compatible<Type, version>> = true;

struct UniversalVectorType {
  template <typename T>
  operator std::vector<T>();
};

struct UniversalType {
  template <typename T>
  operator T();
};

struct UniversalIntegralType {
  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  operator T();
};

struct UniversalNullptrType {
  operator std::nullptr_t();
};

struct UniversalOptionalType {
  template <typename U, typename = std::enable_if_t<optional<U>>>
  operator U();
};

struct UniversalCompatibleType {
  template <typename U, typename = std::enable_if_t<is_compatible_v<U>>>
  operator U();
};

template <typename T, typename construct_param_t, typename = void,
          typename... Args>
struct is_constructable_impl : std::false_type {};
template <typename T, typename construct_param_t, typename... Args>
struct is_constructable_impl<
    T, construct_param_t,
    std::void_t<decltype(T{{Args{}}..., {construct_param_t{}}})>, Args...>
    : std::true_type {};

template <typename T, typename construct_param_t, typename... Args>
constexpr bool is_constructable =
    is_constructable_impl<T, construct_param_t, void, Args...>::value;

template <typename T, typename... Args>
inline constexpr std::size_t members_count_impl() {
  if constexpr (is_constructable<T, UniversalVectorType, Args...>) {
    return members_count_impl<T, Args..., UniversalVectorType>();
  }
  else if constexpr (is_constructable<T, UniversalType, Args...>) {
    return members_count_impl<T, Args..., UniversalType>();
  }
  else if constexpr (is_constructable<T, UniversalOptionalType, Args...>) {
    return members_count_impl<T, Args..., UniversalOptionalType>();
  }
  else if constexpr (is_constructable<T, UniversalIntegralType, Args...>) {
    return members_count_impl<T, Args..., UniversalIntegralType>();
  }
  else if constexpr (is_constructable<T, UniversalNullptrType, Args...>) {
    return members_count_impl<T, Args..., UniversalNullptrType>();
  }
  else if constexpr (is_constructable<T, UniversalCompatibleType, Args...>) {
    return members_count_impl<T, Args..., UniversalCompatibleType>();
  }
  else {
    return sizeof...(Args);
  }
}
}  // namespace internal

template <typename T>
inline constexpr std::size_t members_count() {
  using type = std::remove_cvref_t<T>;
  if constexpr (internal::tuple_size<type>) {
    return std::tuple_size<type>::value;
  }
  else {
    return internal::members_count_impl<type>();
  }
}

template <typename T>
constexpr std::size_t members_count_v = members_count<T>();
}  // namespace ylt::reflection