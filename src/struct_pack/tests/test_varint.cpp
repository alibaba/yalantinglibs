#include <cstdint>
#include <ylt/struct_pack.hpp>

#include "doctest.h"
#include "test_struct.hpp"

using namespace struct_pack;
TEST_CASE("test uint32") {
  {
    var_uint32_t v = 0;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = 128 * 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = 128 * 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_uint32_t v = UINT32_MAX;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
}

TEST_CASE("test int32") {
  {
    var_int32_t v = 0;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_int32_t v = 128 / 2 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_int32_t v = (128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_int32_t v = 128 / 2;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_int32_t v = (128 / 2 + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_int32_t v = 128 * 128 / 2 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_int32_t v = (128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_int32_t v = 128 * 128 / 2;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_int32_t v = (128 * 128 / 2 + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_int32_t v = 128 * 128 * 128 / 2 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_int32_t v = (128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_int32_t v = 128 * 128 * 128 / 2;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_int32_t v = (128 * 128 * 128 / 2 + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_int32_t v = 128 * 128 * 128 * 128 / 2 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_int32_t v = (128 * 128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_int32_t v = 128 * 128 * 128 * 128 / 2;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_int32_t v = (128 * 128 * 128 * 128 / 2 + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_int32_t v = INT32_MAX;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_int32_t v = INT32_MIN;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int32_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
}

TEST_CASE("test uint64") {
  {
    var_uint64_t v = 0;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 128 * 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 128 * 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 6);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 6);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 7);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 7);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 8);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 8);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 9);
    CHECK(v == v2);
  }
  {
    var_uint64_t v =
        1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 9);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = 1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 10);
    CHECK(v == v2);
  }
  {
    var_uint64_t v = UINT64_MAX;
    auto buf = serialize(v);
    auto v2 = deserialize<var_uint64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 10);
    CHECK(v == v2);
  }
}

TEST_CASE("test int64") {
  {
    var_int64_t v = 0;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_int64_t v = 128 / 2 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 1);
    CHECK(v == v2);
  }
  {
    var_int64_t v = 128 / 2;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (128 / 2 + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_int64_t v = 128 * 128 / 2 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 2);
    CHECK(v == v2);
  }
  {
    var_int64_t v = 128 * 128 / 2;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (128 * 128 / 2 + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_int64_t v = 128 * 128 * 128 / 2 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 3);
    CHECK(v == v2);
  }
  {
    var_int64_t v = 128 * 128 * 128 / 2;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (128 * 128 * 128 / 2 + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_int64_t v = 128 * 128 * 128 * 128 / 2 - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (128 * 128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 4);
    CHECK(v == v2);
  }
  {
    var_int64_t v = 128 * 128 * 128 * 128 / 2;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (128 * 128 * 128 * 128 / 2 + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 / 2) - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 5);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 / 2);
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 6);
    CHECK(v == v2);
  }
  {
    var_int64_t v = ((1ll * 128 * 128 * 128 * 128 * 128 / 2) + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 6);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 * 128 / 2) - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 6);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 6);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 * 128 / 2);
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 7);
    CHECK(v == v2);
  }
  {
    var_int64_t v = ((1ll * 128 * 128 * 128 * 128 * 128 * 128 / 2) + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 7);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 7);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 7);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2);
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 8);
    CHECK(v == v2);
  }
  {
    var_int64_t v =
        ((1ll * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 8);
    CHECK(v == v2);
  }
  {
    var_int64_t v =
        (1ll * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 8);
    CHECK(v == v2);
  }
  {
    var_int64_t v =
        (1ll * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 8);
    CHECK(v == v2);
  }
  {
    var_int64_t v = (1ll * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2);
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 9);
    CHECK(v == v2);
  }
  {
    var_int64_t v =
        ((1ll * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) + 1) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 9);
    CHECK(v == v2);
  }
  {
    var_int64_t v =
        (1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) - 1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 9);
    CHECK(v == v2);
  }
  {
    var_int64_t v =
        (1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) * -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 9);
    CHECK(v == v2);
  }
  {
    var_int64_t v =
        (1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2);
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 10);
    CHECK(v == v2);
  }
  {
    var_int64_t v =
        ((1ull * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128 / 2) + 1) *
        -1;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 10);
    CHECK(v == v2);
  }
  {
    var_int64_t v = INT64_MAX;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 10);
    CHECK(v == v2);
  }
  {
    var_int64_t v = INT64_MIN;
    auto buf = serialize(v);
    auto v2 = deserialize<var_int64_t>(buf);
    CHECK(detail::calculate_payload_size(v).total == 10);
    CHECK(v == v2);
  }
}

template <typename T>
struct VarRect {
  T x0, y0, x1, y1;
  bool operator==(const VarRect<T> & o) const {
    return x0==o.x0&&y0==o.y0&&x1==o.x1&&y1==o.y1;
  }
};
template <typename T>
struct nested_object2 {
  int id;
  std::string name;
  person p;
  complicated_object o;
  VarRect<T> x;
  bool operator==(const nested_object2<T> &O) const {
    return id==O.id&&name==O.name&&p==O.p&&o==O.o&&x==O.x;
  }
};

TEST_CASE("test nested") {
  {
    VarRect<var_int32_t> rect{
        .x0 = INT32_MAX, .y0 = INT32_MIN, .x1 = 123, .y1 = 0};
    auto buf = serialize(rect);
    auto rect2 = deserialize<VarRect<var_int32_t>>(buf);
    CHECK(rect == rect2);
    nested_object2<var_int32_t> obj{.id = 123,
                                    .name = "jfslkf",
                                    .p = person{.age = 24, .name = "Hello"},
                                    .o = complicated_object{},
                                    .x = rect};
    auto buf2 = serialize(obj);
    auto obj2 = deserialize<nested_object2<var_int32_t>>(buf2);
    CHECK(obj2 == obj);
  }
  {
    VarRect<var_int64_t> rect{
        .x0 = INT64_MAX, .y0 = INT64_MIN, .x1 = 123, .y1 = 0};
    auto buf = serialize(rect);
    auto rect2 = deserialize<VarRect<var_int64_t>>(buf);
    CHECK(rect == rect2);
    nested_object2<var_int64_t> obj{.id = 123,
                                    .name = "jfslkf",
                                    .p = person{.age = 24, .name = "Hello"},
                                    .o = complicated_object{},
                                    .x = rect};
    auto buf2 = serialize(obj);
    auto obj2 = deserialize<nested_object2<var_int64_t>>(buf2);
    CHECK(obj2 == obj);
  }
  {
    VarRect<var_uint32_t> rect{
        .x0 = UINT32_MAX, .y0 = 21321343, .x1 = 123, .y1 = 0};
    auto buf = serialize(rect);
    auto rect2 = deserialize<VarRect<var_uint32_t>>(buf);
    CHECK(rect == rect2);
    nested_object2<var_uint32_t> obj{.id = 123,
                                     .name = "jfslkf",
                                     .p = person{.age = 24, .name = "Hello"},
                                     .o = complicated_object{},
                                     .x = rect};
    auto buf2 = serialize(obj);
    auto obj2 = deserialize<nested_object2<var_uint32_t>>(buf2);
    CHECK(obj2 == obj);
  }
  {
    VarRect<var_uint64_t> rect{
        .x0 = UINT64_MAX, .y0 = 1233143, .x1 = 123, .y1 = 0};
    auto buf = serialize(rect);
    auto rect2 = deserialize<VarRect<var_uint64_t>>(buf);
    CHECK(rect == rect2);
    nested_object2<var_uint64_t> obj{.id = 123,
                                     .name = "jfslkf",
                                     .p = person{.age = 24, .name = "Hello"},
                                     .o = complicated_object{},
                                     .x = rect};
    auto buf2 = serialize(obj);
    auto obj2 = deserialize<nested_object2<var_uint64_t>>(buf2);
    CHECK(obj2 == obj);
  }
}

TEST_CASE("test varint zip size") {
  {
    std::vector<var_int32_t> vec;
    for (int i = 0; i < 100; ++i) vec.push_back(rand() % 128 - 64);
    auto buf = struct_pack::serialize<
        std::string,struct_pack::type_info_config::disable>(vec);
    CHECK(buf.size() == 4 + 1 + 1 * 100);
    CHECK(detail::calculate_payload_size(vec).total == 100);
    CHECK(detail::calculate_payload_size(vec).size_cnt == 1);
  }
  {
    std::vector<var_int64_t> vec;
    for (int i = 0; i < 100; ++i) vec.push_back(rand() % 128 - 64);
    auto buf = struct_pack::serialize<
        std::string,struct_pack::type_info_config::disable>(vec);
    CHECK(buf.size() == 4 + 1 + 1 * 100);
    CHECK(detail::calculate_payload_size(vec).total == 100);
    CHECK(detail::calculate_payload_size(vec).size_cnt == 1);
  }
  {
    std::vector<var_uint32_t> vec;
    for (int i = 0; i < 100; ++i) vec.push_back(rand() % 128);
    auto buf = struct_pack::serialize<
        std::string,struct_pack::type_info_config::disable>(vec);
    CHECK(buf.size() == 4 + 1 + 1 * 100);
    CHECK(detail::calculate_payload_size(vec).total == 100);
    CHECK(detail::calculate_payload_size(vec).size_cnt == 1);
  }
  {
    std::vector<var_uint64_t> vec;
    for (int i = 0; i < 100; ++i) vec.push_back(rand() % 128);
    auto buf = struct_pack::serialize<
        std::string, struct_pack::type_info_config::disable>(vec);
    CHECK(buf.size() == 4 + 1 + 1 * 100);
    CHECK(detail::calculate_payload_size(vec).total == 100);
    CHECK(detail::calculate_payload_size(vec).size_cnt == 1);
  }
}