#include <ylt/simd_util/simd_str_split.h>
#include "doctest.h"

TEST_CASE("test string_view split")
{
    std::string_view sv_tmp = "hello world\t127.0.0.1\t1024\twww.yalantinglibs.com";

    auto tokens = ylt::split_sv(sv_tmp, '\t');

    CHECK(tokens[0] == "hello world");
    CHECK(tokens[1] == "127.0.0.1");
    CHECK(tokens[2] == "1024");
    CHECK(tokens[3] == "www.yalantinglibs.com");
}

TEST_CASE("test string split")
{
    std::string_view sv_tmp = "hello world\t127.0.0.1\t1024\twww.yalantinglibs.com";

    auto tokens = ylt::split_str(sv_tmp, '\t');

    CHECK(tokens[0] == "hello world");
    CHECK(tokens[1] == "127.0.0.1");
    CHECK(tokens[2] == "1024");
    CHECK(tokens[3] == "www.yalantinglibs.com");
}