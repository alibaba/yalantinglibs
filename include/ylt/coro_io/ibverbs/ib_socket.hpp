#pragma once
#include <infiniband/verbs.h>
#include <sys/stat.h>

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
#include "asio/ip/address.hpp"
#include "asio/ip/address_v4.hpp"
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

struct ib_socket_shared_state_t;
struct ib_buffer_queue {
  std::vector<ib_buffer_t> queue;
  uint32_t front = 0, end = 0;
  bool may_empty = true;
  ib_buffer_queue(uint16_t size) {
    assert(size > 0);
    queue.resize(size);
  }
  std::error_code post_recv_real(ibv_sge buffer,
                                 ib_socket_shared_state_t* state);
  std::error_code push(ib_buffer_t buf, ib_socket_shared_state_t* state) {
    if (!buf || buf->length == 0) {
      ELOG_INFO << "Out of ib_buffer_pool limit." << size();
      return {};
    }
    auto ec = post_recv_real(buf.subview(), state);
    if (!ec) {
      may_empty = false;
      front = (front + 1) % queue.size();
      queue[front] = std::move(buf);
    }
    return ec;
  }
  ib_buffer_t pop() {
    end = (end + 1) % queue.size();
    may_empty = true;
    return std::move(queue[end]);
  }
  bool full() const noexcept { return front == end && !may_empty; }
  bool empty() const noexcept { return front == end && may_empty; }
  std::size_t size() {
    if (end >= front) {
      return queue.size() + front - end;
    }
    else {
      return front - end;
    }
  }
};

struct ib_socket_shared_state_t {
  using callback_t =
      std::function<void(std::pair<std::error_code, std::size_t>)>;
  static void resume(std::pair<std::error_code, std::size_t>&& arg,
                     callback_t& handle) {
    if (handle) [[likely]] {
      auto handle_tmp = std::move(handle);
      handle_tmp(std::move(arg));
    }
  }
  std::shared_ptr<ib_device_t> device_;
  std::shared_ptr<ib_buffer_pool_t> ib_buffer_pool_;
  std::unique_ptr<ibv_comp_channel, ib_deleter> channel_;
  std::unique_ptr<ibv_cq, ib_deleter> cq_;
  std::unique_ptr<ibv_qp, ib_deleter> qp_;
  std::unique_ptr<asio::posix::stream_descriptor> fd_;
  std::atomic<bool> has_close_ = 0;
  ib_buffer_queue recv_queue_;
  std::size_t recv_buffer_cnt_;
  std::queue<std::pair<std::error_code, std::size_t>>
      recv_result;  // TODO optimize with circle buffer
  callback_t recv_cb_;
  callback_t send_cb_;
  ib_buffer_t recv_buf_;
  ib_buffer_t send_buf_;
  ib_socket_shared_state_t(std::shared_ptr<ib_buffer_pool_t> ib_buffer_pool,
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
    void* ev_ctx;
    auto cq = cq_.get();
    int r = ibv_get_cq_event(channel_.get(), &cq, &ev_ctx);
    // assert(r >= 0);
    std::error_code ec;
    if (r) {
      ELOG_ERROR << std::make_error_code(std::errc{r}).message();
      return std::make_error_code(std::errc{r});
    }
    ibv_ack_cq_events(cq, 1);
    r = ibv_req_notify_cq(cq, 0);
    if (r) {
      ELOG_ERROR << std::make_error_code(std::errc{r}).message();
      return std::make_error_code(std::errc{r});
    }
    struct ibv_wc wc{};
    int ne = 0;
    while ((ne = ibv_poll_cq(cq_.get(), 1, &wc)) != 0) {
      if (ne < 0) {
        ELOG_ERROR << "poll CQ failed:" << ne;
        return std::make_error_code(std::errc::io_error);
      }
      if (ne > 0) {
        ELOG_TRACE << "Completion was found in CQ with status:" << wc.status;
      }
      ec = make_error_code(wc.status);
      if (wc.status != IBV_WC_SUCCESS) {
        ELOG_WARN << "rdma failed with error code: " << wc.status;
      }
      else {
        ELOG_TRACE << "rdma op success, id:" << (callback_t*)wc.wr_id
                   << ",len:" << wc.byte_len;
        if (wc.wr_id == 0) {  // recv
          if (recv_cb_) {
            assert(recv_result.empty());
            recv_buf_ = recv_queue_.pop();
            if (recv_queue_.empty()) {
              if (recv_queue_.push(ib_buffer_pool_->get_buffer(), this)) {
                std::error_code ec;
                close(ec);
              }
            }
            resume(std::pair{ec, (std::size_t)wc.byte_len}, recv_cb_);
          }
          else {
            recv_result.push(std::pair{ec, (std::size_t)wc.byte_len});
            if (!recv_queue_.full() && recv_result.size()==recv_queue_.size()) {
              if (recv_queue_.push(ib_buffer_pool_->get_buffer(), this)) {
                std::error_code ec;
                close(ec);
              }
            }
          }
        }
        else {
          resume(std::pair{ec, (std::size_t)wc.byte_len},
                 *(callback_t*)wc.wr_id);
        }
        if (cq_ == nullptr) {
          break;
        }
      }
    }
    return ec;
  }
};

inline std::error_code ib_buffer_queue::post_recv_real(
    ibv_sge buffer, ib_socket_shared_state_t* state) {
  ibv_recv_wr rr;
  rr.next = NULL;

  rr.wr_id = 0;
  rr.sg_list = &buffer;
  rr.num_sge = 1;
  ELOG_TRACE << "post recv sge address:" << (void*)buffer.addr
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
  return {};
}

#ifdef YLT_ENABLE_IBV
struct ibverbs_config {
  uint32_t cq_size = 1024;
  uint32_t request_buffer_size = 4 * 1024 * 1024;
  uint32_t recv_buffer_cnt = 2;
  ibv_qp_type qp_type = IBV_QPT_RC;
  ibv_qp_cap cap = {.max_send_wr = 32,
                    .max_recv_wr = 32,
                    .max_send_sge = 1,
                    .max_recv_sge = 1,
                    .max_inline_data = 0};
  std::shared_ptr<coro_io::ib_device_t> device;
  std::shared_ptr<coro_io::ib_buffer_pool_t> buffer_pool;
};
#endif

class ib_socket_t {
 public:
  enum io_type { recv = 0, send = 1 };
  using callback_t = ib_socket_shared_state_t::callback_t;
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
        state_(std::make_shared<ib_socket_shared_state_t>(
            std::move(buffer_pool), config.recv_buffer_cnt,
            config.cap.max_recv_wr)) {
    if (device == nullptr) {
      device = coro_io::g_ib_device();
    }
    state_->device_ = std::move(device);
    init(config);
  }

 private:
  static inline const ibverbs_config default_config;

 public:
  ib_socket_t(
      coro_io::ExecutorWrapper<>* executor = coro_io::get_global_executor(),
      std::shared_ptr<ib_device_t> device = coro_io::g_ib_device(),
      std::shared_ptr<ib_buffer_pool_t> buffer_pool = g_ib_buffer_pool())
      : executor_(executor),
        state_(std::make_shared<ib_socket_shared_state_t>(
            std::move(buffer_pool), default_config.recv_buffer_cnt,
            default_config.cap.max_recv_wr)) {
    if (device == nullptr) {
      device = coro_io::g_ib_device();
    }
    state_->device_ = std::move(device);
    init(default_config);
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
    return state_->recv_buf_.subview();
  }

  ibv_sge get_send_buffer() {
    if (!state_->send_buf_) {
      state_->send_buf_ = buffer_pool()->get_buffer();
    }
    if (!state_->send_buf_) [[unlikely]] {
      return {};
    }
    else {
      return state_->send_buf_.subview();
    }
  }

  void post_recv(callback_t&& cb) { post_recv_impl(std::move(cb)); }

  void post_send(std::span<ibv_sge> buffer, callback_t&& cb) {
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
    ELOG_DEBUG << "Remote QP number =  " << peer_info.qp_num;
    remote_qp_num_ = peer_info.qp_num;
    ELOG_DEBUG << "Remote LID =  " << peer_info.lid;
    std::error_code err_code;
    remote_address_ =
        asio::ip::make_address(detail::gid_to_string(peer_info.gid), err_code);
    if (err_code) {
      ELOG_ERROR << "IBDevice failed to convert remote gid to ip address of "
                    "device error msg: "
                 << err_code.message();
      co_return err_code;
    }
    ELOG_DEBUG << "Remote address = " << remote_address_;
    ELOG_DEBUG << "Remote Request buffer size = " << peer_info.buffer_size;
    buffer_size_ = std::min<uint32_t>(peer_info.buffer_size,
                                      get_config().request_buffer_size);
    ELOG_DEBUG << "Final buffer size = " << buffer_size_;
    if (buffer_size_ > buffer_pool()->max_buffer_size()) {
      ELOG_WARN << "Buffer size larger than buffer_pool limit: "
                << buffer_size_;
      co_return std::make_error_code(std::errc::no_buffer_space);
    }
    try {
      init_qp();
      modify_qp_to_init();
      modify_qp_to_rtr(peer_info.qp_num, peer_info.lid,
                       (uint8_t*)peer_info.gid);
      modify_qp_to_rts();
      init_fd();
    } catch (const std::system_error& err) {
      co_return err.code();
    }

    for (int i = 0; i < conf_.recv_buffer_cnt; ++i) {
      if (auto ec = state_->recv_queue_.push(
              state_->ib_buffer_pool_->get_buffer(), state_.get());
          ec) {
        co_return ec;
      }
    }
    if (state_->recv_queue_.empty()) {
      ELOG_WARN << "buffer out of limit, init ib_socket failed";
      co_return std::make_error_code(std::errc::no_buffer_space);
    }

    peer_info.qp_num = state_->qp_->qp_num;
    peer_info.lid = state_->device_->attr().lid;
    peer_info.buffer_size = conf_.request_buffer_size;
    memcpy(peer_info.gid, &state_->device_->gid(), sizeof(peer_info.gid));
    struct_pack::serialize_to((char*)buffer, sz, peer_info);
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

  asio::ip::address get_remote_address() const noexcept {
    return remote_address_;
  }

  uint32_t get_remote_qp_num() const noexcept { return remote_qp_num_; }

  asio::ip::address get_local_address() const noexcept {
    return state_->device_->gid_address();
  }

  uint32_t get_local_qp_num() const noexcept { return state_->qp_->qp_num; }

  async_simple::coro::Lazy<std::error_code> connect(
      asio::ip::tcp::socket& soc) noexcept {
    try {
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
      auto [ec, len] = co_await async_write(soc, asio::buffer(buffer));
      if (ec) {
        co_return std::move(ec);
      }
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
        ELOG_INFO << "protocol error. connect to unknown protocol server";
        co_return std::make_error_code(std::errc::protocol_error);
      }
      ELOG_TRACE << "Remote QP number =  " << peer_info.qp_num;
      remote_qp_num_ = peer_info.qp_num;
      ELOG_TRACE << "Remote LID =  " << peer_info.lid;
      std::error_code err_code;
      remote_address_ = asio::ip::make_address(
          detail::gid_to_string(peer_info.gid), err_code);
      if (err_code) {
        ELOG_ERROR
            << "IBDevice failed to convert remote gid to ip address of device "
            << " error msg: " << err_code.message();
        co_return err_code;
      }
      ELOG_TRACE << "Remote address = " << remote_address_;
      ELOG_TRACE << "Remote Request buffer size = " << peer_info.buffer_size;
      buffer_size_ = std::min<uint32_t>(peer_info.buffer_size,
                                        get_config().request_buffer_size);
      ELOG_TRACE << "Final buffer size = " << buffer_size_;
      if (buffer_size_ > buffer_pool()->max_buffer_size()) {
        ELOG_WARN << "Buffer size larger than buffer_pool limit: "
                  << buffer_size_;
        co_return std::make_error_code(std::errc::no_buffer_space);
      }
      modify_qp_to_rtr(peer_info.qp_num, peer_info.lid,
                       (uint8_t*)peer_info.gid);
      for (int i = 0; i < conf_.recv_buffer_cnt; ++i) {
        if (auto ec = state_->recv_queue_.push(
                state_->ib_buffer_pool_->get_buffer(), state_.get());
            ec) {
          co_return ec;
        }
      }
      if (state_->recv_queue_.empty()) {
        ELOG_WARN << "buffer out of limit, init ib_socket failed";
        co_return std::make_error_code(std::errc::no_buffer_space);
      }
      modify_qp_to_rts();
      ELOG_INFO << "start init fd";
      init_fd();
    } catch (std::system_error& err) {
      co_return err.code();
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
      auto err_code = std::make_error_code(std::errc{errno});
      ELOG_ERROR << " ibv_channel init failed" << err_code.message();
      throw std::system_error(err_code);
    }
    state_->cq_.reset(ibv_create_cq(state_->device_->context(), config.cq_size,
                                    NULL, state_->channel_.get(), 0));
    if (!state_->cq_) [[unlikely]] {
      auto err_code = std::make_error_code(std::errc{errno});
      ELOG_ERROR << " ibv_cq init failed" << err_code.message();
      throw std::system_error(err_code);
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
      auto err_code = std::make_error_code(std::errc{errno});
      ELOG_ERROR << "init qp failed: " << err_code.message();
      throw std::system_error(err_code);
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
      auto err_code = std::make_error_code(std::errc{ec});
      ELOG_ERROR << "modify qp to init failed: " << err_code.message();
      throw std::system_error(err_code);
    }
  }

  void modify_qp_to_rtr(uint32_t remote_qpn, uint16_t dlid, uint8_t* dgid) {
    ibv_qp_attr attr{};
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = state_->device_->attr().active_mtu;
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
      auto err_code = std::make_error_code(std::errc{ec});
      ELOG_ERROR << "modify qp to rtr failed: " << err_code.message();
      throw std::system_error(err_code);
    }
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
      auto err_code = std::make_error_code(std::errc{ec});
      ELOG_ERROR << "modify qp to rtss failed: " << err_code.message();
      throw std::system_error(err_code);
    }
  }

  void init_fd() {
    int r = ibv_req_notify_cq(state_->cq_.get(), 0);
    if (r) {
      auto err_code = std::make_error_code(std::errc{errno});
      ELOG_ERROR << "ibv_req_notify_cq failed: " << err_code.message();
      throw std::system_error(err_code);
    }

    state_->fd_ = std::make_unique<asio::posix::stream_descriptor>(
        executor_->get_asio_executor(), state_->channel_->fd);
    auto listen_event = [](std::shared_ptr<ib_socket_shared_state_t> self)
        -> async_simple::coro::Lazy<void> {
      std::error_code ec;
      while (!ec) {
        coro_io::callback_awaitor<std::error_code> awaitor;
        ec = co_await awaitor.await_resume([&self](auto handler) {
          self->fd_->async_wait(asio::posix::stream_descriptor::wait_read,
                                [handler](const std::error_code& ec) mutable {
                                  handler.set_value_then_resume(ec);
                                });
        });
        if (!ec) {
          ec = self->poll_completion();
        }

        if (ec) {
          std::error_code ec_ignore;
          self->close(ec_ignore);
          ib_socket_shared_state_t::resume(std::pair{ec, std::size_t{0}},
                                           self->recv_cb_);
          ib_socket_shared_state_t::resume(std::pair{ec, std::size_t{0}},
                                           self->send_cb_);
          break;
        }
      }
      if (ec) {
        ELOG_DEBUG << "ib_socket stop listening by error: " << ec.message();
      }
    };
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
            ib_socket_shared_state_t::resume(std::move(result),
                                             state->recv_cb_);
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
    sr.opcode = IBV_WR_SEND;
    sr.send_flags = IBV_SEND_SIGNALED;
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
    return {};
  }

  void safe_resume(std::error_code ec, callback_t&& cb) {
    asio::dispatch(
        executor_->get_asio_executor(), [ec, cb = std::move(cb)]() mutable {
          ib_socket_shared_state_t::resume(std::pair{ec, std::size_t{0}}, cb);
        });
  }
  asio::ip::address remote_address_;
  uint32_t remote_qp_num_{};
  coro_io::ExecutorWrapper<>* executor_;
  std::string_view remain_data_;
  std::unique_ptr<asio::ip::tcp::socket> soc_;
  std::shared_ptr<ib_socket_shared_state_t> state_;
  ibverbs_config conf_;
  uint32_t buffer_size_ = 0;
};
}  // namespace coro_io
