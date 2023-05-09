#pragma once
#include <iguana/xml_writer.hpp>

namespace struct_xml {
template <typename Stream, typename T>
inline void to_xml(Stream &s, T &&t) {
  iguana::to_xml(s, std::forward<T>(t));
}

template <int Flags = 0, typename Stream, typename T>
inline bool to_xml_pretty(Stream &s, T &&t) {
  return iguana::to_xml_pretty<Flags>(s, std::forward<T>(t));
}
}  // namespace struct_xml