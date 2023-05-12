#pragma once
#include <iguana/xml_writer.hpp>

namespace struct_xml {
template <typename Stream, typename T>
inline void to_xml(T &&t, Stream &s) {
  iguana::to_xml(std::forward<T>(t), s);
}

template <int Flags = 0, typename Stream, typename T>
inline bool to_xml_pretty(T &&t, Stream &s) {
  return iguana::to_xml_pretty<Flags>(std::forward<T>(t), s);
}
}  // namespace struct_xml