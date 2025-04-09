#include <ylt/coro_rdma/ib_device.hpp>

#include "doctest.h"

using namespace ylt::coro_rdma;

TEST_CASE("test device list") {
  auto& inst = ib_devices_t::instance();
  auto& names = inst.dev_names();
  CHECK(!names.empty());
  CHECK(inst.size());
  auto dev = inst.at(names[0]);
  CHECK(dev != nullptr);

  CHECK_THROWS_AS(inst[inst.size()], std::out_of_range);
  CHECK_THROWS_AS(inst.at("no such a device"), std::out_of_range);
}