#include <cstdint>
#include <fstream>
#include <ylt/struct_pack.hpp>

#include "doctest.h"
#include "test_struct.hpp"
using namespace struct_pack;
using namespace doctest;

TEST_CASE("testing deserialize other platform data") {
  std::ifstream ifs("binary_data/test_cross_platform.dat");
  if (!ifs.is_open()) {
    ifs.open("src/struct_pack/tests/binary_data/test_cross_platform.dat");
  }
  REQUIRE(ifs.is_open());
  auto result = struct_pack::deserialize<complicated_object>(ifs);
  REQUIRE(result.has_value());
  auto object = create_complicated_object();
  CHECK(result.value() == object);
}

TEST_CASE("testing deserialize other platform data without debug info") {
  std::ifstream ifs("binary_data/test_cross_platform_without_debug_info.dat");
  if (!ifs.is_open()) {
    ifs.open(
        "src/struct_pack/tests/binary_data/"
        "test_cross_platform_without_debug_info.dat");
  }
  REQUIRE(ifs.is_open());
  auto result = struct_pack::deserialize<complicated_object>(ifs);
  REQUIRE(result.has_value());
  auto object = create_complicated_object();
  CHECK(result.value() == object);
}

TEST_CASE("testing serialize other platform data") {
  std::ifstream ifs("binary_data/test_cross_platform.dat");
  if (!ifs.is_open()) {
    ifs.open("src/struct_pack/tests/binary_data/test_cross_platform.dat");
  }
  REQUIRE(ifs.is_open());
  std::string content(std::istreambuf_iterator<char>{ifs},
                      std::istreambuf_iterator<char>{});
  auto object = create_complicated_object();
  auto buffer =
      struct_pack::serialize<std::string,
                             struct_pack::type_info_config::enable>(object);
  CHECK(buffer == content);
}

TEST_CASE("testing serialize other platform data") {
  std::ifstream ifs("binary_data/test_cross_platform_without_debug_info.dat");
  if (!ifs.is_open()) {
    ifs.open(
        "src/struct_pack/tests/binary_data/"
        "test_cross_platform_without_debug_info.dat");
  }
  REQUIRE(ifs.is_open());
  std::string content(std::istreambuf_iterator<char>{ifs},
                      std::istreambuf_iterator<char>{});
  auto object = create_complicated_object();
  auto buffer =
      struct_pack::serialize<std::string,
                             struct_pack::type_info_config::disable>(object);
  CHECK(buffer == content);
}