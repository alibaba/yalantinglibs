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
#include <system_error>
#include "ylt/util/expected.hpp"
#define CMAKE_INCLUDE
#include <accl/barex/barex.h>
#include <accl/barex/barex_types.h>
#include <accl/barex/xconfig_util.h>
#include <accl/barex/xconnector.h>
#include <accl/barex/xcontext.h>
#include <accl/barex/xsimple_mempool.h>
#include <accl/barex/xthreadpool.h>
#include <accl/barex/xtimer.h>
#include <async_simple/Common.h>
#include <async_simple/coro/Lazy.h>
#include "barex_context.hpp"
#include <memory>
#include <random>
#include <ylt/util/random.hpp>
#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/easylog.hpp>
namespace coro_io {
class barex_socket_t: public std::enable_shared_from_this<barex_socket_t> {
 public:
  async_simple::coro::Lazy<std::error_code> connect(
      const std::string& host, const std::string& port) noexcept {
    std::vector<asio::ip::tcp::endpoint> eps;
    ELOG_TRACE << "start resolve host: " << host << ":" << port;
    auto [ec, iter] = co_await coro_io::async_resolve(
        executor_, host, port);
    if (ec) {
      ELOG_WARN << "async_resolve address "<< (host+":"+port) <<"failed:" << ec.message();
      co_return ec;
    }
    asio::ip::tcp::resolver::iterator end;
    while (iter != end) {
      eps.push_back(iter->endpoint());
      ++iter;
    }
    if (eps.empty()) [[unlikely]] {
      co_return std::make_error_code(std::errc::not_connected);
    }
    co_return co_await connect(eps);
    co_return std::error_code{};
  }

  template <typename EndPointSeq>
  async_simple::coro::Lazy<std::error_code> connect(
      const EndPointSeq& endpoints) noexcept {
    std::uniform_int_distribution<std::size_t> dis(0, endpoints.size()-1);
    const asio::ip::tcp::endpoint& endpoint = endpoints[dis(ylt::util::random_engine())];

    context_ = co_await get_user_context(executor_);

    auto result = co_await coro_io::async_io<ylt::expected<std::pair<accl::barex::XChannel *, accl::barex::Status>,accl::barex::BarexResult>>(
      [&](auto&& cb) {
        accl::barex::BarexResult result = context_->connector_->Connect(endpoint.address().to_string(),endpoint.port(),[&](accl::barex::XChannel *channel, accl::barex::Status s) {
          cb(std::pair{channel,s});
        });
        if (!result) {
          cb (result);
        }
      },
      *this);
    if (result.has_value()) {
      if (result.value().second.IsOk()) {
        ELOG_INFO << "connection success.";
        channel_.reset(result.value().first);
      }
      else {
        ELOG_INFO << "connection failed:" << result.value().second.ErrMsg();
      }
    }
    else {
      ELOG_INFO << "connection failed:" << result.error();
    }

  }

 private:
  std::unique_ptr<accl::barex::XChannel> channel_;
  barex_context_t* barex_context_;
  coro_io::ExecutorWrapper<>* executor_;
};
}  // namespace coro_io