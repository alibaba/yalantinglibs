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
#pragma once

#include <algorithm>
#include <cstdint>
#define CMAKE_INCLUDE
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

#include <memory>
#include <ylt/easylog.hpp>

namespace coro_io {

struct barex_device_t {
  accl::barex::XDevice* device_ = nullptr;
  std::unique_ptr<accl::barex::XSimpleMempool> mempool_ = nullptr;

  struct config_t {
    std::string dev_name;
    uint64_t max_buffer_size = UINT32_MAX;
  };

  static accl::barex::XDeviceManager* get_manager() noexcept {
    accl::barex::XDeviceManager* manager;
    accl::barex::XDeviceManager::Singleton(manager);
    assert(manager != nullptr);
    return manager;
  }

  static std::vector<std::shared_ptr<barex_device_t>> get_devices(
      accl::barex::XDeviceManager* manager,
      const barex_device_t::config_t& config) noexcept {
    std::vector<std::shared_ptr<barex_device_t>> vec;
    auto devs = manager->AllDevices();
    vec.resize(devs.size());
    for (std::size_t i = 0; i < devs.size(); ++i) {
      vec[i] = std::make_shared<barex_device_t>();
      vec[i]->init(devs[i], config);
    }
    return vec;
  }
  accl::barex::BarexResult init(accl::barex::XDevice* device,
                                const barex_device_t::config_t& config) {
    device_ = device;
    if (device == nullptr) {
      ELOG_INFO << "init XDevice failed";
      return accl::barex::BarexResult::BAREX_ERR_DEVICES;
    }
    accl::barex::XSimpleMempool* mempool;
    auto result = accl::barex::XSimpleMempool::NewInstance(
        mempool, "mempool of device " + device_->name_, {device},
        config.max_buffer_size);
    if (result) {
      ELOG_INFO << "init XSimpleMempool failed:" << result;
      return result;
    }
    assert(mempool != nullptr);
    mempool_.reset(mempool);
    return {};
  }
  ~barex_device_t() {
    ELOG_INFO << "destruct barex device";
    if (mempool_) {
      mempool_->Shutdown();
      mempool_->WaitStop();
    }
  }
};

inline std::vector<std::shared_ptr<barex_device_t>>& get_global_barex_devices(
    const barex_device_t::config_t& config = {}) {
  static auto manager = barex_device_t::get_manager();
  static auto barex_devices = barex_device_t::get_devices(manager, config);
  return barex_devices;
}
inline std::shared_ptr<barex_device_t> get_global_barex_device(
    const barex_device_t::config_t& config = {}) {
  auto& devices = get_global_barex_devices(config);
  if (devices.empty()) {
    ELOG_ERROR << "There is no rdma device in machine";
    return nullptr;
  }
  if (config.dev_name == "") {
    return devices[0];
  }
  auto iter = std::find_if(devices.begin(), devices.end(),
                           [&](const std::shared_ptr<barex_device_t>& device) {
                             return config.dev_name == device->device_->name_;
                           });
  if (iter == devices.end()) {
    iter = devices.begin();
    ELOG_ERROR << "Could not found rdma device with name:" << config.dev_name
               << ". Instead by default device:" << iter->get()->device_->name_;
  }
  return *iter;
}

}  // namespace coro_io