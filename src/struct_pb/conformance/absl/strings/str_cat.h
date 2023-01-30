#pragma
#include <string>

// #include "google/protobuf/stubs/strutil.h"
//  compat for absl::StrCat
namespace absl {
// using google::protobuf::StrCat;
template <typename T>
std::string StrCat(const T& t) {
  if constexpr (std::same_as<T, std::string>) {
    return t;
  }
  else if constexpr (std::integral<T>) {
    return std::to_string(t);
  }
  else if constexpr (std::is_enum_v<T>) {
    int n = static_cast<int>(t);
    return std::to_string(n);
  }
  else {
    return std::string(t);
  }
}

template <typename T, typename... Args>
std::string StrCat(const T& t, const Args&... args) {
  return StrCat(t) + StrCat(args...);
}
}  // namespace absl