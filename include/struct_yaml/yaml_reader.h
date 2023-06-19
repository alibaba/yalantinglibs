#pragma once
#include <iguana/yaml_reader.hpp>

namespace struct_yaml {

template <typename T, iguana::string_t View>
inline void from_yaml(T &value, const View &view){
  iguana::from_yaml(value, view);
}

template <typename T, iguana::string_t View>
inline void from_yaml(T &value, const View &view, std::error_code &ec){
  iguana::from_yaml(value, view, ec);
}
}