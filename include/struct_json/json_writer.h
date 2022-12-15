#pragma once

#include <iguana/json_writer.hpp>
#include <iguana/prettify.hpp>

namespace struct_json {
template <typename Stream, typename T>
IGUANA_INLINE void to_json(Stream &s, T &&t) {
  iguana::to_json(s, t);
}

inline std::string prettify(const auto &in, const bool tabs = false,
                            const uint32_t indent_size = 3) noexcept {
  return iguana::prettify(in, tabs, indent_size);
}
}  // namespace struct_json