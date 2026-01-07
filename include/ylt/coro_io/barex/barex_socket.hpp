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
    co_return std::error_code{};
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
        ELOG_INFO << "init XConnector failed, get nullptr ctx";
        co_return std::make_error_code(std::errc::not_connected);
      }
    }
    if (connector_ == nullptr) [[unlikely]] {
      accl::barex::XConnector* connector;
      auto result = accl::barex::XConnector::NewInstance(
          connector, 1, accl::barex::TIMER_30S,
          {barex_context_->context_.get()});
      if (result) {
        ELOG_INFO << "init XConnector failed:" << result;
        co_return detail::to_std_error_code(result);
      }
      assert(connector != nullptr);
      connector_.reset(connector);
    }
    ELOG_DEBUG << "accl barex connect to " << endpoint.address().to_string()
               << ":" << endpoint.port();
    auto result = co_await coro_io::async_io<
        std::pair<std::error_code, accl::barex::XChannel*>>(
        [&](auto&& cb) {
          accl::barex::BarexResult result = connector_->Connect(
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
      ELOG_INFO << "connection success.";
      channel_ = result.second;
      channel_->SetUserData("", shared_from_this());
    }
    else {
      ELOG_INFO << "connection failed:" << result.first.message();
    }
    co_return result.first;
  }

  auto get_executor() const noexcept { return executor_->get_asio_executor(); }

  auto get_coro_executor() const noexcept { return executor_; }

  void cancel() noexcept { close(); }

  void close() noexcept {
    bool expected = false;
    if (!has_closed_.compare_exchange_strong(expected, true)) {
      return;
    }
    ELOG_WARN << "socket closing";
    if (channel_ && channel_->IsActive()) {
      channel_->Close();
      channel_->Destroy();
      ELOG_WARN << "finish socket closing";
    }
    if (connector_) {
      ELOG_INFO << "connector shutdowning";
      connector_->Shutdown();
      ELOG_INFO << "connector waitstoping";
      connector_->WaitStop();
      ELOG_INFO << "connector finish destruct work";
      connector_ = nullptr;
    }
    if (this->barex_context_) {
      barex_context_ = nullptr;
    }
    if (recv_cb_) {
      ELOG_INFO << "start recv cb";
      recv_cb_(std::make_error_code(std::errc::operation_canceled), 0);
      ELOG_INFO << "finish recv cb";
    }
    if (promise_.has_value()) {
      ELOG_INFO << "start set value";
      promise_->setValue(std::make_error_code(std::errc::operation_canceled));
      ELOG_INFO << "finish set value";
    }
  }

  bool has_closed() const noexcept {
    return channel_ == nullptr || has_closed_;
  }

  barex_socket_impl_t(coro_io::ExecutorWrapper<>* executor,
                      barex_socket_t::config_t config = {})
      : executor_(executor), config_(std::move(config)) {
    ELOG_INFO << "create barex socket:" << this;
    if (config_.dev == nullptr) {
      config_.dev = coro_io::get_global_barex_device();
    }
    if (config_.dev == nullptr) {
      throw std::runtime_error("no barex device available");
    }
  }
  barex_socket_impl_t(barex_socket_t::config_t config)
      : executor_(coro_io::get_global_executor()), config_(std::move(config)) {
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
    ELOG_INFO << "create barex socket:" << this;
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
    ELOG_INFO << "recv " << sz << " bytes, total bytes: " << read_target_size_
              << " bytes_number:" << bytes_number;
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
        ELOG_WARN << "too many data in read_buffer";
        // TODO close connection
      }
    }
    if (recv_cb_ && (ec || (read_target_size_ > 0 &&
                            read_target_size_ >= expected_read_size_))) {
      ELOG_INFO << "recv over, call";
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
    ELOG_DEBUG << "post_recv, size:" << total_size << "this:" << (void*)this;
    read_target_size_ = 0;
    expected_read_size_ = is_read_some ? 0 : total_size;
    recv_cb_ = std::move(cb);
  }

  async_simple::coro::Lazy<std::error_code> waiting_write_over() {
    ELOG_DEBUG << "waiting promise now, used buffer cnt:" << used_buffer_cnt_ << " max buffer cnt:" << config_.max_buffer_cnt;
    promise_ = async_simple::Promise<std::error_code>{};
    auto ec = co_await promise_->getFuture();
    ELOG_DEBUG << "wake up from promise";
    if (ec) {
      ELOG_DEBUG << "recv data error: " << ec.message();
    }
    co_return ec;
  }

  bool is_send_queue_full() {
    return used_buffer_cnt_ >= config_.max_buffer_cnt;
  }

  std::size_t consume_buffer(std::span<char>& buffer) {
    ELOG_INFO << "read buffer cnt:" << read_buffer_.size()
              << ",now waiting recv bytes:" << now_waiting_recv_bytes_;
    if (read_buffer_.empty()) {
      return 0;
    }
    std::size_t total = 0;
    while (buffer.size() && read_buffer_.size() &&
           read_buffer_.top().bytes_number <= now_waiting_recv_bytes_) {
      ELOG_INFO << "before consume view size:"
                << read_buffer_.top().view.size();
      auto sz = read_buffer_.top().consume(buffer);
      now_waiting_recv_bytes_ += sz;
      ELOG_INFO << "after consume view size:" << read_buffer_.top().view.size();
      ELOG_INFO << "consume a piece of buffer, len: " << sz
                << ",now waiting recv bytes:" << now_waiting_recv_bytes_;
      total += sz;
      if (read_buffer_.top().view.empty()) {
        read_buffer_.pop();
      }
    }
    return total;
  }

  std::error_code post_send(std::span<char> data) {
    assert(data.size() <= config_.buffer_size);
    accl::barex::memp_t mem;
    accl::barex::BarexResult result =
        channel_->AllocLimitBuffer(mem, config_.buffer_size, accl::barex::CPU);
    if (result) {
      ELOG_WARN << "allocate write buffer failed:" << result;
      return detail::to_std_error_code(result);
    }
    mem.buf_len = data.size();
    memcpy(mem.buf, data.data(), data.size());
    ++used_buffer_cnt_;
    ELOG_INFO << "send " << data.size()
              << " bytes, flags= " << total_send_bytes_;
    result = channel_->Send(
        mem, true, {.flags = total_send_bytes_},
        [self = shared_from_this(), len = data.size()](accl::barex::Status s) {
          ELOG_DEBUG << "send len:" << len << ", callback:" << s.ErrMsg();
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
  std::unique_ptr<accl::barex::XConnector> connector_;
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
  impl_->get_config().max_buffer_cnt = std::max(config.max_buffer_cnt, std::size_t{1});
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
    ELOG_INFO << "trying close socket";
    impl_->close();
  }
}

inline accl::barex::BarexResult barex_context_t::set_channel_callback() {
  auto result =
      context_->SetChannelConnectedHook([this](accl::barex::XChannel* channel) {
        ELOG_WARN << "channel connect callback running";
        if (channel->IsServerChannel()) {
          auto soc = std::make_shared<barex_socket_impl_t>(
              this->shared_from_this(), channel);

          channel->SetUserData("", soc);
          if (auto acceptor = this->acceptor_.lock(); acceptor) {
            acceptor->accept_connection(std::move(soc));
          }
        }
      });
  if (result) {
    ELOG_WARN << "set channel connect callback failed:" << result;
    return result;
  }
  result = context_->SetChannelClosedHook([](accl::barex::XChannel* channel) {
    ELOG_WARN << "channel close callback running";
    auto data = channel->GetUserData("");
    auto socket = dynamic_pointer_cast<barex_socket_impl_t>(data);
    assert(socket != nullptr);
    socket->close();
    channel->RemoveUserData("");
    data = nullptr;
  });
  if (result) {
    ELOG_WARN << "set channel closed callback failed:" << result;
  }
  return result;
}

inline async_simple::coro::Lazy<std::shared_ptr<barex_socket_impl_t>>
barex_acceptor_impl_t::accept() {
  std::shared_ptr<barex_socket_impl_t> soc;
  do {
    sockets.try_dequeue(soc);
    if (soc) {
      ELOG_INFO << "get new connection";
      new_connection_cnt_--;
    }
    else {
      ELOG_INFO << "waiting for new connection";
      auto lock_guard = co_await mutex_.coScopedLock();

      co_await cv_.wait(mutex_, [this]() {
        return new_connection_cnt_ > 0 || has_closed_;
      });
    }
  } while (!soc && !has_closed_);
  co_return soc;
}

inline void barex_recv_callback_t::OnRecvCall(
    accl::barex::XChannel* channel, char* buf, size_t len,
    accl::barex::x_msg_header header) {
  ELOG_INFO << "recv callback running";
  auto socket =
      std::dynamic_pointer_cast<barex_socket_impl_t>(channel->GetUserData(""));
  socket->on_recv(std::error_code{}, std::span{buf, len}, header.flags);
  return;
}

}  // namespace coro_io