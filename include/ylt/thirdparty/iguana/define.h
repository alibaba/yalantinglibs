#pragma once

#if defined __clang__
#define IGUANA_INLINE __attribute__((always_inline)) inline
#define IGUANA__INLINE_LAMBDA __attribute__((always_inline)) constexpr
#elif defined _MSC_VER
#define IGUANA_INLINE __forceinline
#define IGUANA__INLINE_LAMBDA constexpr
#else
#define IGUANA_INLINE __attribute__((always_inline)) inline
#define IGUANA__INLINE_LAMBDA constexpr __attribute__((always_inline))
#endif
