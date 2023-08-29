#pragma once
#include <cstddef>
#include <string>
#include <utility>

namespace cinatra::detail {

#if __cpp_lib_string_resize_and_overwrite >= 202110L
inline void resize(std::string& str, std::size_t sz) {
  str.resize_and_overwrite(sz, [](char*, std::size_t sz) {
    return sz;
  });
}
#elif (defined(__clang_major__) && __clang_major__ <= 11) || \
    (defined(_MSC_VER) && _MSC_VER <= 1920)
// old clang has bug in global friend function. discard it.
// old msvc don't support visit private, discard it.
inline void resize(std::string& str, std::size_t sz) { str.resize(sz); }
#else

#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
template <typename Money_t, Money_t std::string::*p>
class string_thief {
 public:
  friend void string_set_length_hacker(std::string& bank, std::size_t sz) {
    (bank.*p)(sz);
  }
};
#elif defined(_MSVC_STL_VERSION)
template <typename Money_t, Money_t std::string::*p>
class string_thief {
 public:
  friend void string_set_length_hacker(std::string& bank, std::size_t sz) {
    (bank.*p)._Myval2._Mysize = sz;
  }
};
#endif

#if defined(__GLIBCXX__)  // libstdc++
template class string_thief<void(std::string::size_type),
                            &std::string::_M_set_length>;
#elif defined(_LIBCPP_VERSION)
template class string_thief<void(std::string::size_type),
                            &std::string::__set_size>;
#elif defined(_MSVC_STL_VERSION)
template class string_thief<decltype(std::string::_Mypair),
                            &std::string::_Mypair>;
#endif

#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION) || \
    defined(_MSVC_STL_VERSION)
void string_set_length_hacker(std::string& bank, std::size_t sz);
#endif

inline void resize(std::string& str, std::size_t sz) {
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION) || \
    defined(_MSVC_STL_VERSION)
  str.reserve(sz);
  string_set_length_hacker(str, sz);
  str[sz] = '\0';
#else
  str.resize(sz);
#endif
}
#endif
};  // namespace cinatra::detail