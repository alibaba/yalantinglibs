#pragma once
#include "member_count.hpp"

#if __has_include(<concepts>) || defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 10)
// modify based on:
// https://github.com/getml/reflect-cpp/blob/main/include/rfl/internal/bind_fake_object_to_tuple.hpp
// thanks for alxn4's greate idea!
namespace ylt::reflection {
namespace internal {

template <class T, std::size_t n>
struct object_tuple_view_helper {
  static constexpr auto tuple_view() {
    static_assert(
        sizeof(T) < 0,
        "\n\nThis error occurs for one of two reasons:\n\n"
        "1) You have created a struct with more than 100 fields, which is "
        "unsupported. \n\n"
        "2) Your struct is not an aggregate type.\n\n");
  }

  static constexpr auto tuple_view(T&) {
    static_assert(
        sizeof(T) < 0,
        "\n\nThis error occurs for one of two reasons:\n\n"
        "1) You have created a struct with more than 100 fields, which is "
        "unsupported. \n\n"
        "2) Your struct is not an aggregate type.\n\n");
  }
};

template <class T>
struct object_tuple_view_helper<T, 0> {
  static constexpr auto tuple_view() { return std::tie(); }
  static constexpr auto tuple_view(auto&) { return std::tie(); }
};

template <class T>
struct wrapper {
  inline static T value;
};

template <class T>
inline constexpr T& get_fake_object() noexcept {
  return wrapper<T>::value;
}

#define RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS( \
    n, ...)                                                                         \
  template <class T>                                                                \
  struct object_tuple_view_helper<T, n> {                                           \
    static constexpr auto tuple_view() {                                            \
      auto& [__VA_ARGS__] = get_fake_object<std::remove_cvref_t<T>>();              \
      auto ref_tup = std::tie(__VA_ARGS__);                                         \
      auto get_ptrs = [](auto&... _refs) {                                          \
        return std::make_tuple(&_refs...);                                          \
      };                                                                            \
      return std::apply(get_ptrs, ref_tup);                                         \
    }                                                                               \
    static constexpr auto tuple_view(auto& t) {                                     \
      auto& [__VA_ARGS__] = t;                                                      \
      return std::tie(__VA_ARGS__);                                                 \
    }                                                                               \
  }

/*The following boilerplate code was generated using a Python script:
macro =
"RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS"
with open("generated_code4.cpp", "w", encoding="utf-8") as codefile:
    codefile.write(
        "\n".join(
            [
                f"{macro}({i}, {', '.join([f'f{j}' for j in range(i)])});"
                for i in range(1, 129)
            ]
        )
    )
*/

RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(1,
                                                                           f0);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(2,
                                                                           f0,
                                                                           f1);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(3,
                                                                           f0,
                                                                           f1,
                                                                           f2);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    4, f0, f1, f2, f3);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    5, f0, f1, f2, f3, f4);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    6, f0, f1, f2, f3, f4, f5);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    7, f0, f1, f2, f3, f4, f5, f6);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    8, f0, f1, f2, f3, f4, f5, f6, f7);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    9, f0, f1, f2, f3, f4, f5, f6, f7, f8);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    10, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    11, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    12, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    13, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    14, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    15, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    16, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    17, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    18, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    19, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    20, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    21, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    22, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    23, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    24, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    25, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    26, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    27, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    28, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    29, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    30, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    31, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    32, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    33, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    34, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    35, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    36, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    37, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    38, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    39, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    40, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    41, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    42, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    43, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    44, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    45, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    46, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    47, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    48, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    49, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    50, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    51, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    52, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    53, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    54, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    55, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    56, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    57, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    58, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    59, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    60, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    61, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    62, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    63, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    64, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    65, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    66, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    67, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    68, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    69, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    70, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    71, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    72, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    73, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    74, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    75, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    76, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    77, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    78, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    79, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    80, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    81, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    82, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    83, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    84, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    85, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    86, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    87, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    88, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    89, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    90, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    91, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    92, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    93, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    94, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    95, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    96, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    97, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    98, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    99, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    100, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    101, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    102, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    103, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    104, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    105, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    106, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    107, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    108, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    109, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    110, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    111, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    112, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    113, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    114, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    115, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    116, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    117, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    118, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    119, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    120, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    121, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119, f120);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    122, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119, f120, f121);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    123, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119, f120, f121, f122);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    124, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119, f120, f121, f122, f123);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    125, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119, f120, f121, f122, f123, f124);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    126, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119, f120, f121, f122, f123, f124, f125);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    127, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119, f120, f121, f122, f123, f124, f125, f126);
RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS(
    128, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
    f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30,
    f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45,
    f46, f47, f48, f49, f50, f51, f52, f53, f54, f55, f56, f57, f58, f59, f60,
    f61, f62, f63, f64, f65, f66, f67, f68, f69, f70, f71, f72, f73, f74, f75,
    f76, f77, f78, f79, f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90,
    f91, f92, f93, f94, f95, f96, f97, f98, f99, f100, f101, f102, f103, f104,
    f105, f106, f107, f108, f109, f110, f111, f112, f113, f114, f115, f116,
    f117, f118, f119, f120, f121, f122, f123, f124, f125, f126, f127);

#undef RFL_INTERNAL_OBJECT_IF_YOU_SEE_AN_ERROR_REFER_TO_DOCUMENTATION_ON_C_ARRAYS

template <class T>
inline constexpr auto tuple_view(T& t) {
  return internal::object_tuple_view_helper<T, members_count_v<T>>::tuple_view(
      t);
}
}  // namespace internal

template <class T>
inline constexpr auto struct_to_tuple() {
  return internal::object_tuple_view_helper<T,
                                            members_count_v<T>>::tuple_view();
}

template <class T>
inline constexpr auto object_to_tuple(T& t) {
  return internal::tuple_view(t);
}
}  // namespace ylt::reflection
#endif
