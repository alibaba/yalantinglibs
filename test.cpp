#include <string_view>
#include <type_traits>
template <std::size_t Idx>
using Idx_t = std::integral_constant<std::size_t, Idx>;
template <typename T, auto field, std::size_t Idx>
struct ThiefMember {
  friend auto& steal_impl(T& t, Idx_t<Idx>) {
    if constexpr (std::is_member_function_pointer_v<decltype(field)>) {
      return (t.*field)();
    }
    else {
      return t.*field;
    }
  }
};
template <typename T, auto field, std::size_t Idx>
struct ThiefMemberConst {
  friend const auto& steal_impl(const T& t, Idx_t<Idx>) {
    if constexpr (std::is_member_function_pointer_v<decltype(field)>) {
      return (t.*field)();
    }
    else {
      return t.*field;
    }
  }
};
class Bank2 {
  float money = 999'999'999'999;
  std::string_view name_;
  bool sex_;
  bool& sex() { return sex_; }
  const bool& sex() const { return sex_; }

 public:
  std::string_view& name() { return name_; }
  const std::string_view& name() const { return name_; }
  int age;
};
template <typename T, typename U, std::size_t Idx>
constexpr inline decltype(auto) member_function_test(U X, Idx_t<Idx>) {
  return X;
};
template <typename T, typename U, std::size_t Idx>
constexpr inline decltype(auto) member_function_test(U (T::*X)(), Idx_t<Idx>) {
  return X;
};
template <typename T, typename U, std::size_t Idx>
constexpr inline decltype(auto) const_member_function_test(U X, Idx_t<Idx>) {
  return X;
};
template <typename T, typename U, std::size_t Idx>
constexpr inline decltype(auto) const_member_function_test(U (T::*X)() const,
                                                           Idx_t<Idx>) {
  return X;
};
#define STRUCT_PACK_VISIT_PRIVATE_HELPER(Idx, X, Bank)                \
  auto& steal_impl(Bank& bank, Idx_t<Idx>);                           \
  const auto& steal_impl(const Bank& bank, Idx_t<Idx>);               \
  template <typename T>                                               \
  constexpr inline decltype(auto) member_function_test(               \
      auto (T::*x)()->decltype(std::declval<T&>().X()), Idx_t<Idx>) { \
    return x;                                                         \
  };                                                                  \
  template <typename T>                                               \
  constexpr inline decltype(auto) const_member_function_test(         \
      auto (T::*x)() const->decltype(std::declval<const T&>().X()),   \
      Idx_t<Idx>) {                                                   \
    return x;                                                         \
  };                                                                  \
  template struct ThiefMember<                                        \
      Bank, member_function_test<Bank>(&Bank::X, Idx_t<Idx>{}), Idx>; \
  template struct ThiefMemberConst<                                   \
      Bank, const_member_function_test<Bank>(&Bank::X, Idx_t<Idx>{}), Idx>;

STRUCT_PACK_VISIT_PRIVATE_HELPER(0, money, Bank2)
STRUCT_PACK_VISIT_PRIVATE_HELPER(1, name, Bank2)
STRUCT_PACK_VISIT_PRIVATE_HELPER(2, age, Bank2)
STRUCT_PACK_VISIT_PRIVATE_HELPER(3, sex, Bank2)

void test(Bank2& bank) {
  steal_impl(bank, Idx_t<0>{}) = 100.04;
  steal_impl(bank, Idx_t<1>{}) = "hi";
  steal_impl(bank, Idx_t<2>{}) = 0;
  // not ok in msvc
  steal_impl(bank, Idx_t<3>{}) = true;
}