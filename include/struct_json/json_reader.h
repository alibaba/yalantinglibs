#pragma once

#include <iguana/json_reader.hpp>
namespace struct_json {
template <typename T, typename It>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end) {
  iguana::from_json(value, it, end);
}

template <typename T, typename It>
IGUANA_INLINE void from_json(T &value, It &&it, It &&end,
                             std::error_code &ec) noexcept {
  iguana::from_json(value, it, end, ec);
}

template <typename T, iguana::json_view View>
IGUANA_INLINE void from_json(T &value, const View &view) {
  iguana::from_json(value, view);
}

template <typename T, iguana::json_view View>
IGUANA_INLINE void from_json(T &value, const View &view,
                             std::error_code &ec) noexcept {
  iguana::from_json(value, view, ec);
}

template <typename T, iguana::json_byte Byte>
IGUANA_INLINE void from_json(T &value, const Byte *data, size_t size) {
  iguana::from_json(value, data, size);
}

template <typename T, iguana::json_byte Byte>
IGUANA_INLINE void from_json(T &value, const Byte *data, size_t size,
                             std::error_code &ec) noexcept {
  iguana::from_json(value, data, size, ec);
}

template <typename T>
IGUANA_INLINE void from_json_file(T &value, const std::string &filename) {
  iguana::from_json_file(value, filename);
}

template <typename T>
IGUANA_INLINE void from_json_file(T &value, const std::string &filename,
                                  std::error_code &ec) noexcept {
  iguana::from_json_file(value, filename, ec);
}

}  // namespace struct_json