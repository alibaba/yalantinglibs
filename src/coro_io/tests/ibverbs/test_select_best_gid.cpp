/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstring>
#include <vector>

#include "doctest.h"
#include "ylt/coro_io/ibverbs/ib_device.hpp"

using namespace coro_io;

namespace {

// Helper: create a fake ibv_gid_entry
ibv_gid_entry make_gid_entry(uint32_t gid_index, uint32_t gid_type,
                             uint32_t ndev_ifindex, const uint8_t gid_raw[16]) {
  ibv_gid_entry entry{};
  entry.gid_index = gid_index;
  entry.gid_type = gid_type;
  entry.ndev_ifindex = ndev_ifindex;
  memcpy(entry.gid.raw, gid_raw, 16);
  return entry;
}

// IPv4-mapped global address: ::ffff:10.0.0.1
const uint8_t gid_ipv4_global[] = {0, 0, 0,    0,    0,  0, 0, 0,
                                   0, 0, 0xff, 0xff, 10, 0, 0, 1};

// IPv4-mapped loopback: ::ffff:127.0.0.1
const uint8_t gid_ipv4_loopback[] = {0, 0, 0,    0,    0,   0, 0, 0,
                                     0, 0, 0xff, 0xff, 127, 0, 0, 1};

// IPv4-mapped link-local: ::ffff:169.254.1.1
const uint8_t gid_ipv4_link_local[] = {0, 0, 0,    0,    0,   0,   0, 0,
                                       0, 0, 0xff, 0xff, 169, 254, 1, 1};

// IPv6 loopback: ::1
const uint8_t gid_ipv6_loopback[] = {0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 1};

// IPv6 link-local: fe80::1
const uint8_t gid_ipv6_link_local[] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0,
                                       0,    0,    0, 0, 0, 0, 0, 1};

// IPv6 global unicast: 2001:db8::1
const uint8_t gid_ipv6_global[] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
                                   0,    0,    0,    0,    0, 0, 0, 1};

}  // namespace

TEST_CASE("select_best_gid: empty entries returns -1") {
  std::vector<ibv_gid_entry> entries;
  CHECK(ib_device_t::select_best_gid(entries) == -1);
}

TEST_CASE("select_best_gid: device type priority IB > RoCEv2 > RoCEv1") {
  std::vector<ibv_gid_entry> entries;
  entries.push_back(
      make_gid_entry(0, IBV_GID_TYPE_ROCE_V1, 1, gid_ipv4_global));
  entries.push_back(
      make_gid_entry(1, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv4_global));
  entries.push_back(make_gid_entry(2, IBV_GID_TYPE_IB, 0, gid_ipv6_global));

  CHECK(ib_device_t::select_best_gid(entries) == 2);
}

TEST_CASE("select_best_gid: RoCEv2 preferred over RoCEv1") {
  std::vector<ibv_gid_entry> entries;
  entries.push_back(
      make_gid_entry(5, IBV_GID_TYPE_ROCE_V1, 1, gid_ipv4_global));
  entries.push_back(
      make_gid_entry(3, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv4_global));

  CHECK(ib_device_t::select_best_gid(entries) == 3);
}

TEST_CASE("select_best_gid: same type, global > link-local > loopback") {
  SUBCASE("global beats link-local") {
    std::vector<ibv_gid_entry> entries;
    entries.push_back(
        make_gid_entry(0, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv6_link_local));
    entries.push_back(
        make_gid_entry(1, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv4_global));

    CHECK(ib_device_t::select_best_gid(entries) == 1);
  }

  SUBCASE("link-local beats loopback") {
    std::vector<ibv_gid_entry> entries;
    entries.push_back(
        make_gid_entry(0, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv4_loopback));
    entries.push_back(
        make_gid_entry(1, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv6_link_local));

    CHECK(ib_device_t::select_best_gid(entries) == 1);
  }

  SUBCASE("global beats loopback") {
    std::vector<ibv_gid_entry> entries;
    entries.push_back(
        make_gid_entry(0, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv6_loopback));
    entries.push_back(
        make_gid_entry(1, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv6_global));

    CHECK(ib_device_t::select_best_gid(entries) == 1);
  }
}

TEST_CASE("select_best_gid: IPv4 link-local classified correctly") {
  std::vector<ibv_gid_entry> entries;
  entries.push_back(
      make_gid_entry(0, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv4_link_local));
  entries.push_back(
      make_gid_entry(1, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv4_global));

  CHECK(ib_device_t::select_best_gid(entries) == 1);
}

TEST_CASE("select_best_gid: type priority overrides address rank") {
  // IB with link-local should still beat RoCEv2 with global
  std::vector<ibv_gid_entry> entries;
  entries.push_back(
      make_gid_entry(0, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv6_global));
  entries.push_back(make_gid_entry(1, IBV_GID_TYPE_IB, 0, gid_ipv6_link_local));

  CHECK(ib_device_t::select_best_gid(entries) == 1);
}

TEST_CASE("select_best_gid: single loopback entry still selected") {
  std::vector<ibv_gid_entry> entries;
  entries.push_back(
      make_gid_entry(7, IBV_GID_TYPE_ROCE_V2, 1, gid_ipv4_loopback));

  CHECK(ib_device_t::select_best_gid(entries) == 7);
}

TEST_CASE("select_best_gid: real mlx5_bond scenario") {
  // Real GID table from mlx5_bond_0:
  // INDEX 0: fe80::0225:9dff:fe78:82ef  v1  (link-local, RoCEv1)
  // INDEX 1: fe80::0225:9dff:fe78:82ef  v2  (link-local, RoCEv2)
  // INDEX 2: fd03:4516:1090:4e40::1     v1  (ULA global, RoCEv1)
  // INDEX 3: fd03:4516:1090:4e40::1     v2  (ULA global, RoCEv2)
  // INDEX 4: fe80:4516:1090:4e40::1     v1  (link-local, RoCEv1)
  // INDEX 5: fe80:4516:1090:4e40::1     v2  (link-local, RoCEv2)
  //
  // Expected best: INDEX 3 (RoCEv2 + global ULA address)

  const uint8_t gid_fe80_mac[] = {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x02, 0x25, 0x9d, 0xff,
                                  0xfe, 0x78, 0x82, 0xef};
  const uint8_t gid_fd03[] = {0xfd, 0x03, 0x45, 0x16, 0x10, 0x90, 0x4e, 0x40,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  const uint8_t gid_fe80_alt[] = {0xfe, 0x80, 0x45, 0x16, 0x10, 0x90,
                                  0x4e, 0x40, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x01};

  std::vector<ibv_gid_entry> entries;
  entries.push_back(make_gid_entry(0, IBV_GID_TYPE_ROCE_V1, 1, gid_fe80_mac));
  entries.push_back(make_gid_entry(1, IBV_GID_TYPE_ROCE_V2, 1, gid_fe80_mac));
  entries.push_back(make_gid_entry(2, IBV_GID_TYPE_ROCE_V1, 1, gid_fd03));
  entries.push_back(make_gid_entry(3, IBV_GID_TYPE_ROCE_V2, 1, gid_fd03));
  entries.push_back(make_gid_entry(4, IBV_GID_TYPE_ROCE_V1, 1, gid_fe80_alt));
  entries.push_back(make_gid_entry(5, IBV_GID_TYPE_ROCE_V2, 1, gid_fe80_alt));

  CHECK(ib_device_t::select_best_gid(entries) == 3);
}
