#pragma once
#include <infiniband/verbs.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <system_error>
#include <utility>

#include "asio/dispatch.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/posix/stream_descriptor.hpp"
#include "async_simple/coro/Lazy.h"
#include "ib_device.hpp"
#include "ib_error.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/ibverbs/ib_buffer.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/struct_pack.hpp"

namespace coro_io {

class ib_socket_t {
 public:
  struct config_t {
    bool enable_zero_copy = true;
    uint32_t cq_size = 1024;
    std::size_t buffer_size = 8 * 1024 * 1024;
    std::chrono::milliseconds tcp_handshake_timeout =
        std::chrono::milliseconds{1000};
    ibv_qp_type qp_type = IBV_QPT_RC;
    ibv_qp_cap cap = {.max_send_wr = 512,
                      .max_recv_wr = 512,
                      .max_send_sge = 1,
                      .max_recv_sge = 1,
                      .max_inline_data = 0};
  };
  ib_socket_t(coro_io::ExecutorWrapper<>* executor,
              std::shared_ptr<ib_device_t> device, const config_t& config,
              ib_buffer_pool_t* buffer_pool = &g_ib_buffer_pool())
      : device_(std::move(device)),
        executor_(executor),
        ib_buffer_pool_(buffer_pool),
        state_(std::make_shared<shared_state_t>()) {
    init(config);
  }
  ib_socket_t(
      coro_io::ExecutorWrapper<>* executor = coro_io::get_global_executor(),
      std::shared_ptr<ib_device_t> device = coro_io::g_ib_device(),
      ib_buffer_pool_t* buffer_pool = &g_ib_buffer_pool())
      : device_(std::move(device)),
        executor_(executor),
        ib_buffer_pool_(buffer_pool),
        state_(std::make_shared<shared_state_t>()) {
    config_t config{};
    init(config);
  }
  ib_socket_t(ib_socket_t&&) = default;
  ib_socket_t& operator=(ib_socket_t&&) = default;

 private:
  void init(const config_t& config) {
    state_->channel_.reset(ibv_create_comp_channel(device_->context()));
    if (!state_->channel_) [[unlikely]] {
      throw std::system_error(std::make_error_code(std::errc{errno}));
    }
    state_->cq_.reset(ibv_create_cq(device_->context(), config.cq_size, NULL,
                                    state_->channel_.get(), 0));
    if (!state_->cq_) [[unlikely]] {
      throw std::system_error(std::make_error_code(std::errc{errno}));
    }
  }

 public:
  struct ib_socket_info {
    uint8_t gid[16];  // GID
    uint16_t lid;     // LID of the IB port
    uint32_t qp_num;  // QP number
    uint32_t length;  // buffer length
  };
  bool is_open() const noexcept {
    return state_->fd_ != nullptr && state_->fd_->is_open();
  }
  ib_buffer_pool_t& buffer_pool() const noexcept { return *ib_buffer_pool_; }
  using callback_t =
      std::function<void(std::pair<std::error_code, std::size_t>)>;

 private:
  struct shared_state_t {
    std::unique_ptr<ibv_comp_channel, ib_deleter> channel_;
    std::unique_ptr<ibv_cq, ib_deleter> cq_;
    std::unique_ptr<asio::posix::stream_descriptor> fd_;
    std::atomic<bool> has_close_ = 0;
    callback_t recv_cb_;
    callback_t send_cb_;
    void cancel() {
      if (fd_) {
        std::error_code ec;
        fd_->cancel(ec);
      }
    }
    void close(std::error_code& ec) {
      auto has_close = has_close_.exchange(true);
      if (!has_close) {
        if (fd_) {
          fd_->cancel(ec);
          if (ec)
            return;
          fd_->close(ec);
          if (ec) {
            return;
          }
        }
      }
    }
    std::error_code poll_completion() {
      ELOG_INFO << "IO EVENT COMMING";
      void* ev_ctx;
      auto cq = cq_.get();
      ELOG_INFO << "get_cq_event";
      int r = ibv_get_cq_event(channel_.get(), &cq, &ev_ctx);
      // assert(r >= 0);
      std::error_code ec;
      if (r) {
        ELOG_INFO << std::make_error_code(std::errc{r}).message();
        return std::make_error_code(std::errc{r});
      }
      ELOG_INFO << "ack_cq_events";
      ibv_ack_cq_events(cq, 1);
      r = ibv_req_notify_cq(cq, 0);
      if (r) {
        ELOG_INFO << std::make_error_code(std::errc{r}).message();
        return std::make_error_code(std::errc{r});
      }
      ELOG_INFO << "ibv_poll_cq";
      struct ibv_wc wc;
      int ne = 0;
      while ((ne = ibv_poll_cq(cq_.get(), 1, &wc)) != 0) {
        if (ne < 0) {
          ELOGV(ERROR, "poll CQ failed %d", ne);
          return std::make_error_code(std::errc::io_error);
        }
        if (ne > 0) {
          ELOGV(DEBUG, "Completion was found in CQ with status %d", wc.status);
        }

        ec = make_error_code(wc.status);
        if (wc.status != IBV_WC_SUCCESS) {
          ELOGV(DEBUG, "rdma failed with error code:%d", wc.status);
        }
        else {
          ELOG_INFO << "ready resume:" << (callback_t*)wc.wr_id;
          resume(std::pair{ec, (std::size_t)wc.byte_len},
                 *(callback_t*)wc.wr_id);
        }
      }
      return ec;
    }
  };

  std::shared_ptr<ib_device_t> device_;
  ib_buffer_pool_t* ib_buffer_pool_;
  std::unique_ptr<ibv_qp, ib_deleter> qp_;
  std::shared_ptr<shared_state_t> state_;
  config_t conf_;
  uint64_t remote_buffer_size_ = 0;
  coro_io::ExecutorWrapper<>* executor_;
  std::unique_ptr<asio::ip::tcp::socket> tcp_soc_;

 private:
  void init_qp() {
    struct ibv_qp_init_attr qp_init_attr;
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.qp_type = conf_.qp_type;
    qp_init_attr.sq_sig_all = 1;
    qp_init_attr.send_cq = state_->cq_.get();
    qp_init_attr.recv_cq = state_->cq_.get();
    qp_init_attr.cap = conf_.cap;
    auto qp = ibv_create_qp(device_->pd(), &qp_init_attr);
    if (qp == nullptr) {
      throw std::system_error(std::make_error_code(std::errc{errno}));
    }
    qp_.reset(qp);
  }
  void modify_qp_to_init() {
    // modify the QP to init
    ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = device_->port();
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                           IBV_ACCESS_REMOTE_WRITE;
    int flags =
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    if (auto ec = ibv_modify_qp(qp_.get(), &attr, flags); ec) {
      throw std::system_error(std::make_error_code(std::errc{ec}));
    }
    ELOGV(INFO, "Modify QP to INIT done!");
  }
  void modify_qp_to_rtr(uint32_t remote_qpn, uint16_t dlid, uint8_t* dgid) {
    ibv_qp_attr attr{};
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_256;
    attr.dest_qp_num = remote_qpn;
    attr.rq_psn = 0;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 0x12;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = dlid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = device_->port();
    if (device_->gid_index() >= 0) {
      attr.ah_attr.is_global = 1;
      attr.ah_attr.port_num = 1;
      memcpy(&attr.ah_attr.grh.dgid, dgid, 16);
      attr.ah_attr.grh.flow_label = 0;
      attr.ah_attr.grh.hop_limit = 1;
      attr.ah_attr.grh.sgid_index = device_->gid_index();
      attr.ah_attr.grh.traffic_class = 0;
    }
    int flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
                IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC |
                IBV_QP_MIN_RNR_TIMER;
    if (auto ec = ibv_modify_qp(qp_.get(), &attr, flags); ec) {
      throw std::system_error(std::make_error_code(std::errc{ec}));
    }
    ELOGV(INFO, "Modify QP to RTR done!");
  }
  void modify_qp_to_rts() {
    ibv_qp_attr attr{};
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 0x12;  // 18
    attr.retry_cnt = 6;
    attr.rnr_retry = 0;
    attr.sq_psn = 0;
    attr.max_rd_atomic = 1;
    int flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    if (auto ec = ibv_modify_qp(qp_.get(), &attr, flags); ec != 0) {
      throw std::system_error(std::make_error_code(std::errc{ec}));
    }
    ELOGV(INFO, "Modify QP to RTS done!");
  }
  inline static void resume(std::pair<std::error_code, std::size_t>&& arg,
                            callback_t& handle) {
    ELOG_INFO << "resume callback:" << &handle;
    if (handle) [[likely]] {
      ELOG_INFO << "resuming:" << &handle;
      auto handle_tmp = std::move(handle);
      handle_tmp(std::move(arg));
      ELOG_INFO << "resume over,clear cb:" << &handle;
    }
    else {
      ELOG_INFO << "resume empty";
    }
  }

  void init_fd() {
    ELOG_INFO << "state_->channel_->fd:" << state_->channel_->fd;

    int r = ibv_req_notify_cq(state_->cq_.get(), 0);
    if (r) {
      throw std::system_error(std::make_error_code(std::errc{errno}));
    }

    state_->fd_ = std::make_unique<asio::posix::stream_descriptor>(
        executor_->get_asio_executor(), state_->channel_->fd);
    ELOG_INFO << "init listen event";
    auto listen_event = [](std::shared_ptr<shared_state_t> self)
        -> async_simple::coro::Lazy<void> {
      std::error_code ec;
      while (!ec) {
        coro_io::callback_awaitor<std::error_code> awaitor;

        ELOG_INFO << "posix::stream_fd::wait_read now";
        ELOG_INFO << "fd:" << self->fd_->native_handle();
        ec = co_await awaitor.await_resume([&self](auto handler) {
          self->fd_->async_wait(
              asio::posix::stream_descriptor::wait_read,
              [handler](const std::error_code& ec) mutable {
                ELOG_INFO << "asio awake from posix::stream_fd::wait_read";
                handler.set_value_then_resume(ec);
              });
        });
        ELOG_INFO << "awake from posix::stream_fd::wait_read";

        if (ec) {
          // if (std::errc{ec.value()}!=std::errc::operation_canceled) {
          std::error_code ec_ignore;
          self->close(ec_ignore);
          // }
          resume(std::pair{ec, std::size_t{0}}, self->recv_cb_);
          resume(std::pair{ec, std::size_t{0}}, self->send_cb_);
          break;
          // if (std::errc{ec.value()}!=std::errc::operation_canceled) {
          //   break;
          // }
        }
        ec = self->poll_completion();
      }
      if (ec) {
        ELOG_INFO << ec.message();
      }
    };
    ELOG_INFO << "start coroutine listener";
    listen_event(state_).start([](auto&& ec) {
    });
  }

  std::error_code post_recv(ib_buffer_view_t buffer, callback_t&& handler) {
    ibv_recv_wr rr{};
    ibv_sge sge{};
    ibv_recv_wr* bad_wr = nullptr;

    // prepare the scatter / gather entry
    sge.addr = (uintptr_t)buffer.address();
    sge.length = buffer.length();
    sge.lkey = buffer.lkey();

    rr.next = NULL;
    rr.wr_id = (std::size_t)&state_->recv_cb_;
    rr.sg_list = &sge;
    rr.num_sge = 1;

    // prepare the receive work request
    ELOG_INFO << "set cb:" << (void*)rr.wr_id;
    state_->recv_cb_ = std::move(handler);
    if (state_->has_close_) {
      return std::make_error_code(std::errc::operation_canceled);
    }

    // post the receive request to the RQ
    if (auto ec = ibv_post_recv(qp_.get(), &rr, &bad_wr); ec) {
      auto error_code = std::make_error_code(std::errc{ec});
      ELOG_INFO << "ibv post recv failed: " << error_code.message();
      return error_code;
    }
    ELOG_INFO << "ibv post recv ok";
    return {};
  }
  std::error_code post_send(ib_buffer_view_t buffer, callback_t&& handler) {
    ibv_send_wr sr{};
    ibv_sge sge{};
    ibv_send_wr* bad_wr = nullptr;

    // prepare the scatter / gather entry
    sge.addr = (uintptr_t)buffer.address();
    sge.length = buffer.length();
    sge.lkey = buffer.lkey();

    sr.next = NULL;
    sr.wr_id = (std::size_t)&state_->send_cb_;
    sr.sg_list = &sge;
    sr.num_sge = 1;
    sr.opcode = IBV_WR_SEND;
    sr.send_flags = IBV_SEND_SIGNALED;

    // prepare the receive work request
    ELOG_INFO << "set cb:" << (void*)sr.wr_id;
    state_->send_cb_ = std::move(handler);
    if (state_->has_close_) {
      return std::make_error_code(std::errc::operation_canceled);
    }

    // post the receive request to the RQ
    if (auto ec = ibv_post_send(qp_.get(), &sr, &bad_wr); ec) {
      auto error_code = std::make_error_code(std::errc{ec});
      ELOG_INFO << "ibv post send failed: " << error_code.message();
      return error_code;
    }
    ELOG_INFO << "ibv post send ok";
    return {};
  }

 public:
  ~ib_socket_t() {
    auto state = state_;
    if (state) {
      asio::dispatch(executor_->get_asio_executor(),
                     [state = std::move(state)]() {
                       std::error_code ec;
                       state->close(ec);
                     });
    }
  }
  enum io_type { read, write };

  template <io_type io>
  void async_io_impl(ib_buffer_view_t buffer, callback_t&& cb) {
    std::error_code ec;
    if constexpr (io == read) {
      ELOG_INFO << "POST RECV";
      ec = post_recv(buffer, std::move(cb));
    }
    else {
      ELOG_INFO << "POST SEND";
      ec = post_send(buffer, std::move(cb));
    }
    if (ec) [[unlikely]] {
      asio::dispatch(executor_->get_asio_executor(),
                     [state = state_, ec, cb = std::move(cb)]() mutable {
                       resume(std::pair{ec, std::size_t{0}}, cb);
                     });
    }
  }

  template <io_type io, bool has_next_buffer = false>
  async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_io(
      ib_buffer_view_t buffer, std::span<char> next_buffer = {},
      ib_buffer_t* new_buffer = nullptr) {
    coro_io::callback_awaitor<std::pair<std::error_code, std::size_t>> awaitor;
    if constexpr (io == read) {
      if (tcp_soc_) [[unlikely]] {
        ib_socket_info peer_info{};
        constexpr auto sz = struct_pack::get_needed_size(peer_info);
        char buffer[sz.size()];
        ELOG_INFO << "serialize peer_info";
        peer_info.qp_num = qp_->qp_num;
        peer_info.lid = device_->attr().lid;
        peer_info.length = conf_.buffer_size;
        memcpy(peer_info.gid, &device_->gid(), sizeof(peer_info.gid));
        struct_pack::serialize_to((char*)buffer, sz, peer_info);
        ELOG_INFO << "send qp info to client";
        auto tcp_soc = std::move(tcp_soc_);
        auto ec = co_await async_write(*tcp_soc, asio::buffer(buffer));
        if (ec.first) {
          ELOG_INFO << "write ib socket info to client";
          std::error_code ec2;
          state_->close(ec2);
          co_return std::pair{ec.first, std::size_t{0}};
        }
      }
    }
    auto result =
        co_await coro_io::async_io<std::pair<std::error_code, std::size_t>>(
            [this, &buffer](auto&& cb) {
              this->async_io_impl<io>(buffer, std::move(cb));
            },
            *this);
    if constexpr (has_next_buffer) {
      if (!result.first) [[likely]] {
        *new_buffer = ib_buffer_t::regist(device_, next_buffer.data(),
                                          next_buffer.size());
      }
    }
    co_return std::move(result);
  }

  template <io_type io>
  async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_io(
      ib_buffer_t& buffer, std::span<char> next_buffer) {
    coro_io::callback_awaitor<std::pair<std::error_code, std::size_t>> awaitor;
    ib_buffer_t new_buffer;
    auto result = co_await async_io<io, true>(buffer, next_buffer, &new_buffer);
    buffer = std::move(new_buffer);
    co_return result;
  }
  std::size_t get_remote_buffer_size() const noexcept {
    return remote_buffer_size_;
  }
  config_t& get_config() noexcept { return conf_; }
  const config_t& get_config() const noexcept { return conf_; }
  auto get_device() const noexcept { return device_; }
  async_simple::coro::Lazy<std::error_code> accept(
      std::unique_ptr<asio::ip::tcp::socket> soc) noexcept {
    ib_socket_t::ib_socket_info peer_info;
    constexpr auto sz = struct_pack::get_needed_size(peer_info);
    char buffer[sz.size()];
    auto [ec2, canceled] = co_await async_simple::coro::collectAll<
        async_simple::SignalType::Terminate>(
        async_read(*soc, asio::buffer(buffer)),
        coro_io::sleep_for(std::chrono::milliseconds{10000}));
    if (ec2.value().first) [[unlikely]] {
      co_return std::move(ec2.value().first);
    }
    else if (canceled.value()) [[unlikely]] {
      co_return std::make_error_code(std::errc::timed_out);
    }
    auto ec3 = struct_pack::deserialize_to(peer_info, std::span{buffer});
    if (ec3) [[unlikely]] {
      co_return std::make_error_code(std::errc::protocol_error);
    }
    ELOGV(INFO, "Remote QP number = %d", peer_info.qp_num);
    ELOGV(INFO, "Remote LID = %d", peer_info.lid);
    ELOGV(INFO, "Remote GID = %d", peer_info.gid);
    ELOGV(INFO, "Remote buffer size = %d", peer_info.length);
    remote_buffer_size_ = peer_info.length;
    try {
      init_qp();
      modify_qp_to_init();
      modify_qp_to_rtr(peer_info.qp_num, peer_info.lid,
                       (uint8_t*)peer_info.gid);
      modify_qp_to_rts();
      ELOG_INFO << "start init fd";
      init_fd();
    } catch (const std::system_error& err) {
      ELOG_INFO << "accept failed";
      co_return err.code();
    } catch (const std::exception& err) {
      ELOG_INFO << "accept failed";
      std::cout << err.what();
    } catch (...) {
      ELOG_INFO << "unknown exception";
    }
    tcp_soc_ = std::move(soc);
    co_return std::error_code{};
  }

  async_simple::coro::Lazy<std::error_code> connect(
      asio::ip::tcp::socket& soc) noexcept {
    try {
      ELOG_INFO << "client start connect";
      init_qp();
      modify_qp_to_init();
      ib_socket_t::ib_socket_info peer_info{};
      peer_info.qp_num = qp_->qp_num;
      peer_info.lid = device_->attr().lid;
      peer_info.length = conf_.buffer_size;
      memcpy(peer_info.gid, &device_->gid(), sizeof(peer_info.gid));
      constexpr auto sz = struct_pack::get_needed_size(peer_info);
      char buffer[sz.size()];
      struct_pack::serialize_to((char*)buffer, sz, peer_info);
      ELOG_INFO << "client start write buffer";
      auto [ec, len] = co_await async_write(soc, asio::buffer(buffer));
      if (ec) {
        co_return std::move(ec);
      }
      ELOG_INFO << "client start read buffer";
      std::tie(ec, len) = co_await async_read(soc, asio::buffer(buffer));
      if (ec) {
        ELOG_INFO << ec.message();
        co_return std::move(ec);
      }
      auto ec2 = struct_pack::deserialize_to(peer_info, std::span{buffer});
      if (ec2) [[unlikely]] {
        co_return std::make_error_code(std::errc::protocol_error);
      }
      ELOGV(INFO, "Remote QP number = %d", peer_info.qp_num);
      ELOGV(INFO, "Remote LID = %d", peer_info.lid);
      ELOGV(INFO, "Remote GID = %d", peer_info.gid);
      ELOGV(INFO, "Remote buffer size = %d", peer_info.length);
      remote_buffer_size_ = peer_info.length;
      modify_qp_to_rtr(peer_info.qp_num, peer_info.lid,
                       (uint8_t*)peer_info.gid);

      modify_qp_to_rts();
      ELOG_INFO << "start init fd";
      init_fd();
    } catch (std::system_error& err) {
      co_return err.code();
    } catch (const std::exception& err) {
      ELOG_INFO << "accept failed";
      std::cout << err.what();
    } catch (...) {
      ELOG_INFO << "unknown exception";
    }
    co_return std::error_code{};
  }
  auto get_executor() const { return executor_->get_asio_executor(); }
  auto get_coro_executor() const { return executor_; }
  auto cancel() const { state_->cancel(); }
};
}  // namespace coro_io
