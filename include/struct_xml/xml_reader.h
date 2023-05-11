#pragma once
#include <iguana/xml_reader.hpp>

namespace struct_xml {

template <int Flags = 0, typename T>
inline bool from_xml(T &&t, char *buf) {
  return iguana::from_xml<Flags>(std::forward<T>(t), buf);
}

template <int Flags = 0, typename T>
inline bool from_xml(T &&t, const std::string &str) {
  return from_xml<Flags>(std::forward<T>(t), str.data());
}

template <int Flags = 0, typename T>
inline bool from_xml(T &&t, std::string_view str) {
  return from_xml<Flags>(std::forward<T>(t), str.data());
}

inline std::string get_last_read_err() { return iguana::get_last_read_err(); }

using any_t = iguana::any_t;
}  // namespace struct_xml