#pragma once
#include <string>
#if __has_include(<memory_resource>)
#include <memory_resource>
#endif

namespace iguana {
#if __has_include(<memory_resource>)
inline char iguana_buf[2048];
inline std::pmr::monotonic_buffer_resource iguana_resource(iguana_buf, 2048);
using string_stream = std::pmr::string;
#else
using string_stream = std::string;
#endif
} // namespace iguana