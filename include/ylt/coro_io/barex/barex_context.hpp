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

#include <accl/barex/xlistener.h>
#include <accl/barex/barex_types.h>
#include <accl/barex/xconfig_util.h>
#include <accl/barex/xconnector.h>
#include <accl/barex/xcontext.h>
#include <accl/barex/xsimple_mempool.h>
#include <accl/barex/xthreadpool.h>
#include <accl/barex/xtimer.h>
#include <accl/barex/xchannel.h>
#include <async_simple/coro/Lazy.h>

#include <memory>
#include <string>

#include "barex_device.hpp"

namespace coro_io {

  struct barex_context_t {

    class coro_barex_callback_t : public accl::barex::XChannelCallback {
      public:
      void OnRecvCall(accl::barex::XChannel *channel, char *buf, size_t len, accl::barex::x_msg_header header) {
        // TODO
      }
    }

    std::unique_ptr<accl::barex::XContext> context_ = nullptr;
    std::unique_ptr<accl::barex::XConnector> connector_ = nullptr;

    barex_context_t(barex_device_t& dev) {
      init(dev);
    }
  private:
    accl::barex::BarexResult init(barex_device_t& dev) {
      accl::barex::XConnector* context;
      auto result = accl::barex::XContext::NewInstance(context, accl::barex::ContextConfig{}, new coro_barex_callback_t{},dev.device_, dev.mempool_.get());
      if (result) {
        ELOG_INFO << "init XContext failed:" << result;
        return result;
      }
      assert(context!=nullptr);
      context_.reset(context);
      accl::barex::XConnector* connector;
      result = accl::barex::XConnector::NewInstance(connector, 1, accl::barex::TIMER_30S, {context});
      if (result) {
        ELOG_INFO << "init XConnector failed:" << result;
        return result;
      }
      assert(connector!=nullptr);
      connector_.reset(connector);
      return result;
    }
  };



}  // namespace coro_io