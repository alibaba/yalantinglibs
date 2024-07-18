#pragma once
#include "common_macro.hpp"
#include "generate/visit_user_macro_gen.hpp"

#define YLT_VISIT_(func, object, ...)                   \
  YLT_CONCAT(YLT_CALL_ARGS, YLT_ARG_COUNT(__VA_ARGS__)) \
  (func, object, __VA_ARGS__)
#define YLT_VISIT(func, object, ...) YLT_VISIT_(func, object, __VA_ARGS__)
