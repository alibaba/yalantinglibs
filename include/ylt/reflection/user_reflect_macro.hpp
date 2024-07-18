#pragma once
#include "internal/foreach_user_macro.hpp"
#include "internal/visit_user_macro.hpp"

namespace ylt::reflection {
#define YLT_REFL(STRUCT, ...)                                                 \
  template <typename Visitor>                                                 \
  inline static constexpr decltype(auto) visit_members(STRUCT &t,             \
                                                       Visitor &&visitor) {   \
    return YLT_VISIT(visitor, t, __VA_ARGS__);                                \
  }                                                                           \
  template <typename Visitor>                                                 \
  inline static constexpr void for_each(STRUCT &t, Visitor &&visitor) {       \
    YLT_FOREACH(visitor, t, __VA_ARGS__);                                     \
  }                                                                           \
  template <typename Visitor>                                                 \
  inline static constexpr decltype(auto) visit_members(const STRUCT &t,       \
                                                       Visitor &&visitor) {   \
    return YLT_VISIT(visitor, t, __VA_ARGS__);                                \
  }                                                                           \
  template <typename Visitor>                                                 \
  inline static constexpr void for_each(const STRUCT &t, Visitor &&visitor) { \
    YLT_FOREACH(visitor, t, __VA_ARGS__);                                     \
  }
}  // namespace ylt::reflection
