#include "doctest.h"

// Type literals are disabled in release builds. Define NDEBUG before any YLT
// header so this regression runs in every build type.
#ifndef NDEBUG
#define NDEBUG
#endif

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <ylt/struct_pack.hpp>

namespace {
struct VariadicRequestConfig {
  uint64_t replica_num = 1;
  struct_pack::compatible<std::vector<std::string>, 4> residency_affinity_ids;
};

using RequestId = std::pair<uint64_t, uint64_t>;
using VariadicRequest = std::tuple<RequestId, std::string, uint64_t,
                                   VariadicRequestConfig, std::string>;
}  // namespace

TEST_CASE(
    "variadic serialization preserves compatible fields without type "
    "literals") {
  const RequestId id{1, 2};
  const std::string key = "key";
  const std::string tenant = "default";
  const uint64_t size = 4096;
  VariadicRequestConfig config;

  SUBCASE("non-empty compatible value") {
    config.residency_affinity_ids = std::vector<std::string>{"affinity-a"};
    const auto wire = struct_pack::serialize(id, key, size, config, tenant);
    const auto decoded = struct_pack::deserialize<VariadicRequest>(wire);

    REQUIRE(decoded.has_value());
    const auto& affinity = std::get<3>(*decoded).residency_affinity_ids;
    CHECK(affinity.has_value());
    if (affinity.has_value()) {
      CHECK(affinity->size() == 1);
      CHECK(affinity->front() == "affinity-a");
    }
  }

  SUBCASE("present but empty compatible value") {
    config.residency_affinity_ids = std::vector<std::string>{};
    const auto wire = struct_pack::serialize(id, key, size, config, tenant);
    const auto decoded = struct_pack::deserialize<VariadicRequest>(wire);

    REQUIRE(decoded.has_value());
    const auto& affinity = std::get<3>(*decoded).residency_affinity_ids;
    CHECK(affinity.has_value());
    if (affinity.has_value()) {
      CHECK(affinity->empty());
    }
  }
}
