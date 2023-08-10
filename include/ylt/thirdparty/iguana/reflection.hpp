//
// Created by Qiyu on 17-6-5.
//

#ifndef IGUANA_REFLECTION_HPP
#define IGUANA_REFLECTION_HPP
#include <array>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include "detail/itoa.hpp"
#include "detail/string_stream.hpp"
#include "detail/traits.hpp"
#include "frozen/string.h"
#include "frozen/unordered_map.h"

namespace iguana::detail {
/******************************************/
/* arg list expand macro, now support 120 args */
#define MAKE_ARG_LIST_1(op, arg, ...) op(arg)
#define MAKE_ARG_LIST_2(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_1(op, __VA_ARGS__))
#define MAKE_ARG_LIST_3(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_2(op, __VA_ARGS__))
#define MAKE_ARG_LIST_4(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_3(op, __VA_ARGS__))
#define MAKE_ARG_LIST_5(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_4(op, __VA_ARGS__))
#define MAKE_ARG_LIST_6(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_5(op, __VA_ARGS__))
#define MAKE_ARG_LIST_7(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_6(op, __VA_ARGS__))
#define MAKE_ARG_LIST_8(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_7(op, __VA_ARGS__))
#define MAKE_ARG_LIST_9(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_8(op, __VA_ARGS__))
#define MAKE_ARG_LIST_10(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_9(op, __VA_ARGS__))
#define MAKE_ARG_LIST_11(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_10(op, __VA_ARGS__))
#define MAKE_ARG_LIST_12(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_11(op, __VA_ARGS__))
#define MAKE_ARG_LIST_13(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_12(op, __VA_ARGS__))
#define MAKE_ARG_LIST_14(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_13(op, __VA_ARGS__))
#define MAKE_ARG_LIST_15(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_14(op, __VA_ARGS__))
#define MAKE_ARG_LIST_16(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_15(op, __VA_ARGS__))
#define MAKE_ARG_LIST_17(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_16(op, __VA_ARGS__))
#define MAKE_ARG_LIST_18(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_17(op, __VA_ARGS__))
#define MAKE_ARG_LIST_19(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_18(op, __VA_ARGS__))
#define MAKE_ARG_LIST_20(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_19(op, __VA_ARGS__))
#define MAKE_ARG_LIST_21(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_20(op, __VA_ARGS__))
#define MAKE_ARG_LIST_22(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_21(op, __VA_ARGS__))
#define MAKE_ARG_LIST_23(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_22(op, __VA_ARGS__))
#define MAKE_ARG_LIST_24(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_23(op, __VA_ARGS__))
#define MAKE_ARG_LIST_25(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_24(op, __VA_ARGS__))
#define MAKE_ARG_LIST_26(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_25(op, __VA_ARGS__))
#define MAKE_ARG_LIST_27(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_26(op, __VA_ARGS__))
#define MAKE_ARG_LIST_28(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_27(op, __VA_ARGS__))
#define MAKE_ARG_LIST_29(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_28(op, __VA_ARGS__))
#define MAKE_ARG_LIST_30(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_29(op, __VA_ARGS__))
#define MAKE_ARG_LIST_31(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_30(op, __VA_ARGS__))
#define MAKE_ARG_LIST_32(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_31(op, __VA_ARGS__))
#define MAKE_ARG_LIST_33(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_32(op, __VA_ARGS__))
#define MAKE_ARG_LIST_34(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_33(op, __VA_ARGS__))
#define MAKE_ARG_LIST_35(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_34(op, __VA_ARGS__))
#define MAKE_ARG_LIST_36(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_35(op, __VA_ARGS__))
#define MAKE_ARG_LIST_37(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_36(op, __VA_ARGS__))
#define MAKE_ARG_LIST_38(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_37(op, __VA_ARGS__))
#define MAKE_ARG_LIST_39(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_38(op, __VA_ARGS__))
#define MAKE_ARG_LIST_40(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_39(op, __VA_ARGS__))
#define MAKE_ARG_LIST_41(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_40(op, __VA_ARGS__))
#define MAKE_ARG_LIST_42(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_41(op, __VA_ARGS__))
#define MAKE_ARG_LIST_43(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_42(op, __VA_ARGS__))
#define MAKE_ARG_LIST_44(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_43(op, __VA_ARGS__))
#define MAKE_ARG_LIST_45(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_44(op, __VA_ARGS__))
#define MAKE_ARG_LIST_46(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_45(op, __VA_ARGS__))
#define MAKE_ARG_LIST_47(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_46(op, __VA_ARGS__))
#define MAKE_ARG_LIST_48(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_47(op, __VA_ARGS__))
#define MAKE_ARG_LIST_49(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_48(op, __VA_ARGS__))
#define MAKE_ARG_LIST_50(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_49(op, __VA_ARGS__))
#define MAKE_ARG_LIST_51(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_50(op, __VA_ARGS__))
#define MAKE_ARG_LIST_52(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_51(op, __VA_ARGS__))
#define MAKE_ARG_LIST_53(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_52(op, __VA_ARGS__))
#define MAKE_ARG_LIST_54(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_53(op, __VA_ARGS__))
#define MAKE_ARG_LIST_55(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_54(op, __VA_ARGS__))
#define MAKE_ARG_LIST_56(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_55(op, __VA_ARGS__))
#define MAKE_ARG_LIST_57(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_56(op, __VA_ARGS__))
#define MAKE_ARG_LIST_58(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_57(op, __VA_ARGS__))
#define MAKE_ARG_LIST_59(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_58(op, __VA_ARGS__))
#define MAKE_ARG_LIST_60(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_59(op, __VA_ARGS__))
#define MAKE_ARG_LIST_61(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_60(op, __VA_ARGS__))
#define MAKE_ARG_LIST_62(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_61(op, __VA_ARGS__))
#define MAKE_ARG_LIST_63(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_62(op, __VA_ARGS__))
#define MAKE_ARG_LIST_64(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_63(op, __VA_ARGS__))
#define MAKE_ARG_LIST_65(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_64(op, __VA_ARGS__))
#define MAKE_ARG_LIST_66(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_65(op, __VA_ARGS__))
#define MAKE_ARG_LIST_67(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_66(op, __VA_ARGS__))
#define MAKE_ARG_LIST_68(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_67(op, __VA_ARGS__))
#define MAKE_ARG_LIST_69(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_68(op, __VA_ARGS__))
#define MAKE_ARG_LIST_70(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_69(op, __VA_ARGS__))
#define MAKE_ARG_LIST_71(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_70(op, __VA_ARGS__))
#define MAKE_ARG_LIST_72(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_71(op, __VA_ARGS__))
#define MAKE_ARG_LIST_73(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_72(op, __VA_ARGS__))
#define MAKE_ARG_LIST_74(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_73(op, __VA_ARGS__))
#define MAKE_ARG_LIST_75(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_74(op, __VA_ARGS__))
#define MAKE_ARG_LIST_76(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_75(op, __VA_ARGS__))
#define MAKE_ARG_LIST_77(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_76(op, __VA_ARGS__))
#define MAKE_ARG_LIST_78(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_77(op, __VA_ARGS__))
#define MAKE_ARG_LIST_79(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_78(op, __VA_ARGS__))
#define MAKE_ARG_LIST_80(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_79(op, __VA_ARGS__))
#define MAKE_ARG_LIST_81(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_80(op, __VA_ARGS__))
#define MAKE_ARG_LIST_82(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_81(op, __VA_ARGS__))
#define MAKE_ARG_LIST_83(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_82(op, __VA_ARGS__))
#define MAKE_ARG_LIST_84(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_83(op, __VA_ARGS__))
#define MAKE_ARG_LIST_85(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_84(op, __VA_ARGS__))
#define MAKE_ARG_LIST_86(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_85(op, __VA_ARGS__))
#define MAKE_ARG_LIST_87(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_86(op, __VA_ARGS__))
#define MAKE_ARG_LIST_88(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_87(op, __VA_ARGS__))
#define MAKE_ARG_LIST_89(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_88(op, __VA_ARGS__))
#define MAKE_ARG_LIST_90(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_89(op, __VA_ARGS__))
#define MAKE_ARG_LIST_91(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_90(op, __VA_ARGS__))
#define MAKE_ARG_LIST_92(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_91(op, __VA_ARGS__))
#define MAKE_ARG_LIST_93(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_92(op, __VA_ARGS__))
#define MAKE_ARG_LIST_94(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_93(op, __VA_ARGS__))
#define MAKE_ARG_LIST_95(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_94(op, __VA_ARGS__))
#define MAKE_ARG_LIST_96(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_95(op, __VA_ARGS__))
#define MAKE_ARG_LIST_97(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_96(op, __VA_ARGS__))
#define MAKE_ARG_LIST_98(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_97(op, __VA_ARGS__))
#define MAKE_ARG_LIST_99(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_98(op, __VA_ARGS__))
#define MAKE_ARG_LIST_100(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_99(op, __VA_ARGS__))
#define MAKE_ARG_LIST_101(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_100(op, __VA_ARGS__))
#define MAKE_ARG_LIST_102(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_101(op, __VA_ARGS__))
#define MAKE_ARG_LIST_103(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_102(op, __VA_ARGS__))
#define MAKE_ARG_LIST_104(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_103(op, __VA_ARGS__))
#define MAKE_ARG_LIST_105(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_104(op, __VA_ARGS__))
#define MAKE_ARG_LIST_106(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_105(op, __VA_ARGS__))
#define MAKE_ARG_LIST_107(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_106(op, __VA_ARGS__))
#define MAKE_ARG_LIST_108(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_107(op, __VA_ARGS__))
#define MAKE_ARG_LIST_109(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_108(op, __VA_ARGS__))
#define MAKE_ARG_LIST_110(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_109(op, __VA_ARGS__))
#define MAKE_ARG_LIST_111(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_110(op, __VA_ARGS__))
#define MAKE_ARG_LIST_112(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_111(op, __VA_ARGS__))
#define MAKE_ARG_LIST_113(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_112(op, __VA_ARGS__))
#define MAKE_ARG_LIST_114(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_113(op, __VA_ARGS__))
#define MAKE_ARG_LIST_115(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_114(op, __VA_ARGS__))
#define MAKE_ARG_LIST_116(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_115(op, __VA_ARGS__))
#define MAKE_ARG_LIST_117(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_116(op, __VA_ARGS__))
#define MAKE_ARG_LIST_118(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_117(op, __VA_ARGS__))
#define MAKE_ARG_LIST_119(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_118(op, __VA_ARGS__))
#define MAKE_ARG_LIST_120(op, arg, ...) \
  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_119(op, __VA_ARGS__))

#define ADD_VIEW(str) std::string_view(#str, sizeof(#str) - 1)

#define SEPERATOR ,
#define CON_STR_1(element, ...) ADD_VIEW(element)
#define CON_STR_2(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_1(__VA_ARGS__))
#define CON_STR_3(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_2(__VA_ARGS__))
#define CON_STR_4(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_3(__VA_ARGS__))
#define CON_STR_5(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_4(__VA_ARGS__))
#define CON_STR_6(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_5(__VA_ARGS__))
#define CON_STR_7(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_6(__VA_ARGS__))
#define CON_STR_8(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_7(__VA_ARGS__))
#define CON_STR_9(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_8(__VA_ARGS__))
#define CON_STR_10(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_9(__VA_ARGS__))
#define CON_STR_11(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_10(__VA_ARGS__))
#define CON_STR_12(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_11(__VA_ARGS__))
#define CON_STR_13(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_12(__VA_ARGS__))
#define CON_STR_14(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_13(__VA_ARGS__))
#define CON_STR_15(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_14(__VA_ARGS__))
#define CON_STR_16(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_15(__VA_ARGS__))
#define CON_STR_17(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_16(__VA_ARGS__))
#define CON_STR_18(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_17(__VA_ARGS__))
#define CON_STR_19(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_18(__VA_ARGS__))
#define CON_STR_20(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_19(__VA_ARGS__))
#define CON_STR_21(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_20(__VA_ARGS__))
#define CON_STR_22(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_21(__VA_ARGS__))
#define CON_STR_23(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_22(__VA_ARGS__))
#define CON_STR_24(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_23(__VA_ARGS__))
#define CON_STR_25(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_24(__VA_ARGS__))
#define CON_STR_26(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_25(__VA_ARGS__))
#define CON_STR_27(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_26(__VA_ARGS__))
#define CON_STR_28(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_27(__VA_ARGS__))
#define CON_STR_29(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_28(__VA_ARGS__))
#define CON_STR_30(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_29(__VA_ARGS__))
#define CON_STR_31(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_30(__VA_ARGS__))
#define CON_STR_32(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_31(__VA_ARGS__))
#define CON_STR_33(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_32(__VA_ARGS__))
#define CON_STR_34(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_33(__VA_ARGS__))
#define CON_STR_35(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_34(__VA_ARGS__))
#define CON_STR_36(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_35(__VA_ARGS__))
#define CON_STR_37(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_36(__VA_ARGS__))
#define CON_STR_38(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_37(__VA_ARGS__))
#define CON_STR_39(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_38(__VA_ARGS__))
#define CON_STR_40(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_39(__VA_ARGS__))
#define CON_STR_41(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_40(__VA_ARGS__))
#define CON_STR_42(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_41(__VA_ARGS__))
#define CON_STR_43(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_42(__VA_ARGS__))
#define CON_STR_44(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_43(__VA_ARGS__))
#define CON_STR_45(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_44(__VA_ARGS__))
#define CON_STR_46(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_45(__VA_ARGS__))
#define CON_STR_47(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_46(__VA_ARGS__))
#define CON_STR_48(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_47(__VA_ARGS__))
#define CON_STR_49(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_48(__VA_ARGS__))
#define CON_STR_50(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_49(__VA_ARGS__))
#define CON_STR_51(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_50(__VA_ARGS__))
#define CON_STR_52(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_51(__VA_ARGS__))
#define CON_STR_53(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_52(__VA_ARGS__))
#define CON_STR_54(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_53(__VA_ARGS__))
#define CON_STR_55(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_54(__VA_ARGS__))
#define CON_STR_56(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_55(__VA_ARGS__))
#define CON_STR_57(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_56(__VA_ARGS__))
#define CON_STR_58(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_57(__VA_ARGS__))
#define CON_STR_59(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_58(__VA_ARGS__))
#define CON_STR_60(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_59(__VA_ARGS__))
#define CON_STR_61(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_60(__VA_ARGS__))
#define CON_STR_62(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_61(__VA_ARGS__))
#define CON_STR_63(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_62(__VA_ARGS__))
#define CON_STR_64(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_63(__VA_ARGS__))
#define CON_STR_65(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_64(__VA_ARGS__))
#define CON_STR_66(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_65(__VA_ARGS__))
#define CON_STR_67(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_66(__VA_ARGS__))
#define CON_STR_68(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_67(__VA_ARGS__))
#define CON_STR_69(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_68(__VA_ARGS__))
#define CON_STR_70(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_69(__VA_ARGS__))
#define CON_STR_71(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_70(__VA_ARGS__))
#define CON_STR_72(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_71(__VA_ARGS__))
#define CON_STR_73(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_72(__VA_ARGS__))
#define CON_STR_74(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_73(__VA_ARGS__))
#define CON_STR_75(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_74(__VA_ARGS__))
#define CON_STR_76(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_75(__VA_ARGS__))
#define CON_STR_77(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_76(__VA_ARGS__))
#define CON_STR_78(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_77(__VA_ARGS__))
#define CON_STR_79(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_78(__VA_ARGS__))
#define CON_STR_80(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_79(__VA_ARGS__))
#define CON_STR_81(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_80(__VA_ARGS__))
#define CON_STR_82(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_81(__VA_ARGS__))
#define CON_STR_83(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_82(__VA_ARGS__))
#define CON_STR_84(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_83(__VA_ARGS__))
#define CON_STR_85(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_84(__VA_ARGS__))
#define CON_STR_86(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_85(__VA_ARGS__))
#define CON_STR_87(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_86(__VA_ARGS__))
#define CON_STR_88(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_87(__VA_ARGS__))
#define CON_STR_89(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_88(__VA_ARGS__))
#define CON_STR_90(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_89(__VA_ARGS__))
#define CON_STR_91(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_90(__VA_ARGS__))
#define CON_STR_92(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_91(__VA_ARGS__))
#define CON_STR_93(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_92(__VA_ARGS__))
#define CON_STR_94(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_93(__VA_ARGS__))
#define CON_STR_95(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_94(__VA_ARGS__))
#define CON_STR_96(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_95(__VA_ARGS__))
#define CON_STR_97(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_96(__VA_ARGS__))
#define CON_STR_98(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_97(__VA_ARGS__))
#define CON_STR_99(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_98(__VA_ARGS__))
#define CON_STR_100(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_99(__VA_ARGS__))
#define CON_STR_101(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_100(__VA_ARGS__))
#define CON_STR_102(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_101(__VA_ARGS__))
#define CON_STR_103(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_102(__VA_ARGS__))
#define CON_STR_104(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_103(__VA_ARGS__))
#define CON_STR_105(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_104(__VA_ARGS__))
#define CON_STR_106(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_105(__VA_ARGS__))
#define CON_STR_107(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_106(__VA_ARGS__))
#define CON_STR_108(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_107(__VA_ARGS__))
#define CON_STR_109(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_108(__VA_ARGS__))
#define CON_STR_110(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_109(__VA_ARGS__))
#define CON_STR_111(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_110(__VA_ARGS__))
#define CON_STR_112(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_111(__VA_ARGS__))
#define CON_STR_113(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_112(__VA_ARGS__))
#define CON_STR_114(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_113(__VA_ARGS__))
#define CON_STR_115(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_114(__VA_ARGS__))
#define CON_STR_116(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_115(__VA_ARGS__))
#define CON_STR_117(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_116(__VA_ARGS__))
#define CON_STR_118(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_117(__VA_ARGS__))
#define CON_STR_119(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_118(__VA_ARGS__))
#define CON_STR_120(element, ...) \
  ADD_VIEW(element) SEPERATOR MARCO_EXPAND(CON_STR_119(__VA_ARGS__))
#define RSEQ_N()                                                               \
  119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105,   \
      104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, \
      87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70,  \
      69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52,  \
      51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34,  \
      33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,  \
      15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
              _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, \
              _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
              _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, \
              _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, \
              _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, \
              _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, \
              _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
              _105, _106, _107, _108, _109, _110, _111, _112, _113, _114,      \
              _115, _116, _117, _118, _119, N, ...)                            \
  N

#define MARCO_EXPAND(...) __VA_ARGS__
#define APPLY_VARIADIC_MACRO(macro, ...) MARCO_EXPAND(macro(__VA_ARGS__))

#define ADD_REFERENCE(t) std::reference_wrapper<decltype(t)>(t)
#define ADD_REFERENCE_CONST(t) \
  std::reference_wrapper<std::add_const_t<decltype(t)>>(t)
#define FIELD(t) t
#define MAKE_NAMES(...) #__VA_ARGS__,

// note use MACRO_CONCAT like A##_##B direct may cause marco expand error
#define MACRO_CONCAT(A, B) MACRO_CONCAT1(A, B)
#define MACRO_CONCAT1(A, B) A##_##B

#define MAKE_ARG_LIST(N, op, arg, ...) \
  MACRO_CONCAT(MAKE_ARG_LIST, N)(op, arg, __VA_ARGS__)

#define GET_ARG_COUNT_INNER(...) MARCO_EXPAND(ARG_N(__VA_ARGS__))
#define GET_ARG_COUNT(...) GET_ARG_COUNT_INNER(__VA_ARGS__, RSEQ_N())

#define MAKE_STR_LIST(...) \
  MACRO_CONCAT(CON_STR, GET_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define MAKE_META_DATA_IMPL(STRUCT_NAME, ...)                                 \
  [[maybe_unused]] inline static auto iguana_reflect_members(                 \
      STRUCT_NAME const &) {                                                  \
    struct reflect_members {                                                  \
      constexpr decltype(auto) static apply_impl() {                          \
        return std::make_tuple(__VA_ARGS__);                                  \
      }                                                                       \
      using size_type =                                                       \
          std::integral_constant<size_t, GET_ARG_COUNT(__VA_ARGS__)>;         \
      constexpr static std::string_view name() { return name_##STRUCT_NAME; } \
      constexpr static std::string_view struct_name() {                       \
        return std::string_view(#STRUCT_NAME, sizeof(#STRUCT_NAME) - 1);      \
      }                                                                       \
      constexpr static std::string_view fields() {                            \
        return fields_##STRUCT_NAME;                                          \
      }                                                                       \
      constexpr static size_t value() { return size_type::value; }            \
      constexpr static std::array<frozen::string, size_type::value> arr() {   \
        return arr_##STRUCT_NAME;                                             \
      }                                                                       \
    };                                                                        \
    return reflect_members{};                                                 \
  }

#define MAKE_META_DATA(STRUCT_NAME, TABLE_NAME, N, ...)                       \
  static constexpr inline std::array<frozen::string, N> arr_##STRUCT_NAME = { \
      MARCO_EXPAND(MACRO_CONCAT(CON_STR, N)(__VA_ARGS__))};                   \
  static constexpr inline std::string_view fields_##STRUCT_NAME = {           \
      MAKE_NAMES(__VA_ARGS__)};                                               \
  static constexpr inline std::string_view name_##STRUCT_NAME = TABLE_NAME;   \
  MAKE_META_DATA_IMPL(STRUCT_NAME,                                            \
                      MAKE_ARG_LIST(N, &STRUCT_NAME::FIELD, __VA_ARGS__))

#define REFLECTION_ALIAS_IMPL(STRUCT_NAME, ALIAS, N, ...) \
  MAKE_META_DATA_IMPL_ALIAS(STRUCT_NAME, ALIAS, __VA_ARGS__)

#define FLDALIAS(a, b) \
  std::pair { a, b }

template <typename... Args>
constexpr auto get_mem_ptr_tp(Args... pair) {
  return std::make_tuple(std::get<0>(pair)...);
}

template <size_t N, typename... Args>
constexpr std::array<frozen::string, N> get_alias_arr(Args... pairs) {
  return std::array<frozen::string, sizeof...(Args)>{
      frozen::string(std::get<1>(pairs))...};
}

#define MAKE_META_DATA_IMPL_ALIAS(STRUCT_NAME, ALIAS, ...)                    \
  [[maybe_unused]] inline static auto iguana_reflect_members(                 \
      STRUCT_NAME const &) {                                                  \
    struct reflect_members {                                                  \
      constexpr decltype(auto) static apply_impl() {                          \
        return iguana::detail::get_mem_ptr_tp(__VA_ARGS__);                   \
      }                                                                       \
      using size_type = std::integral_constant<                               \
          size_t, std::tuple_size_v<decltype(std::make_tuple(__VA_ARGS__))>>; \
      constexpr static std::string_view name() { return ALIAS; }              \
      constexpr static size_t value() { return size_type::value; }            \
      constexpr static std::array<frozen::string, size_type::value> arr() {   \
        return iguana::detail::get_alias_arr<size_type::value>(__VA_ARGS__);  \
      }                                                                       \
    };                                                                        \
    return reflect_members{};                                                 \
  }

#define MAKE_META_DATA_IMPL_EMPTY(STRUCT_NAME)                              \
  inline auto iguana_reflect_members(STRUCT_NAME const &) {                 \
    struct reflect_members {                                                \
      constexpr decltype(auto) static apply_impl() {                        \
        return std::make_tuple();                                           \
      }                                                                     \
      using size_type = std::integral_constant<size_t, 0>;                  \
      constexpr static std::string_view name() {                            \
        return std::string_view(#STRUCT_NAME, sizeof(#STRUCT_NAME) - 1);    \
      }                                                                     \
      constexpr static size_t value() { return size_type::value; }          \
      constexpr static std::array<frozen::string, size_type::value> arr() { \
        return arr_##STRUCT_NAME;                                           \
      }                                                                     \
    };                                                                      \
    return reflect_members{};                                               \
  }

#define MAKE_META_DATA_EMPTY(STRUCT_NAME)                                \
  constexpr inline std::array<frozen::string, 0> arr_##STRUCT_NAME = {}; \
  MAKE_META_DATA_IMPL_EMPTY(STRUCT_NAME)

template <typename... Args>
inline auto get_value_type(std::tuple<Args...>) {
  return std::variant<Args...>{};
}

inline constexpr frozen::string filter_str(const frozen::string &str) {
  if (str.size() > 3 && str[0] == '_' && str[1] == '_' && str[2] == '_') {
    auto ptr = str.data() + 3;
    return frozen::string(ptr, str.size() - 3);
  }
  return str;
}

template <typename T, size_t... Is>
inline constexpr auto get_iguana_struct_map_impl(
    const std::array<frozen::string, sizeof...(Is)> &arr, T &&t,
    std::index_sequence<Is...>) {
  using ValueType = decltype(get_value_type(t));
  return frozen::unordered_map<frozen::string, ValueType, sizeof...(Is)>{
      {filter_str(arr[Is]),
       ValueType{std::in_place_index<Is>, std::get<Is>(t)}}...};
}
}  // namespace iguana::detail

namespace iguana {
#define REFLECTION_WITH_NAME(STRUCT_NAME, TABLE_NAME, ...)            \
  MAKE_META_DATA(STRUCT_NAME, TABLE_NAME, GET_ARG_COUNT(__VA_ARGS__), \
                 __VA_ARGS__)

template <typename T>
inline auto iguana_reflect_type(const T &t);

inline std::unordered_map<
    std::string_view,
    std::vector<std::pair<std::string_view, std::string_view>>>
    g_iguana_custom_map;
template <typename T>
inline constexpr auto get_iguana_struct_map() {
  using reflect_members = decltype(iguana_reflect_type(std::declval<T>()));
  if constexpr (reflect_members::value() == 0) {
    return std::array<int, 0>{};
  }
  else {
    return detail::get_iguana_struct_map_impl(
        reflect_members::arr(), reflect_members::apply_impl(),
        std::make_index_sequence<reflect_members::value()>{});
  }
}

#define REFLECTION(STRUCT_NAME, ...)                                    \
  MAKE_META_DATA(STRUCT_NAME, #STRUCT_NAME, GET_ARG_COUNT(__VA_ARGS__), \
                 __VA_ARGS__)

#define REFLECTION_EMPTY(STRUCT_NAME) MAKE_META_DATA_EMPTY(STRUCT_NAME)

#define REFLECTION_ALIAS(STRUCT_NAME, ALIAS, ...) \
  REFLECTION_ALIAS_IMPL(                          \
      STRUCT_NAME, ALIAS,                         \
      std::tuple_size_v<decltype(std::make_tuple(__VA_ARGS__))>, __VA_ARGS__)

#ifdef _MSC_VER
#define IGUANA_UNIQUE_VARIABLE(str) MACRO_CONCAT(str, __COUNTER__)
#else
#define IGUANA_UNIQUE_VARIABLE(str) MACRO_CONCAT(str, __LINE__)
#endif
template <typename T>
struct iguana_required_struct;
#define REQUIRED_IMPL(STRUCT_NAME, N, ...)                      \
  template <>                                                   \
  struct iguana::iguana_required_struct<STRUCT_NAME> {          \
    inline static constexpr auto requied_arr() {                \
      std::array<std::string_view, N> arr_required = {          \
          MARCO_EXPAND(MACRO_CONCAT(CON_STR, N)(__VA_ARGS__))}; \
      return arr_required;                                      \
    }                                                           \
  };

#define REQUIRED(STRUCT_NAME, ...) \
  REQUIRED_IMPL(STRUCT_NAME, GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

template <class T, class = void>
struct has_iguana_required_arr : std::false_type {};

template <class T>
struct has_iguana_required_arr<
    T, std::void_t<decltype(iguana_required_struct<T>::requied_arr())>>
    : std::true_type {};

template <class T>
constexpr bool has_iguana_required_arr_v = has_iguana_required_arr<T>::value;

inline std::string_view trim_sv(std::string_view str) {
  std::string_view whitespaces(" \t\f\v\n\r");
  auto first = str.find_first_not_of(whitespaces);
  auto last = str.find_last_not_of(whitespaces);
  if (first == std::string_view::npos || last == std::string_view::npos)
    return std::string_view();
  return str.substr(first, last - first + 1);
}

inline int add_custom_fields(std::string_view key,
                             std::vector<std::string_view> v) {
  std::vector<std::pair<std::string_view, std::string_view>> vec;
  for (auto val : v) {
    std::string_view str = {val.data() + 1, val.size() - 2};
    size_t pos = str.find(',');
    if (pos == std::string_view::npos || pos == str.size() - 1) {
      continue;
    }

    std::string_view origin = str.substr(0, pos);
    std::string_view alias = str.substr(pos + 1);

    vec.push_back(std::make_pair(trim_sv(origin), trim_sv(alias)));
  }
  g_iguana_custom_map.emplace(key, std::move(vec));
  return 0;
}

#ifdef _MSC_VER
#define IGUANA_UNIQUE_VARIABLE(str) MACRO_CONCAT(str, __COUNTER__)
#else
#define IGUANA_UNIQUE_VARIABLE(str) MACRO_CONCAT(str, __LINE__)
#endif

#define CUSTOM_FIELDS_IMPL(STRUCT_NAME, N, ...)                                \
  inline auto IGUANA_UNIQUE_VARIABLE(STRUCT_NAME) = iguana::add_custom_fields( \
      #STRUCT_NAME, {MARCO_EXPAND(MACRO_CONCAT(CON_STR, N)(__VA_ARGS__))});

#define CUSTOM_FIELDS(STRUCT_NAME, ...) \
  CUSTOM_FIELDS_IMPL(STRUCT_NAME, GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

template <typename T>
using Reflect_members = decltype(iguana_reflect_members(std::declval<T>()));

template <typename T, typename = void>
struct is_public_reflection : std::false_type {};

template <typename T>
struct is_public_reflection<
    T, std::void_t<decltype(iguana_reflect_members(std::declval<T>()))>>
    : std::true_type {};

template <typename T>
constexpr bool is_public_reflection_v = is_public_reflection<T>::value;

template <typename T, typename = void>
struct is_private_reflection : std::false_type {};

template <typename T>
struct is_private_reflection<
    T, std::void_t<decltype(std::declval<T>().iguana_reflect_members(
           std::declval<T>()))>> : std::true_type {};

template <typename T>
constexpr bool is_private_reflection_v = is_private_reflection<T>::value;

template <typename T, typename = void>
struct is_reflection : std::false_type {};

template <typename T>
struct is_reflection<T, std::enable_if_t<is_private_reflection_v<T>>>
    : std::true_type {};

template <typename T>
struct is_reflection<T, std::enable_if_t<is_public_reflection_v<T>>>
    : std::true_type {};

template <typename T>
inline auto iguana_reflect_type(const T &t) {
  if constexpr (is_public_reflection_v<T>) {
    return iguana_reflect_members(t);
  }
  else {
    return t.iguana_reflect_members(t);
  }
}

template <typename T>
inline constexpr bool is_reflection_v = is_reflection<T>::value;

template <std::size_t index, template <typename...> typename Condition,
          typename Tuple, typename Owner>
constexpr int element_index_helper() {
  if constexpr (index == std::tuple_size_v<Tuple>) {
    return index;
  }
  else {
    using type_v = decltype(std::declval<Owner>().*
                            std::declval<std::tuple_element_t<index, Tuple>>());
    using item_type = std::decay_t<type_v>;

    return Condition<item_type>::value
               ? index
               : element_index_helper<index + 1, Condition, Tuple, Owner>();
  }
}

template <template <typename...> typename Condition, typename T>
constexpr int tuple_element_index() {
  using M = decltype(iguana_reflect_type(std::declval<T>()));
  using Tuple = decltype(M::apply_impl());
  return element_index_helper<0, Condition, Tuple, T>();
}

template <size_t I, typename T>
constexpr decltype(auto) get(T &&t) {
  using M = decltype(iguana_reflect_type(std::forward<T>(t)));
  using U = decltype(std::forward<T>(t).*(std::get<I>(M::apply_impl())));

  if constexpr (std::is_array_v<U>) {
    auto s = std::forward<T>(t).*(std::get<I>(M::apply_impl()));
    std::array<char, sizeof(U)> arr;
    memcpy(arr.data(), s, arr.size());
    return arr;
  }
  else
    return std::forward<T>(t).*(std::get<I>(M::apply_impl()));
}

template <template <typename...> typename Condition, typename T>
constexpr size_t get_type_index() {
  return tuple_element_index<Condition, T>();
}

template <typename T, size_t... Is>
constexpr auto get_impl(T const &t, std::index_sequence<Is...>) {
  return std::make_tuple(get<Is>(t)...);
}

template <typename T, size_t... Is>
constexpr auto get_impl(T &t, std::index_sequence<Is...>) {
  return std::make_tuple(std::ref(get<Is>(t))...);
}

template <typename T>
constexpr auto get(T const &t) {
  using M = decltype(iguana_reflect_type(t));
  return get_impl(t, std::make_index_sequence<M::value()>{});
}

template <typename T>
constexpr auto get_ref(T &t) {
  using M = decltype(iguana_reflect_type(t));
  return get_impl(t, std::make_index_sequence<M::value()>{});
}

template <typename T, size_t I>
constexpr const auto get_name() {
  using M = decltype(iguana_reflect_type(std::declval<T>()));
  static_assert(I < M::value(), "out of range");
  return M::arr()[I];
}

template <typename T>
constexpr const auto get_name(size_t i) {
  using M = decltype(iguana_reflect_type(std::declval<T>()));
  //		static_assert(I<M::value(), "out of range");
  return M::arr()[i];
}

template <typename T>
constexpr const std::string_view get_name() {
  using M = decltype(iguana_reflect_type(std::declval<T>()));
  return M::name();
}

template <typename T>
constexpr const std::string_view get_fields() {
  using M = Reflect_members<T>;
  return M::fields();
}

template <typename T>
constexpr std::enable_if_t<is_reflection<T>::value, size_t> get_value() {
  using M = decltype(iguana_reflect_type(std::declval<T>()));
  return M::value();
}

template <typename T>
constexpr std::enable_if_t<!is_reflection<T>::value, size_t> get_value() {
  return 1;
}

template <typename T>
constexpr auto get_array() {
  using M = decltype(iguana_reflect_type(std::declval<T>()));
  return M::arr();
}

template <typename T>
inline bool has_custom_fields(std::string_view key = "") {
  if (key.empty()) {
    return !g_iguana_custom_map.empty();
  }

  return g_iguana_custom_map.find(key) != g_iguana_custom_map.end();
}

template <typename T>
inline std::string_view get_custom_fields(std::string_view origin) {
  constexpr std::string_view name = get_name<T>();
  auto it = g_iguana_custom_map.find(name);
  if (it == g_iguana_custom_map.end()) {
    return "";
  }

  auto &vec = it->second;
  auto find_it = std::find_if(vec.begin(), vec.end(), [origin](auto &pair) {
    return pair.first == origin;
  });

  if (find_it == vec.end()) {
    return "";
  }

  return (*find_it).second;
}

template <typename T>
constexpr auto get_index(std::string_view name) {
  using M = decltype(iguana_reflect_type(std::declval<T>()));
  constexpr auto arr = M::arr();

  auto it = std::find_if(arr.begin(), arr.end(), [name](auto str) {
    return (str == name);
  });

  return std::distance(arr.begin(), it);
}

template <class Tuple, class F, std::size_t... Is>
void tuple_switch(std::size_t i, Tuple &&t, F &&f, std::index_sequence<Is...>) {
  ((i == Is &&
    ((std::forward<F>(f)(std::get<Is>(std::forward<Tuple>(t)))), false)),
   ...);
}

//-------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------//
template <typename... Args, typename F, std::size_t... Idx>
constexpr void for_each(std::tuple<Args...> &t, F &&f,
                        std::index_sequence<Idx...>) {
  (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}),
   ...);
}

template <typename... Args, typename F, std::size_t... Idx>
constexpr void for_each(const std::tuple<Args...> &t, F &&f,
                        std::index_sequence<Idx...>) {
  (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}),
   ...);
}

template <typename T, typename F>
constexpr std::enable_if_t<is_reflection<T>::value> for_each(T &&t, F &&f) {
  using M = decltype(iguana_reflect_type(std::forward<T>(t)));
  for_each(M::apply_impl(), std::forward<F>(f),
           std::make_index_sequence<M::value()>{});
}

template <typename T, typename F>
constexpr std::enable_if_t<is_tuple<std::decay_t<T>>::value> for_each(T &&t,
                                                                      F &&f) {
  constexpr const size_t SIZE = std::tuple_size_v<std::decay_t<T>>;
  for_each(std::forward<T>(t), std::forward<F>(f),
           std::make_index_sequence<SIZE>{});
}
}  // namespace iguana
#endif  // IGUANA_REFLECTION_HPP
