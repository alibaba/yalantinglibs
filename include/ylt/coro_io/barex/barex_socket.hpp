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

#include <queue>
#include <stdexcept>

#define CMAKE_INCLUDE
#include <accl/barex/barex.h>
#include <accl/barex/barex_types.h>
#include <accl/barex/xchannel.h>
#include <accl/barex/xconfig_util.h>
#include <accl/barex/xconnector.h>
#include <accl/barex/xcontext.h>
#include <accl/barex/xsimple_mempool.h>
#include <accl/barex/xthreadpool.h>
#include <accl/barex/xtimer.h>
#include <async_simple/Common.h>
#include <async_simple/coro/Lazy.h>

#include <cstring>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <system_error>
#include <vector>
#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/easylog.hpp>
#include <ylt/util/random.hpp>

#include "async_simple/Promise.h"
#include "async_simple/coro/FutureAwaiter.h"
#include "barex_context.hpp"
#include "ylt/coro_io/barex/barex_acceptor.hpp"
#include "ylt/coro_io/barex/barex_device.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
namespace coro_io {

namespace detail {

inline std::size_t write_to_position(std::span<char>& src,
                                     std::vector<std::span<char>>& buffers,
                                     std::size_t index = 0) {
  std::size_t total = 0;
  for (auto it = buffers.begin(); it != buffers.end() && src.size(); ++it) {
    auto& buf = *it;
    if (index == 0 || index < buf.size()) {
      size_t to_write = std::min(buf.size() - index, src.size());
      memcpy(buf.data() + index, src.data(), to_write);
      src = src.subspan(to_write);
      total += to_write;
      index = 0;
    }
    else if (index > 0) {
      index -= buf.size();
    }
  }
  return total;
}
}  // namespace detail

class barex_socket_t {
 public:
  struct config_t {
    std::size_t buffer_size = 1024 * 1024;
    std::size_t max_buffer_cnt = 8;
    std::shared_ptr<barex_device_t> dev = nullptr;
  };

  enum io_type { recv, send };

 public:
  barex_socket_t() {}

  barex_socket_t(std::shared_ptr<barex_socket_impl_t> impl)
      : impl_(std::move(impl)) {};

  barex_socket_t(std::shared_ptr<barex_socket_impl_t> impl,
                 const config_t& config);
  barex_socket_t(barex_socket_t&& other) = default;

  barex_socket_t& operator=(barex_socket_t&&) = default;

  barex_socket_t(coro_io::ExecutorWrapper<>* executor, config_t config)
      : impl_(std::make_shared<barex_socket_impl_t>(executor, config)) {}

  barex_socket_t(config_t config)
      : impl_(std::make_shared<barex_socket_impl_t>(config)) {}

  // create socket for server
  barex_socket_t(std::shared_ptr<barex_context_t> barex_context,
                 accl::barex::XChannel* channel)
      : impl_(std::make_shared<barex_socket_impl_t>(std::move(barex_context),
                                                    channel)) {}
  // create socket for server
  barex_socket_t(std::shared_ptr<barex_context_t> barex_context,
                 accl::barex::XChannel* channel, config_t conf)
      : impl_(std::make_shared<barex_socket_impl_t>(std::move(barex_context),
                                                    channel, conf)) {}

  auto get_impl() const noexcept { return impl_; }
  operator bool() const { return impl_ != nullptr; }
  auto get_executor();
  coro_io::ExecutorWrapper<>* get_coro_executor();
  bool has_closed() const;
  config_t& get_config();
  void close();
  void cancel();
  ~barex_socket_t();

 private:
  std::shared_ptr<barex_socket_impl_t> impl_;
};

class barex_socket_impl_t;
namespace detail {
struct connect_controller : public accl::barex::XChannel::UserData {
  connect_controller() = default;
  connect_controller(std::shared_ptr<barex_socket_impl_t>&& soc)
      : soc(std::move(soc)) {}
  std::shared_ptr<barex_socket_impl_t> soc;
  std::vector<std::pair<std::vector<char>, uint64_t>> unhandle_datas;
  bool has_closed = false;
};
}  // namespace detail
class barex_socket_impl_t
    : public std::enable_shared_from_this<barex_socket_impl_t>,
      public accl::barex::XChannel::UserData {
 public:
  async_simple::coro::Lazy<std::error_code> connect(
      const std::string& host, const std::string& port) noexcept {
    std::vector<asio::ip::tcp::endpoint> eps;
    ELOG_TRACE << "start resolve host: " << host << ":" << port;
    auto [ec, iter] = co_await coro_io::async_resolve(executor_, host, port);
    if (ec) {
      ELOG_WARN << "async_resolve address " << (host + ":" + port)
                << "failed:" << ec.message();
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
  }

  template <typename EndPointSeq>
  async_simple::coro::Lazy<std::error_code> connect(
      const EndPointSeq& endpoints) noexcept {
    std::uniform_int_distribution<std::size_t> dis(0, endpoints.size() - 1);
    const asio::ip::tcp::endpoint& endpoint =
        endpoints[dis(ylt::util::random_engine())];
    if (barex_context_ == nullptr) [[unlikely]] {
      barex_context_ = get_barex_context(executor_, config_.dev);
      if (barex_context_ == nullptr) {
        ELOG_ERROR << "init XContext failed, get nullptr ctx";
        co_return std::make_error_code(std::errc::not_connected);
      }
    }
    ELOG_DEBUG << "accl barex start connect to "
               << endpoint.address().to_string() << ":" << endpoint.port()
               << ", this=" << (void*)this;
    auto result = co_await coro_io::async_io<
        std::pair<std::error_code, accl::barex::XChannel*>>(
        [&](auto&& cb) {
          accl::barex::BarexResult result =
              barex_context_->get_connector()->Connect(
                  endpoint.address().to_string(), endpoint.port(),
                  [cb](accl::barex::XChannel* channel,
                       accl::barex::Status s) mutable {
                    ELOG_DEBUG << "accl barex connect callback";
                    cb(std::pair<std::error_code, accl::barex::XChannel*>{
                        detail::to_std_error_code(s), channel});
                  });
          if (result) {
            cb(std::pair<std::error_code, accl::barex::XChannel*>{
                detail::to_std_error_code(result), nullptr});
          }
        },
        *this);
    if (result.first.value() == 0) {
      ELOG_DEBUG << "connection success."
                 << " this=" << (void*)this;
      channel_ = result.second;
      channel_->SetUserData(
          "barex_soc",
          std::make_shared<detail::connect_controller>(shared_from_this()));
    }
    else {
      ELOG_DEBUG << "connection failed:" << result.first.message()
                 << ". this=" << (void*)this;
    }
    co_return result.first;
  }

  auto get_executor() const noexcept { return executor_->get_asio_executor(); }

  auto get_coro_executor() const noexcept { return executor_; }

  void cancel() noexcept { close(); }

 private:
  static async_simple::coro::Lazy<void> free_channel_memory(
      std::weak_ptr<barex_context_t> ctx_weak,
      accl::barex::XChannel* channel) noexcept {
    const int max_sleep_cnt = 100;
    std::chrono::milliseconds sleep_duration(100);
    int sleep_cnt = 0;
    for (; sleep_cnt < max_sleep_cnt; sleep_cnt++) {
      if (auto ctx = ctx_weak.lock(); ctx) {
        if (channel->IsDead()) {
          accl::barex::Status s =
              ctx->get_connector()->CloseAndDeleteChannel(channel);
          if (!s.IsOk()) {
            ELOG_WARN << "barex socket delete failed, channel address="
                      << (void*)channel << " error msg: " << s.ErrMsg();
          }
          break;
        }
        else {
          co_await coro_io::sleep_for(sleep_duration);
        }
      }
      else {
        // context is dead, barex socket has deleted
        break;
      }
    }
    if (sleep_cnt >= max_sleep_cnt) {
      ELOG_WARN << "barex socket delete xchannel memory cost too much time, we "
                   "won't delete it, channel address="
                << (void*)channel;
    }
  }

 public:
  void close() noexcept {
    bool expected = false;
    if (!has_closed_.compare_exchange_strong(expected, true)) {
      return;
    }
    ELOG_DEBUG << "barex socket closing, this=" << (void*)this;
    if (barex_context_) {
      if (channel_) {
        if (channel_->IsServerChannel()) {
          accl::barex::Status s = accl::barex::Status::OK();
          if (channel_->IsActive()) {
            s = channel_->Close();
            if (!s.IsOk()) {
              ELOG_WARN << "barex socket closed failed, this=" << (void*)this
                        << " error msg: " << s.ErrMsg();
            }
          }
          s = channel_->Destroy();
          if (!s.IsOk()) {
            ELOG_WARN << "barex socket closed failed, this=" << (void*)this
                      << " error msg: " << s.ErrMsg();
          }
        }
        else {
          auto connector = barex_context_->get_connector();
          accl::barex::Status s = connector->CloseChannel(
              channel_,
              [self = (void*)this, channel = channel_, connector = connector,
               ctx = barex_context_](accl::barex::Status s) {
                if (!s.IsOk()) {
                  ELOG_INFO << "barex socket closed failed (may server close "
                               "first), this="
                            << self << " error msg: " << s.ErrMsg();
                }
                if (channel->IsActive()) {
                  s = channel->Close();
                  if (!s.IsOk()) {
                    ELOG_WARN << "barex socket closed failed, this=" << self
                              << " error msg: " << s.ErrMsg();
                  }
                }

                s = channel->Destroy();
                if (!s.IsOk()) {
                  ELOG_WARN
                      << "barex socket closed failed, this=" << (void*)self
                      << " error msg: " << s.ErrMsg();
                }

                free_channel_memory(ctx, channel).start([](auto&&) {
                });
              });
          if (!s.IsOk()) {
            ELOG_WARN << "barex socket closed failed, this=" << (void*)this
                      << " error msg: " << s.ErrMsg();
          }
        }
      }
    }

    if (this->barex_context_) {
      barex_context_ = nullptr;
    }
    if (recv_cb_) {
      recv_cb_(std::make_error_code(std::errc::operation_canceled), 0);
    }
    if (promise_.has_value()) {
      promise_->setValue(std::make_error_code(std::errc::operation_canceled));
    }
  }

  bool has_closed() const noexcept {
    return channel_ == nullptr || has_closed_;
  }

  barex_socket_impl_t(coro_io::ExecutorWrapper<>* executor,
                      barex_socket_t::config_t config = {})
      : executor_(executor), config_(std::move(config)) {
    ELOG_DEBUG << "create barex socket, this=" << (void*)this;
    if (config_.dev == nullptr) {
      config_.dev = coro_io::get_global_barex_device();
    }
    if (config_.dev == nullptr) {
      throw std::runtime_error("no barex device available");
    }
  }
  barex_socket_impl_t(barex_socket_t::config_t config)
      : executor_(coro_io::get_global_executor()), config_(std::move(config)) {
    ELOG_DEBUG << "create barex socket, this=" << (void*)this;
    if (config_.dev == nullptr) {
      config_.dev = coro_io::get_global_barex_device();
    }
    if (config_.dev == nullptr) {
      throw std::runtime_error("no barex device available");
    }
  }
  barex_socket_impl_t(std::shared_ptr<barex_context_t> barex_context,
                      accl::barex::XChannel* channel,
                      barex_socket_t::config_t conf = {})
      : barex_context_(barex_context),
        channel_(channel),
        executor_(barex_context->executor_),
        config_(std::move(conf)) {
    ELOG_DEBUG << "create barex socket:" << this;
    config_.dev = barex_context->dev_;
    if (config_.dev == nullptr) {
      throw std::runtime_error("no barex device available");
    }
  }

  void on_recv(std::error_code ec, std::span<char> data,
               uint64_t bytes_number) {
    assert(now_waiting_recv_bytes_ <= bytes_number);
    auto write_pos = bytes_number - now_waiting_recv_bytes_;
    auto sz = data.size();
    ELOG_TRACE << "recv " << sz << " bytes, total bytes: " << read_target_size_
               << " bytes_number:" << bytes_number << ", this=" << (void*)this;
    std::size_t copy_size = 0;
    if (read_target_.size()) {
      copy_size = detail::write_to_position(data, read_target_, write_pos);
      read_target_size_ += copy_size;
      do {
        if (read_buffer_.size() &&
            read_buffer_.top().bytes_number >= now_waiting_recv_bytes_ &&
            read_buffer_.top().bytes_number <=
                now_waiting_recv_bytes_ + expected_read_size_) {
          copy_size = detail::write_to_position(
              read_buffer_.top().view, read_target_,
              read_buffer_.top().bytes_number - now_waiting_recv_bytes_);
          read_target_size_ += copy_size;
          if (read_buffer_.top().view.empty()) {
            read_buffer_.pop();
          }
          else {
            break;
          }
        }
        else {
          break;
        }
      } while (true);
    }
    if (data.size()) {
      read_buffer_.push(
          buffer_record(bytes_number + copy_size,
                        std::vector<char>{data.begin(), data.end()}));
      if (read_buffer_.size() > 100) {
        ELOG_WARN << "client send too fast, too many data in read_buffer, this="
                  << (void*)this;
        // TODO: close connection?
      }
    }
    if (recv_cb_ && (ec || (read_target_size_ > 0 &&
                            read_target_size_ >= expected_read_size_))) {
      ELOG_TRACE << "recv data over, this=" << (void*)this;
      now_waiting_recv_bytes_ += read_target_size_;
      auto read_target_size = read_target_size_;
      read_target_size_ = 0;
      read_target_ = {};
      auto recv_cb = std::move(recv_cb_);
      recv_cb(ec, read_target_size);
    }
  }

  void post_recv(std::vector<std::span<char>>&& mem, bool is_read_some,
                 auto&& cb) {
    std::size_t total_size = 0;
    for (auto& e : mem) {
      total_size += e.size();
    }
    read_target_ = std::move(mem);
    ELOG_TRACE << "start post_recv, size:" << total_size
               << ", this=" << (void*)this;
    read_target_size_ = 0;
    expected_read_size_ = is_read_some ? 0 : total_size;
    recv_cb_ = std::move(cb);
  }

  async_simple::coro::Lazy<std::error_code> waiting_write_over() {
    ELOG_TRACE << "waiting promise now, used buffer cnt:" << used_buffer_cnt_
               << " max buffer cnt:" << config_.max_buffer_cnt
               << ", this=" << (void*)this;
    ;
    promise_ = async_simple::Promise<std::error_code>{};
    auto ec = co_await promise_->getFuture();
    ELOG_TRACE << "wake up from promise,recv data:" << ec.message()
               << ", this=" << (void*)this;
    co_return ec;
  }

  bool is_send_queue_full() {
    return used_buffer_cnt_ >= config_.max_buffer_cnt;
  }

  std::size_t consume_buffer(std::span<char>& buffer) {
    if (read_buffer_.empty()) {
      ELOG_TRACE
          << "consume over, there is no data in barex socket buffer, this="
          << (void*)this;
      return 0;
    }
    std::size_t total = 0;
    while (buffer.size() && read_buffer_.size() &&
           read_buffer_.top().bytes_number <= now_waiting_recv_bytes_) {
      auto sz = read_buffer_.top().consume(buffer);
      now_waiting_recv_bytes_ += sz;
      total += sz;
      if (read_buffer_.top().view.empty()) {
        read_buffer_.pop();
      }
    }
    ELOG_TRACE << "consume over, get " << total
               << " bytes, this=" << (void*)this;
    return total;
  }

  std::error_code post_send(std::span<char> data) {
    assert(data.size() <= config_.buffer_size);
    accl::barex::memp_t mem;
    accl::barex::BarexResult result =
        channel_->AllocLimitBuffer(mem, config_.buffer_size, accl::barex::CPU);
    if (result) {
      ELOG_WARN << "allocate write buffer failed:" << result
                << ", this=" << (void*)this;
      return detail::to_std_error_code(result);
    }
    mem.buf_len = data.size();
    memcpy(mem.buf, data.data(), data.size());
    ++used_buffer_cnt_;
    ELOG_TRACE << "send " << data.size()
               << " bytes, flags= " << total_send_bytes_
               << ", this=" << (void*)this;
    result = channel_->Send(
        mem, true, {.flags = total_send_bytes_},
        [self = shared_from_this(), len = data.size(), flag = total_send_bytes_,
         ptr_debug = (void*)this](accl::barex::Status s) {
          ELOG_TRACE << "send data of " << len << "bytes with flag=" << flag
                     << " over, status:" << s.ErrMsg()
                     << ", this=" << ptr_debug;
          --self->used_buffer_cnt_;
          if (self->promise_.has_value()) {
            auto promise = std::move(self->promise_.value());
            self->promise_ = std::nullopt;
            promise.setValue(detail::to_std_error_code(s));
          }
        });
    total_send_bytes_ += data.size();
    if (result) {
      channel_->ReleaseBuffer(mem.buf, accl::barex::CPU);
      --used_buffer_cnt_;
    }
    return detail::to_std_error_code(result);
  }

  std::size_t get_buffer_size() { return config_.buffer_size; }

  barex_socket_t::config_t& get_config() { return config_; }

  const barex_socket_t::config_t& get_config() const { return config_; }

  ~barex_socket_impl_t() { close(); }

 private:
  std::shared_ptr<barex_context_t> barex_context_ = nullptr;
  accl::barex::XChannel* channel_ = nullptr;
  // client:
  coro_io::ExecutorWrapper<>* executor_ = nullptr;
  std::atomic<bool> has_closed_ = false;
  std::vector<std::span<char>> read_target_;
  std::size_t read_target_size_ = 0;
  std::size_t used_buffer_cnt_ = 0, expected_read_size_ = 0;
  uint64_t total_send_bytes_ = 0, now_waiting_recv_bytes_ = 0;
  struct buffer_record {
    buffer_record(uint64_t bytes, std::vector<char> buf)
        : bytes_number(bytes), buffer(buf), view(buffer) {}
    uint64_t bytes_number;
    mutable std::vector<char> buffer;
    mutable std::span<char> view;
    friend bool operator<(const buffer_record& l,
                          const buffer_record& r) noexcept {
      return l.bytes_number > r.bytes_number;
    }
    std::size_t consume(std::span<char>& target) const {
      auto consume_size = std::min(view.size(), target.size());
      memcpy(target.data(), view.data(), consume_size);
      view = view.subspan(consume_size);
      target = target.subspan(consume_size);
      return consume_size;
    }
  };
  std::priority_queue<buffer_record> read_buffer_;
  std::function<void(std::error_code, std::size_t)> recv_cb_;
  std::optional<async_simple::Promise<std::error_code>> promise_;
  barex_socket_t::config_t config_;
};

inline barex_socket_t::barex_socket_t(std::shared_ptr<barex_socket_impl_t> impl,
                                      const config_t& config)
    : impl_(std::move(impl)) {
  impl_->get_config() = config;
  impl_->get_config().max_buffer_cnt =
      std::max(config.max_buffer_cnt, std::size_t{1});
}
inline auto barex_socket_t::get_executor() { return impl_->get_executor(); }
inline coro_io::ExecutorWrapper<>* barex_socket_t::get_coro_executor() {
  return impl_->get_coro_executor();
}
inline bool barex_socket_t::has_closed() const { return impl_->has_closed(); }
inline barex_socket_t::config_t& barex_socket_t::get_config() {
  return impl_->get_config();
}
inline void barex_socket_t::close() { impl_->close(); }
inline void barex_socket_t::cancel() { impl_->cancel(); }
inline barex_socket_t::~barex_socket_t() {
  if (impl_) {
    impl_->close();
  }
}

inline accl::barex::BarexResult barex_context_t::set_channel_callback() {
  auto result =
      context_->SetChannelConnectedHook([this](accl::barex::XChannel* channel) {
        if (channel->IsServerChannel()) {
          auto ctrl = std::make_shared<detail::connect_controller>();
          auto soc = std::make_shared<barex_socket_impl_t>(
              this->shared_from_this(), channel);
          ctrl->soc = soc;
          auto flag = channel->SetUserData("barex_soc", ctrl);
          if (auto acceptor = this->acceptor_.lock(); acceptor) {
            acceptor->accept_connection(soc);
          }
          if (!flag) [[unlikely]] {
            ctrl = dynamic_pointer_cast<detail::connect_controller>(
                channel->GetUserData("barex_soc"));
            assert(ctrl != nullptr);
            asio::dispatch(
                executor_->get_asio_executor(),
                [ctrl = std::move(ctrl), soc = std::move(soc)]() mutable {
                  ctrl->soc = std::move(soc);
                  if (ctrl->unhandle_datas.size()) {
                    ELOG_DEBUG << "has unrecv data, piece cnt="
                               << ctrl->unhandle_datas.size();
                    for (auto& e : ctrl->unhandle_datas) {
                      ctrl->soc->on_recv(std::error_code{}, e.first, e.second);
                    }
                    ctrl->unhandle_datas.clear();
                    if (ctrl->has_closed) {
                      ctrl->soc->close();
                    }
                  }
                });
          }
        }
      });
  if (result) {
    ELOG_WARN << "set channel connect callback failed:" << result;
    return result;
  }
  result = context_->SetChannelClosedHook([](accl::barex::XChannel* channel) {
    auto data = channel->GetUserData("barex_soc");
    auto ctrl = dynamic_pointer_cast<detail::connect_controller>(data);
    if (ctrl != nullptr) {
      ELOG_TRACE << "channel close callback running, this="
                 << (void*)ctrl->soc.get();
      ctrl->has_closed = true;
      if (ctrl->soc) [[likely]] {
        ctrl->soc->close();
        channel->RemoveUserData("barex_soc");
      }
    }
  });
  if (result) {
    ELOG_WARN << "set channel closed callback failed:" << result;
  }
  return result;
}

inline async_simple::coro::Lazy<std::shared_ptr<barex_socket_impl_t>>
barex_acceptor_impl_t::accept() {
  auto ret = co_await accept_impl();
  if (!executor_->currentThreadInExecutor()) {
    co_await coro_io::dispatch(executor_->get_asio_executor());
  }
  co_return std::move(ret);
}
inline async_simple::coro::Lazy<std::shared_ptr<barex_socket_impl_t>>
barex_acceptor_impl_t::accept_impl() {
  std::shared_ptr<barex_socket_impl_t> soc;
  do {
    sockets.try_dequeue(soc);
    if (soc) {
      new_connection_cnt_--;
    }
    else {
      ELOG_TRACE << "waiting for new connection";
      auto lock_guard = co_await mutex_.coScopedLock();
      co_await cv_.wait(mutex_, [this]() {
        return new_connection_cnt_ > 0 || has_closed_;
      });
    }
  } while (!soc && !has_closed_);
  if (soc) {
    ELOG_TRACE << "get new connection, this=" << (void*)soc.get();
  }
  co_return soc;
}

inline void barex_recv_callback_t::OnRecvCall(
    accl::barex::XChannel* channel, char* buf, size_t len,
    accl::barex::x_msg_header header) {
  std::shared_ptr<detail::connect_controller> ctrl =
      std::make_shared<detail::connect_controller>();
  auto flag = channel->SetUserData("barex_soc", ctrl);
  if (!flag) [[likely]] {
    ctrl = std::dynamic_pointer_cast<detail::connect_controller>(
        channel->GetUserData("barex_soc"));
    if (ctrl->soc) [[likely]] {
      ctrl->soc->on_recv(std::error_code{}, std::span{buf, len}, header.flags);
      return;
    }
  }
  ctrl->unhandle_datas.emplace_back(std::vector<char>(buf, buf + len),
                                    header.flags);
  ELOG_DEBUG << "OnRecvCall, socket not found, now piece cnt="
             << ctrl->unhandle_datas.size();
  return;
}

}  // namespace coro_io