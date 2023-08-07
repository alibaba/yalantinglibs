#include <string>
#include <type_traits>

template <std::size_t Idx>
using Idx_t = std::integral_constant<std::size_t, Idx>;

template <typename T, std::size_t Idx, typename = void>
struct is_private_member_field : std::true_type {};

template <typename T, std::size_t Idx, typename = void>
struct is_private_member_function : std::true_type {};

template <typename T, std::size_t Idx, typename = void>
constexpr bool is_private_member = is_private_member_field<T, Idx>::value&&
    is_private_member_function<T, Idx>::value;

template <typename T, auto field, std::size_t Idx>
struct Thief2;

template <typename T, auto (T::*field)() const, std::size_t Idx>
struct Thief2<T,field,Idx> {
  friend const auto& steal_impl(T& t, Idx_t<Idx>) {
      return (t.*field)();
    }
};

template <typename T, auto (T::*field)(), std::size_t Idx>
struct Thief2<T,field,Idx> {
  friend auto& steal_impl(T& t, Idx_t<Idx>) {
    return (t.*field)();\
  }
};

template <typename T, auto field, std::size_t Idx>
struct ThiefMemberFunction {
  friend auto& steal_impl(T& t, Idx_t<Idx>) {
    if constexpr (std::is_same_v<decltype(field),nullptr_t>) {\
       static int useless;
       return useless;
    }\
    else  {\
      return (t.*field)();\
    }\
  }
};

template <typename T, auto field, std::size_t Idx>
struct ThiefMemberFunctionConst {
  friend const auto& steal_impl(const T& t, Idx_t<Idx>) {
    if constexpr (std::is_same_v<decltype(field),nullptr_t>) {\
       static int useless;
       return useless;
    }\
    else  {\
      return (t.*field)();\
    }\
  }
};


class Bank2 {
  float money = 999'999'999'999;
  std::string name_;
  bool sex_;
  bool& sex() {return sex_;}
  const bool& sex() const {return sex_;}
 public:
  std::string& name() { return name_; }
  const std::string& name() const { return name_; }
  int age;
};

template<typename T,typename U>
constexpr inline std::nullptr_t member_function_test(U X) {
    return nullptr;
};

template<typename T, typename U>
constexpr inline auto member_function_test(U (T::* X)()) {
    return X;
};

template<typename T, typename U>
constexpr inline auto const_member_function_test(U X) {
    return nullptr;
};

template<typename T, typename U>
constexpr inline auto const_member_function_test(U (T::* X)() const ) {
    return X;
};



#define STRUCT_PACK_VISIT_PRIVATE_HELPER(Idx, X , Bank) \
auto& steal_impl(Bank& bank, Idx_t<Idx>);\
const auto& steal_impl(const Bank& bank, Idx_t<Idx>);\
template <typename T>\
struct is_private_member_function<T, Idx, std::void_t<decltype(T{}.X())> >\
    : std::false_type {};\
template <typename T>\
struct is_private_member_field<T, Idx, std::void_t<decltype(T{}.X)> >\
    : std::false_type {};\
template struct ThiefMemberFunction<Bank, member_function_test<Bank>(&Bank::X), Idx>;\
template struct ThiefMemberFunctionConst<Bank, const_member_function_test<Bank>(&Bank::X), Idx>;\




//STRUCT_PACK_VISIT_PRIVATE_HELPER(0,money,Bank2)
STRUCT_PACK_VISIT_PRIVATE_HELPER(1,name,Bank2)
//STRUCT_PACK_VISIT_PRIVATE_HELPER(2,age,Bank2)
STRUCT_PACK_VISIT_PRIVATE_HELPER(3,sex,Bank2)

void test(Bank2& bank) {
 // steal_impl(bank,Idx_t<0>{}) = 100.04;
  //static_assert(is_private_member<Bank2, 0>);
  steal_impl(bank,Idx_t<1>{}) = "hi";
  static_assert(!is_private_member<Bank2, 1>);
  //static_assert(!is_private_member<Bank2, 2>);
  steal_impl(bank,Idx_t<3>{}) = true;
  static_assert(is_private_member<Bank2, 3>);
}

