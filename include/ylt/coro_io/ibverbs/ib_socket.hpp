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
    bool enable_read_buffer_when_zero_copy = false;
    uint32_t cq_size = 1024;
    std::size_t request_buffer_size = 8 * 1024 * 1024;
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
  enum io_type { recv, send };
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
    std::unique_ptr<ibv_qp, ib_deleter> qp_;
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
        if (qp_) {
          ibv_qp_attr attr;
          memset(&attr, 0, sizeof(attr));
          attr.qp_state = IBV_QPS_RESET;

          int ret = ibv_modify_qp(qp_.get(), &attr, IBV_QP_STATE);
          if (ret) {
            ELOG_ERROR << "ibv_modify_qp IBV_QPS_RESET failed, "
                       << std::make_error_code(std::errc{ret}).message();
          }
          qp_ = nullptr;
          cq_ = nullptr;
          channel_ = nullptr;
        }

        if (fd_) {
          fd_->cancel(ec);
          if (ec)
            return;
          fd_->close(ec);
        }
      }
    }
    std::error_code poll_completion() {
      ELOG_DEBUG << "IO EVENT COMMING";
      void* ev_ctx;
      auto cq = cq_.get();
      ELOG_DEBUG << "get_cq_event";
      int r = ibv_get_cq_event(channel_.get(), &cq, &ev_ctx);
      // assert(r >= 0);
      std::error_code ec;
      if (r) {
        ELOG_ERROR << std::make_error_code(std::errc{r}).message();
        return std::make_error_code(std::errc{r});
      }
      ELOG_DEBUG << "ack_cq_events";
      ibv_ack_cq_events(cq, 1);
      r = ibv_req_notify_cq(cq, 0);
      if (r) {
        ELOG_ERROR << std::make_error_code(std::errc{r}).message();
        return std::make_error_code(std::errc{r});
      }
      ELOG_DEBUG << "ibv_poll_cq";
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
          ELOGV(ERROR, "rdma failed with error code:%d", wc.status);
        }
        else {
          ELOG_DEBUG << "ready resume:" << (callback_t*)wc.wr_id;
          resume(std::pair{ec, (std::size_t)wc.byte_len},
                 *(callback_t*)wc.wr_id);
          ELOG_DEBUG << "after resume:" << (callback_t*)wc.wr_id;
          if (cq_ == nullptr) {
            break;
          }
        }
      }
      return ec;
    }
  };

  std::shared_ptr<ib_device_t> device_;
  ib_buffer_pool_t* ib_buffer_pool_;
  ib_buffer_t buffer_;
  std::string_view least_data_;

  std::shared_ptr<shared_state_t> state_;
  config_t conf_;
  uint32_t buffer_size_ = 0;
  coro_io::ExecutorWrapper<>* executor_;

 public:
  std::size_t consume(char* dst, std::size_t sz) {
    if (least_data_.size()) {
      auto len = std::min(sz, least_data_.size());
      memcpy(dst, least_data_.data(), len);
      least_data_ = least_data_.substr(len);
      if (least_data_.empty()) {
        ib_buffer_pool_->collect_free(std::move(buffer_));
      }
      return len;
    }
    else {
      return 0;
    }
  }
  std::size_t least_buffer_size() { return least_data_.size(); }
  void set_read_buffer(ib_buffer_t&& buffer, std::string_view data) {
    ib_buffer_pool_->collect_free(std::move(buffer_));
    buffer_ = std::move(buffer);
    least_data_ = data;
  }

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
    state_->qp_.reset(qp);
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
    if (auto ec = ibv_modify_qp(state_->qp_.get(), &attr, flags); ec) {
      throw std::system_error(std::make_error_code(std::errc{ec}));
    }
    ELOGV(INFO, "Modify QP to INIT done!");
  }
  void modify_qp_to_rtr(uint32_t remote_qpn, uint16_t dlid, uint8_t* dgid) {
    ibv_qp_attr attr{};
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_4096;
    attr.dest_qp_num = remote_qpn;
    attr.rq_psn = 0;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 31;
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
    if (auto ec = ibv_modify_qp(state_->qp_.get(), &attr, flags); ec) {
      throw std::system_error(std::make_error_code(std::errc{ec}));
    }
    ELOGV(INFO, "Modify QP to RTR done!");
  }
  void modify_qp_to_rts() {
    ibv_qp_attr attr{};
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 0x12;  // 18
    attr.retry_cnt = 6;
    attr.rnr_retry = 6;
    attr.sq_psn = 0;
    attr.max_rd_atomic = 1;
    int flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    if (auto ec = ibv_modify_qp(state_->qp_.get(), &attr, flags); ec != 0) {
      throw std::system_error(std::make_error_code(std::errc{ec}));
    }
    ELOGV(INFO, "Modify QP to RTS done!");
  }
  inline static void resume(std::pair<std::error_code, std::size_t>&& arg,
                            callback_t& handle) {
    ELOG_DEBUG << "resume callback:" << &handle;
    if (handle) [[likely]] {
      ELOG_DEBUG << "resuming:" << &handle;
      auto handle_tmp = std::move(handle);
      handle_tmp(std::move(arg));
      ELOG_DEBUG << "resume over,clear cb:" << &handle;
    }
    else {
      ELOG_DEBUG << "resume empty";
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

        ELOG_DEBUG << "posix::stream_fd::wait_read now";
        ELOG_DEBUG << "fd:" << self->fd_->native_handle();
        ec = co_await awaitor.await_resume([&self](auto handler) {
          self->fd_->async_wait(
              asio::posix::stream_descriptor::wait_read,
              [handler](const std::error_code& ec) mutable {
                ELOG_DEBUG << "asio awake from posix::stream_fd::wait_read";
                handler.set_value_then_resume(ec);
              });
        });
        ELOG_DEBUG << "awake from posix::stream_fd::wait_read";

        if (!ec) {
          ec = self->poll_completion();
        }

        if (ec) {
          std::error_code ec_ignore;
          self->close(ec_ignore);
          resume(std::pair{ec, std::size_t{0}}, self->recv_cb_);
          resume(std::pair{ec, std::size_t{0}}, self->send_cb_);
          break;
        }
      }
      if (ec) {
        ELOG_INFO << ec.message();
      }
    };
    ELOG_INFO << "start coroutine listener";
    listen_event(state_).start([](auto&& ec) {
    });
  }
  template <typename T>
  std::error_code post_recv(T& sge, callback_t&& handler) {
    ibv_recv_wr rr;
    rr.next = NULL;

    rr.wr_id = (std::size_t)&state_->recv_cb_;
    if constexpr (std::is_same_v<T, ibv_sge>) {
      rr.sg_list = &sge;
      rr.num_sge = 1;
      ELOG_TRACE << "post recv sge address:" << sge.addr << ",length:" << sge.length;
    }
    else {
      rr.sg_list = sge.data();
      rr.num_sge = sge.size();
      for (int i=0;i<sge.size();++i) {
        ELOG_TRACE << "post recv sge["<<std::to_string(i)<<"].address:" << sge.data()[i].addr << ",length:" << sge.data()[i].length;
      }
    }
    ibv_recv_wr* bad_wr = nullptr;
    // prepare the receive work request
    ELOG_DEBUG << "set cb:" << (void*)rr.wr_id;
    state_->recv_cb_ = std::move(handler);
    if (state_->has_close_) {
      return std::make_error_code(std::errc::operation_canceled);
    }

    // post the receive request to the RQ
    if (auto ec = ibv_post_recv(state_->qp_.get(), &rr, &bad_wr); ec) {
      auto error_code = std::make_error_code(std::errc{ec});
      ELOG_ERROR << "ibv post recv failed: " << error_code.message();
      return error_code;
    }
    ELOG_DEBUG << "ibv post recv ok";
    return {};
  }
  template <typename T>
  std::error_code post_send(T& sge, callback_t&& handler) {
    ibv_send_wr sr{};
    ibv_send_wr* bad_wr = nullptr;

    sr.next = NULL;
    sr.wr_id = (std::size_t)&state_->send_cb_;
    if constexpr (std::is_same_v<T, ibv_sge>) {
      sr.sg_list = &sge;
      sr.num_sge = 1;
      ELOG_TRACE << "post send sge address:" << sge.addr << ",length:" << sge.length;
    }
    else {
      sr.sg_list = sge.data();
      sr.num_sge = sge.size();
      for (int i=0;i<sge.size();++i) {
        ELOG_TRACE << "post send sge["<<std::to_string(i)<<"].address:" << sge.data()[i].addr << ",length:" << sge.data()[i].length;
      }
    }
    sr.opcode = IBV_WR_SEND;
    sr.send_flags = IBV_SEND_SIGNALED;
    // prepare the receive work request
    ELOG_DEBUG << "set cb:" << (void*)sr.wr_id;
    state_->send_cb_ = std::move(handler);
    if (state_->has_close_) {
      return std::make_error_code(std::errc::operation_canceled);
    }
    // post the receive request to the RQ
    if (auto ec = ibv_post_send(state_->qp_.get(), &sr, &bad_wr); ec) {
      auto error_code = std::make_error_code(std::errc{ec});
      ELOG_ERROR << "ibv post send failed: " << error_code.message();
      return error_code;
    }
    ELOG_DEBUG << "ibv post send ok";
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

  template <io_type io, typename T>
  void async_io(T&& buffer, callback_t&& cb) {
    std::error_code ec;
    if constexpr (io == recv) {
      ELOG_DEBUG << "POST RECV";
      ec = post_recv(buffer, std::move(cb));
    }
    else {
      ELOG_DEBUG << "POST SEND";
      ec = post_send(buffer, std::move(cb));
    }
    if (ec) [[unlikely]] {
      asio::dispatch(executor_->get_asio_executor(),
                     [state = state_, ec, cb = std::move(cb)]() mutable {
                       resume(std::pair{ec, std::size_t{0}}, cb);
                     });
    }
  }
  uint32_t get_buffer_size() const noexcept { return buffer_size_; }
  config_t& get_config() noexcept { return conf_; }
  const config_t& get_config() const noexcept { return conf_; }
  auto get_device() const noexcept { return device_; }
  async_simple::coro::Lazy<std::error_code> accept(
      asio::ip::tcp::socket& soc) noexcept {
    ib_socket_t::ib_socket_info peer_info;
    constexpr auto sz = struct_pack::get_needed_size(peer_info);
    char buffer[sz.size()];
    auto [ec2, canceled] = co_await async_simple::coro::collectAll<
        async_simple::SignalType::Terminate>(
        async_read(soc, asio::buffer(buffer)),
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
    ELOGV(INFO, "Remote Request buffer size = %d", peer_info.length);
    buffer_size_ =
        std::min<uint32_t>(peer_info.length, get_config().request_buffer_size);
    ELOGV(INFO, "Final buffer size = %d", buffer_size_);
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
    ELOG_INFO << "serialize peer_info";
    peer_info.qp_num = state_->qp_->qp_num;
    peer_info.lid = device_->attr().lid;
    peer_info.length = conf_.request_buffer_size;
    memcpy(peer_info.gid, &device_->gid(), sizeof(peer_info.gid));
    struct_pack::serialize_to((char*)buffer, sz, peer_info);
    ELOG_INFO << "send qp info to client";
    auto ec = co_await async_write(soc, asio::buffer(buffer));
    if (ec.first) {
      ELOG_INFO << "write ib socket info to client";
      std::error_code ec2;
      state_->close(ec2);
      co_return ec.first;
    }
    co_return std::error_code{};
  }

  async_simple::coro::Lazy<std::error_code> connect(
      asio::ip::tcp::socket& soc) noexcept {
    try {
      ELOG_INFO << "client start connect";
      init_qp();
      modify_qp_to_init();
      ib_socket_t::ib_socket_info peer_info{};
      peer_info.qp_num = state_->qp_->qp_num;
      peer_info.lid = device_->attr().lid;
      peer_info.length = conf_.request_buffer_size;
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
      buffer_size_ = std::min<uint32_t>(peer_info.length,
                                        get_config().request_buffer_size);
      ELOGV(INFO, "Final buffer size = %d", buffer_size_);
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
