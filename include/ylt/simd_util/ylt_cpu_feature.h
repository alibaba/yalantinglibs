#pragma once

#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__) || defined(__unix__)
#if defined(__AVX512F__)
#define YLT_HAVE_AVX512
#elif defined(__AVX2__)
#define YLT_HAVE_AVX2
#elif defined(__SSE4_2__)
#define YLT_HAVE_SSE
#else
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define YLT_HAVE_NEON
#endif
#endif

#endif


 