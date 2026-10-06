#pragma once

#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__) || \
    defined(__unix__)
#endif

#if defined(__SSE2__)
#if defined(__SSE4_2__)
#define YLT_HAVE_SSE
#endif
#if defined(__AVX2__)
#define YLT_HAVE_AVX2
#endif
#if defined(__AVX512F__)
#define YLT_HAVE_AVX512
#endif
#else
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define YLT_HAVE_NEON
#endif
#endif
