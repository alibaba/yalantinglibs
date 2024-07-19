#pragma once
#include "common_macro.hpp"

#define ADD_VIEW(str) std::string_view(#str, sizeof(#str) - 1)

#define SEPERATOR ,
#define CON_STR0()
#define CON_STR1(element, ...) ADD_VIEW(element)
#include "generate/member_names_macro_gen.hpp"
