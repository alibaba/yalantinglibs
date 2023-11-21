#define STRUCT_PACK_OPTIMIZE
#include "doctest.h"
#include "ylt/struct_pack.hpp"
TEST_CASE("test width too big") {
  SUBCASE("1") {
    std::string buffer;
    buffer.push_back(0b11000);
    auto result = struct_pack::deserialize<struct_pack::DISABLE_ALL_META_INFO,
                                           std::string>(buffer);
    REQUIRE(result.has_value() == false);
    if constexpr (sizeof(std::size_t) < 8) {
      CHECK(result.error() == struct_pack::errc::too_width_size);
    }
    else {
      CHECK(result.error() == struct_pack::errc::no_buffer_space);
    }
  }
  SUBCASE("2") {
    std::string buffer;
    buffer.push_back(0b11000);
    std::size_t len = 0;
    auto result = struct_pack::deserialize<struct_pack::DISABLE_ALL_META_INFO,
                                           std::string>(buffer, len);
    REQUIRE(result.has_value() == false);
    if constexpr (sizeof(std::size_t) < 8) {
      CHECK(result.error() == struct_pack::errc::too_width_size);
    }
    else {
      CHECK(result.error() == struct_pack::errc::no_buffer_space);
    }
  }
  SUBCASE("3") {
    std::string buffer;
    buffer.push_back(0b11000);
    auto result =
        struct_pack::get_field<std::pair<std::string, std::string>, 0>(buffer);
    REQUIRE(result.has_value() == false);
    if constexpr (sizeof(std::size_t) < 8) {
      CHECK(result.error() == struct_pack::errc::too_width_size);
    }
    else {
      CHECK(result.error() == struct_pack::errc::no_buffer_space);
    }
  }
  SUBCASE("4") {
    std::string buffer;
    buffer.push_back(0b11);
    auto result = struct_pack::deserialize<
        std::pair<std::string, struct_pack::compatible<int>>>(buffer);
    REQUIRE(result.has_value() == false);
    if constexpr (sizeof(std::size_t) < 8) {
      CHECK(result.error() == struct_pack::errc::too_width_size);
    }
    else {
      CHECK(result.error() == struct_pack::errc::no_buffer_space);
    }
  }
}