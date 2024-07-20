#pragma once
#include <string_view>
#include <tuple>
#include <type_traits>

#include "internal/member_names_macro.hpp"
#include "internal/visit_user_macro.hpp"

namespace ylt::reflection {
template <class T>
struct member_traits {
  using value_type = T;
};

template <class T, class Owner>
struct member_traits<T Owner::*> {
  using owner_type = Owner;
  using value_type = T;
};

template <class T>
using member_value_type_t = typename member_traits<T>::value_type;

#define YLT_REFL(STRUCT, ...)                                    \
  template <typename Visitor>                                    \
  inline static constexpr decltype(auto) refl_visit_members(     \
      STRUCT &t, Visitor &&visitor) {                            \
    return YLT_VISIT(visitor, t, __VA_ARGS__);                   \
  }                                                              \
  template <typename Visitor>                                    \
  inline static constexpr decltype(auto) refl_visit_members(     \
      const STRUCT &t, Visitor &&visitor) {                      \
    return YLT_VISIT(visitor, t, __VA_ARGS__);                   \
  }                                                              \
  inline static decltype(auto) refl_object_to_tuple(STRUCT &t) { \
    return YLT_VISIT(std::tie, t, __VA_ARGS__);                  \
  }                                                              \
  inline static constexpr decltype(auto) refl_member_names(      \
      const ylt::reflection::identity<STRUCT> &t) {              \
    MAKE_LIST(STRUCT, __VA_ARGS__)                               \
    return arr_##STRUCT_NAME;                                    \
  }                                                              \
  inline static constexpr std::size_t refl_member_count(         \
      const ylt::reflection::identity<STRUCT> &t) {              \
    return (std::size_t)YLT_ARG_COUNT(__VA_ARGS__);              \
  }

#define MAKE_LIST(STRUCT, ...) \
  MAKE_LIST_IMPL(STRUCT, YLT_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

#define MAKE_LIST_IMPL(STRUCT, N, ...)                            \
  constexpr std::array<std::string_view, N> arr_##STRUCT_NAME = { \
      YLT_MARCO_EXPAND(YLT_CONCAT(CON_STR, N)(__VA_ARGS__))};

#define YLT_REFL_PRIVATE_(STRUCT, ...) \
namespace ylt::reflection {                                 \
  inline constexpr auto get_private_ptrs(const identity<STRUCT>& t);\
  template struct private_visitor<STRUCT, __VA_ARGS__>;      \
}

#define YLT_REFL_PRIVATE(STRUCT, ...) \
  YLT_REFL_PRIVATE_(STRUCT, WRAP_ARGS(CONCAT_ADDR, STRUCT, __VA_ARGS__))

template <typename T, typename = void>
struct is_out_ylt_refl : std::false_type {};

template <typename T>
struct is_out_ylt_refl<T, std::void_t<decltype(refl_member_count(
                              std::declval<ylt::reflection::identity<T>>()))>>
    : std::true_type {};

template <typename T>
constexpr bool is_out_ylt_refl_v = is_out_ylt_refl<T>::value;

template <typename T, typename = void>
struct is_inner_ylt_refl : std::false_type {};

template <typename T>
struct is_inner_ylt_refl<
    T, std::void_t<decltype(std::declval<T>().refl_member_count(
           std::declval<ylt::reflection::identity<T>>()))>> : std::true_type {};

template <typename T>
constexpr bool is_inner_ylt_refl_v = is_inner_ylt_refl<T>::value;

template <typename T, typename = void>
struct is_ylt_refl : std::false_type {};

template <typename T>
inline constexpr bool is_ylt_refl_v = is_ylt_refl<T>::value;

template <typename T>
struct is_ylt_refl<T, std::enable_if_t<is_inner_ylt_refl_v<T>>>
    : std::true_type {};

template <typename T>
struct is_ylt_refl<T, std::enable_if_t<is_out_ylt_refl_v<T>>> : std::true_type {
};
}  // namespace ylt::reflection
