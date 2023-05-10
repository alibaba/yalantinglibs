#pragma once
#include <iguana/xml_reader.hpp>

namespace struct_xml {

template <int Flags = 0, typename T>
inline bool from_xml(T &&t, char *buf) {
  return iguana::from_xml<Flags>(std::forward<T>(t), buf);
}

using any_t = iguana::any_t;
}  // namespace struct_xml