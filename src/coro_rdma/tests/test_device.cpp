#include <ylt/coro_rdma/ib_device.hpp>

#include "doctest.h"

using namespace ylt::coro_rdma;

TEST_CASE("test device list") {
  auto& inst = ib_devices_t::instance();
  auto& names = inst.dev_names();
  CHECK(!names.empty());
}