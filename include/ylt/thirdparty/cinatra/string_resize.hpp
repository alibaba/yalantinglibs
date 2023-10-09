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

template <typename Function, Function func_ptr>
class string_thief {
 public:
  friend void string_set_length_hacker(std::string& self, std::size_t sz) {
#if defined(_MSVC_STL_VERSION)
    (self.*func_ptr)._Myval2._Mysize = sz;
#else
#if (_GLIBCXX_USE_CXX11_ABI == 0) && defined(__GLIBCXX__)
    (self.*func_ptr)()->_M_set_length_and_sharable(sz);
#else
    (self.*func_ptr)(sz);
#endif
#endif
  }
};

#if defined(__GLIBCXX__)  // libstdc++
#if (_GLIBCXX_USE_CXX11_ABI == 0)
template class string_thief<decltype(&std::string::_M_rep),
                            &std::string::_M_rep>;
#else
template class string_thief<decltype(&std::string::_M_set_length),
                            &std::string::_M_set_length>;
#endif
#elif defined(_LIBCPP_VERSION)
template class string_thief<decltype(&std::string::__set_size),
                            &std::string::__set_size>;
#elif defined(_MSVC_STL_VERSION)
template class string_thief<decltype(&std::string::_Mypair),
                            &std::string::_Mypair>;
#endif

void string_set_length_hacker(std::string&, std::size_t);

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