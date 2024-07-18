#pragma once
#include "common_macro.hpp"

#define YLT_CALL0(f, o)
#define YLT_CALL1(f, o, _1) f(CONCAT_MEMBER(o, _1));

#include "generate/foreach_user_macro_gen.hpp"

#define YLT_FOREACH_(fun, funarg, ...) \
  YLT_CONCAT(YLT_CALL, YLT_ARG_COUNT(__VA_ARGS__))(fun, funarg, __VA_ARGS__)
#define YLT_FOREACH(fun, funarg, ...) YLT_FOREACH_(fun, funarg, __VA_ARGS__)
