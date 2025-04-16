#pragma once

#include "ylt_cpu_feature.h"
#include "ylt_simd_macro.h"

#if defined(YLT_HAVE_AVX2)
#define YLT_USING_ARCH_FUNC(func) using avx2::func
#define INCLUDE_ARCH_FILE(file) YLT_STRINGIFY(avx2/file)
#elif defined(YLT_HAVE_AVX512)
#define YLT_USING_ARCH_FUNC(func) using avx512::func
#define INCLUDE_ARCH_FILE(file) YLT_STRINGIFY(avx512/file)
#elif defined(YLT_HAVE_SSE)
#define YLT_USING_ARCH_FUNC(func) using sse::func
#define INCLUDE_ARCH_FILE(file) YLT_STRINGIFY(sse/file)
#else
#if defined(YLT_HAVE_NEON)
#define YLT_USING_ARCH_FUNC(func) using neon::func
#define INCLUDE_ARCH_FILE(file) YLT_STRINGIFY(neon/file)
#endif
#endif

#if !defined(YLT_HAVE_AVX2) && !defined(YLT_HAVE_AVX512) && !defined(YLT_HAVE_SSE) && !defined(YLT_HAVE_NEON)
#define YLT_USING_ARCH_FUNC(func) using common::func
#define INCLUDE_ARCH_FILE(file) YLT_STRINGIFY(common/file)
#endif