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

  CHECK(inst.at("") == dev);

  CHECK_THROWS_AS(inst[inst.size()], std::out_of_range);
  CHECK_THROWS_AS(inst.at("no such a device"), std::out_of_range);
}

TEST_CASE("test device") {
  ib_device_t dev({});

  auto name = dev.name();
  CHECK(!name.empty());
  auto ctx = dev.context();
  CHECK(ctx);
  auto pd = dev.pd();
  CHECK(pd);
  CHECK(dev.port() == 1);
  CHECK(dev.gid_index() == 1);
  auto& attr = dev.attr();
}

TEST_CASE("test device invalid") {
  CHECK_THROWS_AS(ib_device_t dev({0, "", 3}), std::domain_error);
  CHECK_THROWS_AS(ib_device_t dev({0, "no such"}), std::out_of_range);
}

TEST_CASE("test reg_mr") {
  ib_device_t dev({});

  char buf[256];
  auto mr = dev.reg_memory(buf, 256);
  dev.dereg_memory(mr);
  CHECK(mr);
}