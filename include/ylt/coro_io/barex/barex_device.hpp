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
#include <accl/barex/barex.h>
#include <accl/barex/barex_types.h>
#include <accl/barex/xconfig_util.h>
#include <accl/barex/xconnector.h>
#include <accl/barex/xcontext.h>
#include <accl/barex/xdevice_manager.h>
#include <accl/barex/xlistener.h>
#include <accl/barex/xsimple_mempool.h>
#include <accl/barex/xtimer.h>
#include <async_simple/coro/Lazy.h>

#include <ylt/easylog.hpp>

#include <memory>

namespace coro_io {

struct barex_device_t {
  accl::barex::XDevice* device_ = nullptr;
  std::unique_ptr<accl::barex::XSimpleMempool> mempool_ = nullptr;

  static accl::barex::XDeviceManager* get_manager() noexcept {
    accl::barex::XDeviceManager* manager;
    accl::barex::XDeviceManager::Singleton(manager);
    assert(manager!=nullptr);
    return manager;
  }

  static std::vector<barex_device_t> get_devices(accl::barex::XDeviceManager* manager) noexcept {
    std::vector<barex_device_t> vec;
    auto devs=manager->AllDevices();
    vec.resize(devs.size());
    for (std::size_t i=0;i<devs.size();++i) {
      e.init(devs[i]);
    }
    return vec;
  }
  accl::barex::BarexResult init(accl::barex::XDevice* device) {
    device_=device;
    if (device == nullptr) {
      ELOG_INFO << "init XDevice failed";
      return accl::barex::BarexResult::BAREX_ERR_DEVICES;
    }
    accl::barex::XSimpleMempool* mempool;
    auto result = accl::barex::XSimpleMempool::NewInstance(
        mempool, "test-mempool", {device});
    if (result) {
      ELOG_INFO << "init XSimpleMempool failed:" << result;
      return result;
    }
    assert(mempool != nullptr);
    mempool_.reset(mempool);
    return {};
  }
};



std::vector<barex_device_t>& get_global_barex_devices() {
  static auto manager = barex_device_t::get_manager();
  static auto barex_devices = barex_device_t::get_devices(manager);
  return barex_devices;
}
barex_device_t& get_global_barex_device(int device_id = 0) {
  return get_global_barex_devices()[device_id];
}


}  // namespace coro_io