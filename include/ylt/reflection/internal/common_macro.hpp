#pragma once
#include "args_count.hpp"
#define YLT_CONCAT_(l, r) l##r

#define YLT_CONCAT(l, r) YLT_CONCAT_(l, r)

#define CONCAT_MEMBER(t, x) t.x

#define CONCAT_ADDR(T, x) &T::x
