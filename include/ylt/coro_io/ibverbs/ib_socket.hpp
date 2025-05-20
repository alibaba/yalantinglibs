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
#include <queue>
#include <stdexcept>
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
using callback_t = std::function<void(std::pair<std::error_code, std::size_t>)>;
inline void resume(std::pair<std::error_code, std::size_t>&& arg,
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
struct shared_state_t;
struct buffer_queue {
  std::vector<ib_buffer_t> queue;
  uint32_t front = 0, end = 0;
  buffer_queue(uint16_t size) {
    assert(size > 0);
    queue.resize(size);
  }
  std::error_code post_recv_real(ibv_sge buffer, shared_state_t* state);
  std::error_code push(ib_buffer_t buf, shared_state_t* state) {
    ELOG_TRACE << "push. now buffer size:" << size();
    auto ec = post_recv_real(buf.subview(), state);
    if (!ec) {
      front = (front + 1) % queue.size();
      queue[front] = std::move(buf);
    }
    return ec;
  }
  ib_buffer_t pop() {
    ELOG_TRACE << "pop. now buffer size:" << size();
    end = (end + 1) % queue.size();
    return std::move(queue[end]);
  }
  bool full() { return front == end; }
  std::size_t size() {
    if (end >= front) {
      return queue.size() + front - end;
    }
    else {
      return front - end;
    }
  }
};

struct shared_state_t {
  std::shared_ptr<ib_device_t> device_;
  std::shared_ptr<ib_buffer_pool_t> ib_buffer_pool_;
  std::unique_ptr<ibv_comp_channel, ib_deleter> channel_;
  std::unique_ptr<ibv_cq, ib_deleter> cq_;
  std::unique_ptr<ibv_qp, ib_deleter> qp_;
  std::unique_ptr<asio::posix::stream_descriptor> fd_;
  std::atomic<bool> has_close_ = 0;
  buffer_queue recv_queue_;
  std::size_t recv_buffer_cnt_;
  std::queue<std::pair<std::error_code, std::size_t>>
      recv_result;  // TODO optimize
  callback_t recv_cb_;
  callback_t send_cb_;
  ib_buffer_t recv_buf_;
  ib_buffer_t send_buf_;
  shared_state_t(std::shared_ptr<ib_buffer_pool_t> ib_buffer_pool,
                 std::size_t recv_buffer_cnt = 2,
                 std::size_t max_recv_buffer_cnt = 8)
      : ib_buffer_pool_(std::move(ib_buffer_pool)),
        recv_buffer_cnt_(recv_buffer_cnt),
        recv_queue_(max_recv_buffer_cnt) {
    if (ib_buffer_pool_ == nullptr) {
      ib_buffer_pool_ = coro_io::g_ib_buffer_pool();
    }
  }

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
      }
      std::move(recv_buf_).collect();
      std::move(send_buf_).collect();
      if (fd_) {
        fd_->cancel(ec);
        if (ec)
          return;
        fd_->close(ec);
      }
      channel_ = nullptr;
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
    struct ibv_wc wc {};
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
        ELOG_DEBUG << "ready resume: id:" << (callback_t*)wc.wr_id
                   << ",len:" << wc.byte_len;
        if (wc.wr_id == 0) {  // recv
          if (recv_cb_) {
            assert(recv_result.empty());
            bool could_insert = (recv_queue_.size() <= recv_buffer_cnt_);
            recv_buf_ = recv_queue_.pop();  // TODO:enum
            if (could_insert) {
              if (recv_queue_.push(ib_buffer_pool_->get_buffer(), this)) {
                std::error_code ec;
                close(ec);
              }
            }
            resume(std::pair{ec, (std::size_t)wc.byte_len}, recv_cb_);
          }
          else {
            if (!recv_queue_.full()) {
              if (recv_queue_.push(ib_buffer_pool_->get_buffer(), this)) {
                std::error_code ec;
                close(ec);
              }
            }
            recv_result.push(std::pair{ec, (std::size_t)wc.byte_len});
          }
        }
        else {
          resume(std::pair{ec, (std::size_t)wc.byte_len},
                 *(callback_t*)wc.wr_id);
        }
        ELOG_DEBUG << "after resume:" << (callback_t*)wc.wr_id;
        if (cq_ == nullptr) {
          break;
        }
      }
    }
    return ec;
  }
};

inline std::error_code buffer_queue::post_recv_real(ibv_sge buffer,
                                                    shared_state_t* state) {
  ibv_recv_wr rr;
  rr.next = NULL;

  rr.wr_id = 0;
  rr.sg_list = &buffer;
  rr.num_sge = 1;
  ELOG_TRACE << "post recv sge address:" << buffer.addr
             << ",length:" << buffer.length;
  ibv_recv_wr* bad_wr = nullptr;
  // prepare the receive work request
  if (state->has_close_) {
    return std::make_error_code(std::errc::operation_canceled);
  }

  // post the receive request to the RQ
  if (auto ec = ibv_post_recv(state->qp_.get(), &rr, &bad_wr); ec) {
    auto error_code = std::make_error_code(std::errc{ec});
    ELOG_ERROR << "ibv post recv failed: " << error_code.message();
    return error_code;
  }
  ELOG_DEBUG << "ibv post recv ok";
  return {};
}

#ifdef YLT_ENABLE_IBV
struct ibverbs_config {
  uint32_t cq_size = 1024;
  uint32_t request_buffer_size = 8 * 1024 * 1024;
  uint32_t recv_buffer_cnt = 2;
  std::chrono::milliseconds tcp_handshake_timeout =
      std::chrono::milliseconds{1000};
  ibv_qp_type qp_type = IBV_QPT_RC;
  ibv_qp_cap cap = {.max_send_wr = 8,
                    .max_recv_wr = 8,
                    .max_send_sge = 6,
                    .max_recv_sge = 1,
                    .max_inline_data = 0};
  std::shared_ptr<coro_io::ib_device_t> device;
  std::shared_ptr<coro_io::ib_buffer_pool_t> buffer_pool;
};
#endif

class ib_socket_t {
 public:
  enum io_type { recv = 0, send = 1 };
  struct ib_socket_info {
    uint8_t gid[16];       // GID
    uint16_t lid;          // LID of the IB port
    uint32_t buffer_size;  // buffer length
    uint32_t qp_num;       // QP number
  };

  ib_socket_t(
      coro_io::ExecutorWrapper<>* executor, std::shared_ptr<ib_device_t> device,
      const ibverbs_config& config,
      std::shared_ptr<ib_buffer_pool_t> buffer_pool = g_ib_buffer_pool())
      : executor_(executor),
        state_(std::make_shared<shared_state_t>(std::move(buffer_pool),
                                                config.recv_buffer_cnt,
                                                config.cap.max_recv_wr)) {
    if (device == nullptr) {
      device = coro_io::g_ib_device();
    }
    state_->device_ = std::move(device);
    init(config);
  }
  ib_socket_t(
      coro_io::ExecutorWrapper<>* executor = coro_io::get_global_executor(),
      std::shared_ptr<ib_device_t> device = coro_io::g_ib_device(),
      std::shared_ptr<ib_buffer_pool_t> buffer_pool = g_ib_buffer_pool())
      : executor_(executor),
        state_(std::make_shared<shared_state_t>(std::move(buffer_pool))) {
    if (device == nullptr) {
      device = coro_io::g_ib_device();
    }
    state_->device_ = std::move(device);
    ibverbs_config config{};
    init(config);
  }

  ib_socket_t(ib_socket_t&&) = default;
  ib_socket_t& operator=(ib_socket_t&&) = default;
  ~ib_socket_t() { close(); }

  bool is_open() const noexcept {
    return state_->fd_ != nullptr && state_->fd_->is_open();
  }
  std::shared_ptr<ib_buffer_pool_t> buffer_pool() const noexcept {
    return state_->ib_buffer_pool_;
  }

  std::size_t consume(char* dst, std::size_t sz) {
    auto len = std::min(sz, remain_data_.size());
    if (len) {
      memcpy(dst, remain_data_.data(), len);
      remain_data_ = remain_data_.substr(len);
      if (remain_data_.empty()) {
        std::move(state_->recv_buf_).collect();
      }
    }
    ELOG_TRACE << "consume dst:" << dst << "want sz:" << sz << "get sz:" << len;
    return len;
  }

  std::size_t remain_read_buffer_size() { return remain_data_.size(); }
  void set_read_buffer_len(std::size_t has_read_size, std::size_t remain_size) {
    remain_data_ = std::string_view{
        (char*)state_->recv_buf_->addr + has_read_size, remain_size};
    if (remain_size == 0) {
      std::move(state_->recv_buf_).collect();
    }
  }

  ibv_sge get_recv_buffer() {
    assert(remain_read_buffer_size() == 0);
    ELOG_TRACE << "get recv buffer from client";
    return state_->recv_buf_.subview();
  }

  ibv_sge get_send_buffer() {
    ELOG_TRACE << "get send buffer from client";
    if (!state_->send_buf_) {
      ELOG_TRACE << "no buffer now. try get new buffer from pool";
      state_->send_buf_ = buffer_pool()->get_buffer();
    }
    return state_->send_buf_.subview();
  }

  void post_recv(callback_t&& cb) {
    ELOG_DEBUG << "POST RECV";
    post_recv_impl(std::move(cb));
  }

  void post_send(std::span<ibv_sge> buffer, callback_t&& cb) {
    ELOG_DEBUG << "POST SEND";
    if (auto ec = post_send_impl(buffer, std::move(cb)); ec) [[unlikely]] {
      safe_resume(ec, std::move(state_->send_cb_));
    }
  }

  uint32_t get_buffer_size() const noexcept { return buffer_size_; }

  ibverbs_config& get_config() noexcept { return conf_; }
  const ibverbs_config& get_config() const noexcept { return conf_; }
  auto get_device() const noexcept { return state_->device_; }

  async_simple::coro::Lazy<std::error_code> accept() noexcept {
    if (soc_ == nullptr) {
      co_return std::make_error_code(std::errc::invalid_argument);
    }
    ib_socket_t::ib_socket_info peer_info;
    constexpr auto sz = struct_pack::get_needed_size(peer_info);
    char buffer[sz.size()];
    auto [ec2, canceled] = co_await async_simple::coro::collectAll<
        async_simple::SignalType::Terminate>(
        async_read(*soc_, asio::buffer(buffer)),
        coro_io::sleep_for(std::chrono::milliseconds{100000}, executor_));
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
    ELOGV(INFO, "Remote Request buffer size = %d", peer_info.buffer_size);
    buffer_size_ = std::min<uint32_t>(peer_info.buffer_size,
                                      get_config().request_buffer_size);
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

    for (int i = 0; i < conf_.recv_buffer_cnt; ++i) {
      if (auto ec = state_->recv_queue_.push(
              state_->ib_buffer_pool_->get_buffer(), state_.get());
          ec) {
        co_return ec;
      }
      if (state_->recv_queue_.full()) {
        break;
      }
    }

    ELOG_INFO << "serialize peer_info";
    peer_info.qp_num = state_->qp_->qp_num;
    peer_info.lid = state_->device_->attr().lid;
    peer_info.buffer_size = conf_.request_buffer_size;
    memcpy(peer_info.gid, &state_->device_->gid(), sizeof(peer_info.gid));
    struct_pack::serialize_to((char*)buffer, sz, peer_info);
    ELOG_INFO << "send qp info to client";
    auto& soc_ref = *soc_;
    async_write(soc_ref, asio::buffer(buffer))
        .start([soc = std::move(soc_), state = state_](auto&& ec) {
          try {
            if (!ec.value().first) {
              return;
            }
            ELOG_WARN << "rmda connection failed by exception:"
                      << ec.value().first.message();
          } catch (const std::exception& e) {
            ELOG_WARN << "rmda connection failed by exception:" << e.what();
          }
          std::error_code ec_ignore;
          state->close(ec_ignore);
        });
    soc_ = nullptr;
    co_return std::error_code{};
  }
  void prepare_accpet(std::unique_ptr<asio::ip::tcp::socket> soc) noexcept {
    soc_ = std::move(soc);
  }

  async_simple::coro::Lazy<std::error_code> accept(
      std::unique_ptr<asio::ip::tcp::socket> soc) noexcept {
    soc_ = std::move(soc);
    return accept();
  }

  async_simple::coro::Lazy<std::error_code> connect(
      asio::ip::tcp::socket& soc) noexcept {
    try {
      ELOG_INFO << "client start connect";
      init_qp();
      modify_qp_to_init();
      ib_socket_t::ib_socket_info peer_info{};
      peer_info.qp_num = state_->qp_->qp_num;
      peer_info.lid = state_->device_->attr().lid;
      peer_info.buffer_size = conf_.request_buffer_size;
      memcpy(peer_info.gid, &state_->device_->gid(), sizeof(peer_info.gid));
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
      else if (state_->has_close_) {
        ELOG_INFO << "connect closed when connecting";
        co_return std::make_error_code(std::errc::operation_canceled);
      }
      auto ec2 = struct_pack::deserialize_to(peer_info, std::span{buffer});
      if (ec2) [[unlikely]] {
        co_return std::make_error_code(std::errc::protocol_error);
      }
      ELOGV(INFO, "Remote QP number = %d", peer_info.qp_num);
      ELOGV(INFO, "Remote LID = %d", peer_info.lid);
      ELOGV(INFO, "Remote GID = %d", peer_info.gid);
      ELOGV(INFO, "Remote buffer size = %d", peer_info.buffer_size);
      buffer_size_ = std::min<uint32_t>(peer_info.buffer_size,
                                        get_config().request_buffer_size);
      ELOGV(INFO, "Final buffer size = %d", buffer_size_);
      modify_qp_to_rtr(peer_info.qp_num, peer_info.lid,
                       (uint8_t*)peer_info.gid);

      for (int i = 0; i < conf_.recv_buffer_cnt; ++i) {
        if (auto ec = state_->recv_queue_.push(
                state_->ib_buffer_pool_->get_buffer(), state_.get());
            ec) {
          co_return ec;
        }
        if (state_->recv_queue_.full()) {
          break;
        }
      }
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

  void close() {
    auto state = state_;
    if (state) {
      asio::dispatch(executor_->get_asio_executor(),
                     [state = std::move(state)]() {
                       std::error_code ec;
                       state->close(ec);
                     });
    }
  }
  auto get_executor() const { return executor_->get_asio_executor(); }
  auto get_coro_executor() const { return executor_; }
  auto cancel() const { state_->cancel(); }

 private:
  void init(const ibverbs_config& config) {
    conf_ = config;
    state_->channel_.reset(ibv_create_comp_channel(state_->device_->context()));
    if (!state_->channel_) [[unlikely]] {
      throw std::system_error(std::make_error_code(std::errc{errno}));
    }
    state_->cq_.reset(ibv_create_cq(state_->device_->context(), config.cq_size,
                                    NULL, state_->channel_.get(), 0));
    if (!state_->cq_) [[unlikely]] {
      throw std::system_error(std::make_error_code(std::errc{errno}));
    }
  }

  void init_qp() {
    struct ibv_qp_init_attr qp_init_attr;
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.qp_type = conf_.qp_type;
    qp_init_attr.sq_sig_all = 1;
    qp_init_attr.send_cq = state_->cq_.get();
    qp_init_attr.recv_cq = state_->cq_.get();
    qp_init_attr.cap = conf_.cap;
    auto qp = ibv_create_qp(state_->device_->pd(), &qp_init_attr);
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
    attr.port_num = state_->device_->port();
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
    attr.min_rnr_timer = 1;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = dlid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = state_->device_->port();
    if (state_->device_->gid_index() >= 0) {
      attr.ah_attr.is_global = 1;
      attr.ah_attr.port_num = 1;
      memcpy(&attr.ah_attr.grh.dgid, dgid, 16);
      attr.ah_attr.grh.flow_label = 0;
      attr.ah_attr.grh.hop_limit = 1;
      attr.ah_attr.grh.sgid_index = state_->device_->gid_index();
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
    attr.retry_cnt = 5;
    attr.rnr_retry = 5;
    attr.sq_psn = 0;
    attr.max_rd_atomic = 1;
    int flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    if (auto ec = ibv_modify_qp(state_->qp_.get(), &attr, flags); ec != 0) {
      throw std::system_error(std::make_error_code(std::errc{ec}));
    }
    ELOGV(INFO, "Modify QP to RTS done!");
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

  void post_recv_impl(callback_t&& handler) {
    coro_io::dispatch(
        [state = state_, handler = std::move(handler)]() {
          state->recv_cb_ = std::move(handler);
          if (!state->recv_result.empty()) {
            auto result = state->recv_result.front();
            bool could_insert =
                (state->recv_queue_.size() <= state->recv_buffer_cnt_);
            state->recv_result.pop();
            state->recv_buf_ = std::move(state->recv_queue_.pop());
            if (could_insert) {
              if (state->recv_queue_.push(state->ib_buffer_pool_->get_buffer(),
                                          state.get())) {
                std::error_code ec;
                state->close(ec);
              }
            }
            resume(std::move(result), state->recv_cb_);
          }
        },
        executor_->get_asio_executor())
        .start([](auto&&) {
        });
  }

  std::error_code post_send_impl(std::span<ibv_sge> sge, callback_t&& handler) {
    ibv_send_wr sr{};
    ibv_send_wr* bad_wr = nullptr;

    sr.next = NULL;
    sr.wr_id = (std::size_t)&state_->send_cb_;
    sr.sg_list = sge.data();
    sr.num_sge = sge.size();
    ELOG_TRACE << "post send sge size:" << sge.size();
    for (int i = 0; i < sge.size(); ++i) {
      ELOG_TRACE << "post send sge[" << std::to_string(i)
                 << "].address:" << sge.data()[i].addr
                 << ",length:" << sge.data()[i].length;
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

  void safe_resume(std::error_code ec, callback_t&& cb) {
    asio::dispatch(executor_->get_asio_executor(),
                   [ec, cb = std::move(cb)]() mutable {
                     resume(std::pair{ec, std::size_t{0}}, cb);
                   });
  }

  coro_io::ExecutorWrapper<>* executor_;
  std::string_view remain_data_;
  std::unique_ptr<asio::ip::tcp::socket> soc_;
  std::shared_ptr<shared_state_t> state_;
  ibverbs_config conf_;
  uint32_t buffer_size_ = 0;
};
}  // namespace coro_io
