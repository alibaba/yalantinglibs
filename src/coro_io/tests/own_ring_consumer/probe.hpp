#pragma once

#include <type_traits>
#include <ylt/coro_io/coro_file.hpp>

#if !defined(ASIO_HAS_IO_URING) || !defined(ASIO_HAS_FILE)
#error "The consumer must inherit the file io_uring compile definitions"
#endif

#if YLT_EXPECT_OWN_RING
static_assert(YLT_ENABLE_OWN_RING == 1);
static_assert(std::is_same_v<coro_io::random_coro_file,
                             coro_io::own_ring_random_coro_file>);
#else
static_assert(std::is_same_v<coro_io::random_coro_file,
                             coro_io::native_random_coro_file>);
#endif

bool probe(coro_io::random_coro_file &file);
