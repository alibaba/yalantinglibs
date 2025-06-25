#pragma once
#include <infiniband/verbs.h>
#include <sys/stat.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <queue>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "asio/dispatch.hpp"
#include "asio/ip/address.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/posix/stream_descriptor.hpp"
#include "async_simple/Signal.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/util/move_only_function.h"
#include "ib_device.hpp"
#include "ib_error.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/ibverbs/ib_buffer.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/struct_pack.hpp"
#include "ylt/util/concurrentqueue.h"

namespace coro_io {
namespace detail {
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
  std::error_code push_recv(ib_buffer_t buf, ib_socket_shared_state_t* state) {
    if (!buf || buf->length == 0) {
      ELOG_INFO << "Out of ib_buffer_pool limit." << size();
      return {};
    }
    auto ec = post_recv_real(buf.subview(), state);
    if (!ec) {
      push(std::move(buf));
    }
    return ec;
  }
  void push(ib_buffer_t buf) {
    may_empty = false;
    front = (front + 1) % queue.size();
    queue[front] = std::move(buf);
  }
  ib_buffer_t pop() {
    end = (end + 1) % queue.size();
    may_empty = true;
    return std::move(queue[end]);
  }
  bool full() const noexcept { return front == end && !may_empty; }
  bool empty() const noexcept { return front == end && may_empty; }
  std::size_t size() {
    if (end > front) {
      return queue.size() + front - end;
    }
    else if (end == front) {
      return empty() ? 0 : queue.size();
    }
    else {
      return front - end;
    }
  }
};

struct ib_socket_shared_state_t
    : std::enable_shared_from_this<ib_socket_shared_state_t> {
  using callback_t = async_simple::util::move_only_function<void(
      std::pair<std::error_code, std::size_t>)>;
  static void resume(std::pair<std::error_code, std::size_t>&& arg,
                     callback_t& handle) {
    if (handle) [[likely]] {
      auto handle_tmp = std::move(handle);
      handle_tmp(std::move(arg));
    }
  }
  coro_io::ExecutorWrapper<>* executor_;
  std::shared_ptr<ib_device_t> device_;
  ib_buffer_queue recv_queue_, send_queue_;
  std::size_t recv_buffer_cnt_;
  std::queue<std::pair<std::error_code, std::size_t>>
      recv_result;  // TODO optimize with circle buffer
  callback_t recv_cb_;
  std::deque<callback_t> send_cb_;
  ib_buffer_t recv_buf_;
  std::unique_ptr<asio::posix::stream_descriptor> fd_;
  std::unique_ptr<ibv_comp_channel, ib_deleter> channel_;
  std::unique_ptr<ibv_cq, ib_deleter> cq_;
  std::unique_ptr<ibv_qp, ib_deleter> qp_;

  asio::ip::tcp::socket soc_;
  std::atomic<bool> has_close_ = false;
  bool peer_close_ = false;

  ib_socket_shared_state_t(std::shared_ptr<ib_device_t> device,
                           coro_io::ExecutorWrapper<>* executor,
                           std::size_t recv_buffer_cnt,
                           std::size_t max_recv_buffer_cnt)
      : device_(device),
        executor_(executor),
        soc_(executor->get_asio_executor()),
        recv_buffer_cnt_(recv_buffer_cnt),
        recv_queue_(max_recv_buffer_cnt),
        send_queue_(2) {}

  ib_socket_shared_state_t(ib_socket_shared_state_t&&) = delete;
  ib_socket_shared_state_t& operator=(ib_socket_shared_state_t&&) = delete;

  void return_send_buffer(ib_buffer_t buffer) {
    if (!send_queue_.full()) {
      send_queue_.push(std::move(buffer));
    }
  }

  void cancel() {
    assert(executor_->get_asio_executor().running_in_this_thread());
    std::error_code ec;
    soc_.cancel(ec);
    if (fd_) {
      fd_->cancel(ec);
    }
  }

  void close_impl() {
    std::error_code ec;
    soc_.cancel(ec);
    soc_.close(ec);
    if (qp_) {
      ibv_qp_attr attr;
      memset(&attr, 0, sizeof(attr));
      attr.qp_state = IBV_QPS_RESET;

      int ret = ibv_modify_qp(qp_.get(), &attr, IBV_QP_STATE);
      if (ret) {
        ELOG_ERROR << "ibv_modify_qp IBV_QPS_RESET failed, "
                   << std::make_error_code(std::errc{ret}).message();
      }
    }
    if (fd_) {
      fd_->cancel(ec);
      fd_->release();
    }
  }

  void close(bool should_check = true) {
    assert(executor_->get_asio_executor().running_in_this_thread());

    bool has_close = false;
    if (should_check) {
      has_close = has_close_.exchange(true);
    }
    if (!has_close) {
      if (fd_ && !peer_close_) {
        shutdown().start([self = shared_from_this()](auto&&) {
          self->close_impl();
        });
      }
      else {
        close_impl();
      }
    }
  }

  void add_recv_buffer(ib_buffer_t buffer, std::size_t free_buffer_limit) {
    if (!peer_close_ && !recv_queue_.full()) {
      auto free_buffer_cnt = recv_queue_.size() - recv_result.size();
      if (free_buffer_cnt < free_buffer_limit) {
        if (!buffer) {
          buffer = device_->get_buffer_pool()->get_buffer();
        }
        if (recv_queue_.push_recv(std::move(buffer), this)) {
          close();
        }
      }
    }
  }

  auto get_executor() const noexcept { return executor_->get_asio_executor(); }

  async_simple::coro::Lazy<void> shutdown() {
    ELOG_TRACE << "start to nofity peer close";
    co_await collectAll<async_simple::SignalType::Terminate>(
        coro_io::async_io<std::pair<std::error_code, std::size_t>>(
            [this](auto&& cb) mutable {
              post_send_impl({}, false, std::move(cb));
            },
            *this),
        coro_io::sleep_for(std::chrono::seconds{1}, executor_));
    ELOG_TRACE << "finished to nofity peer close";
    co_return;
  }

  struct resume_struct {
    std::error_code ec;
    std::size_t len;
    uint64_t wr_id;
  };

  void post_send_impl(std::span<ibv_sge> sge, bool enable_inline_send,
                      callback_t&& handler) {
    ibv_send_wr sr{};
    ibv_send_wr* bad_wr = nullptr;
    if (enable_inline_send) {
      sr.send_flags = IBV_SEND_INLINE;
    }
    sr.next = NULL;
    sr.wr_id = 1;
    sr.sg_list = sge.data();
    sr.num_sge = sge.size();
    sr.opcode = IBV_WR_SEND;
    sr.send_flags |= IBV_SEND_SIGNALED;
    std::error_code err;
    if ((sge.size() && has_close_))
        [[unlikely]] {  // if sge is empty, client don't real close, it's trying
                        // to notify peer to close
      err = std::make_error_code(std::errc::operation_canceled);
    }
    // post the receive request to the RQ
    else if (auto ec = ibv_post_send(qp_.get(), &sr, &bad_wr); ec)
        [[unlikely]] {
      err = std::make_error_code(std::errc{std::abs(ec)});
      ELOG_ERROR << "ibv post send failed: " << err.message();
    }
    if (err) [[unlikely]] {
      ib_socket_shared_state_t::resume(std::pair{err, std::size_t{0}}, handler);
    }
    else {
      send_cb_.push_back(std::move(handler));
    }
  }

  void post_recv_impl(callback_t&& handler) {
    if (has_close_) [[unlikely]] {
      ib_socket_shared_state_t::resume(
          std::pair{std::make_error_code(std::errc::io_error), 0}, handler);
      return;
    }
    recv_cb_ = std::move(handler);
    if (!recv_result.empty()) {
      auto result = recv_result.front();
      recv_result.pop();
      recv_buf_ = std::move(recv_queue_.pop());
      add_recv_buffer({}, recv_buffer_cnt_ / 2);
      ib_socket_shared_state_t::resume(std::move(result), recv_cb_);
    }
  }

  std::error_code poll_completion() {
    void* ev_ctx;
    auto cq = cq_.get();
    int r = ibv_get_cq_event(channel_.get(), &cq, &ev_ctx);
    // assert(r >= 0);
    std::error_code ec;
    if (r) {
      ELOG_INFO << "there isn't any Completion event to read";
      return std::make_error_code(std::errc{std::errc::io_error});
    }
    ibv_ack_cq_events(cq, 1);
    r = ibv_req_notify_cq(cq, 0);
    if (r) {
      ELOG_ERROR << std::make_error_code(std::errc{r}).message();
      return std::make_error_code(std::errc{r});
    }
    struct ibv_wc wc{};
    int ne = 0;
    std::vector<resume_struct> vec;
    callback_t tmp_recv_callback;
    while ((ne = ibv_poll_cq(cq_.get(), 1, &wc)) != 0) {
      if (ne < 0) {
        ELOG_ERROR << "poll CQ failed:" << ne;
        ec = std::make_error_code(std::errc::io_error);
        break;
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
          if (wc.byte_len == 0) {
            ELOG_DEBUG << "close by peer";
            peer_close_ = true;
            close();
            continue;
          }
          if (recv_cb_) {
            assert(recv_result.empty());
            recv_buf_ = recv_queue_.pop();
            add_recv_buffer({}, recv_buffer_cnt_ / 2);
            tmp_recv_callback = std::move(recv_cb_);
            vec.push_back({ec, wc.byte_len, 0});
          }
          else {
            recv_result.push(std::pair{ec, (std::size_t)wc.byte_len});
            add_recv_buffer({}, recv_buffer_cnt_ / 2);
          }
        }
        else {
          vec.push_back({ec, wc.byte_len, wc.wr_id});
        }
        if (cq_ == nullptr) {
          break;
        }
      }
    }
    for (auto& result : vec) {
      if (result.wr_id == 0) {
        resume(std::pair{result.ec, result.len}, tmp_recv_callback);
      }
      else {
        resume(std::pair{result.ec, result.len}, send_cb_.front());
        send_cb_.pop_front();
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
    auto error_code = std::make_error_code(std::errc{std::abs(ec)});
    ELOG_ERROR << "ibv post recv failed: " << error_code.message();
    return error_code;
  }
  return {};
}
}  // namespace detail

class ib_socket_t {
 public:
  // workaround for some compiler error if we put ib_socket_config_base_t as
  // nested struct
  struct config_t {
    uint32_t cq_size = 128;
    uint32_t recv_buffer_cnt = 4;
    ibv_qp_type qp_type = IBV_QPT_RC;
    ibv_qp_cap cap = {.max_send_wr = 32,
                      .max_recv_wr = 32,
                      .max_send_sge = 3,
                      .max_recv_sge = 1,
                      .max_inline_data = 256};
    std::shared_ptr<ib_device_t> device;
  };
  enum io_type { recv = 0, send = 1 };
  using callback_t = detail::ib_socket_shared_state_t::callback_t;
  struct ib_socket_info {
    uint8_t gid[16];       // GID
    uint16_t lid;          // LID of the IB port
    uint32_t buffer_size;  // buffer length
    uint32_t qp_num;       // QP number
  };

  ib_socket_t(coro_io::ExecutorWrapper<>* executor, const config_t& config)
      : executor_(executor) {
    init(config);
  }

 public:
  ib_socket_t(
      coro_io::ExecutorWrapper<>* executor = coro_io::get_global_executor())
      : executor_(executor) {
    init(config_t{});
  }

  ib_socket_t(ib_socket_t&&) = default;
  ib_socket_t& operator=(ib_socket_t&& o) {
    close();
    remote_address_ = std::move(o.remote_address_);
    remote_qp_num_ = o.remote_qp_num_;
    remain_data_ = o.remain_data_;
    state_ = std::move(o.state_);
    executor_ = o.executor_;
    conf_ = std::move(o.conf_);
    buffer_size_ = o.buffer_size_;
    return *this;
  }
  ~ib_socket_t() { close(); }

  bool is_open() const noexcept {
    return state_->fd_ != nullptr && state_->fd_->is_open() &&
           !state_->has_close_;
  }
  std::shared_ptr<ib_buffer_pool_t> buffer_pool() const noexcept {
    return state_->device_->get_buffer_pool();
  }

  std::size_t consume(char* dst, std::size_t sz) {
    auto len = std::min(sz, remain_data_.size());
    if (len) {
      memcpy(dst, remain_data_.data(), len);
      remain_data_ = remain_data_.substr(len);
      if (remain_data_.empty()) {
        assert(state_->recv_buf_);
        state_->add_recv_buffer(std::move(state_->recv_buf_),
                                state_->recv_buffer_cnt_ / 2 + 1);
      }
    }
    return len;
  }

  std::size_t remain_read_buffer_size() { return remain_data_.size(); }
  void set_read_buffer_len(std::size_t has_read_size, std::size_t remain_size) {
    remain_data_ = std::string_view{
        (char*)state_->recv_buf_->addr + has_read_size, remain_size};
    if (remain_size == 0) {
      state_->add_recv_buffer(std::move(state_->recv_buf_),
                              state_->recv_buffer_cnt_ / 2 + 1);
    }
  }

  ibv_sge get_recv_buffer() {
    assert(remain_read_buffer_size() == 0);
    assert(state_->recv_buf_);
    return state_->recv_buf_.subview();
  }

  void post_recv(callback_t&& cb) { state_->post_recv_impl(std::move(cb)); }

  void post_send(std::span<ibv_sge> buffer, bool enable_inline_send,
                 callback_t&& cb) {
    if (buffer.size()) {
      ELOG_TRACE << "post send sge cnt:" << buffer.size()
                 << ", address:" << buffer[0].addr
                 << ",length:" << buffer[0].length;
    }
    state_->post_send_impl(buffer, enable_inline_send, std::move(cb));
  }

  uint32_t get_buffer_size() const noexcept { return buffer_size_; }

  config_t& get_config() noexcept { return conf_; }
  const config_t& get_config() const noexcept { return conf_; }
  auto get_device() const noexcept { return state_->device_; }

  async_simple::coro::Lazy<std::error_code> accept() noexcept {
    ib_socket_t::ib_socket_info peer_info;
    constexpr auto sz = struct_pack::get_needed_size(peer_info);
    char buffer[sz.size()];
    auto [ec, _] = co_await async_read(state_->soc_, asio::buffer(buffer));
    if (ec) [[unlikely]] {
      co_return ec;
    }
    if (state_->channel_ == nullptr) [[unlikely]] {
      co_return std::make_error_code(std::errc::protocol_error);
    }
    auto ec2 = struct_pack::deserialize_to(peer_info, std::span{buffer});
    if (ec2) [[unlikely]] {
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
    buffer_size_ =
        std::min<uint32_t>(peer_info.buffer_size, buffer_pool()->buffer_size());
    ELOG_DEBUG << "Final buffer size = " << buffer_size_;
    if (buffer_size_ > buffer_pool()->buffer_size()) {
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
      if (auto ec = state_->recv_queue_.push_recv(
              state_->device_->get_buffer_pool()->get_buffer(), state_.get());
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
    peer_info.buffer_size = buffer_pool()->buffer_size();
    memcpy(peer_info.gid, &state_->device_->gid(), sizeof(peer_info.gid));
    struct_pack::serialize_to((char*)buffer, sz, peer_info);
    async_write(state_->soc_, asio::buffer(buffer))
        .start([state = state_](auto&& ec) {
          try {
            if (!ec.value().first) {
              std::error_code ec;
              state->soc_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
              state->soc_.close(ec);
              return;
            }
            ELOG_WARN << "rdma connection failed by exception:"
                      << ec.value().first.message();
          } catch (const std::exception& e) {
            ELOG_WARN << "rdma connection failed by exception:" << e.what();
          }
          state->close();
        });
    co_return std::error_code{};
  }
  void prepare_accpet(asio::ip::tcp::socket soc) noexcept {
    state_->soc_ = std::move(soc);
  }

  async_simple::coro::Lazy<std::error_code> accept(
      asio::ip::tcp::socket soc) noexcept {
    state_->soc_ = std::move(soc);
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

  async_simple::coro::Lazy<std::error_code> connect_impl() noexcept {
    try {
      init_qp();
      modify_qp_to_init();
      for (int i = 0; i < conf_.recv_buffer_cnt; ++i) {
        if (auto ec = state_->recv_queue_.push_recv(
                state_->device_->get_buffer_pool()->get_buffer(), state_.get());
            ec) {
          co_return ec;
        }
      }
      if (state_->recv_queue_.empty()) {
        ELOG_WARN << "buffer out of limit, init ib_socket failed";
        co_return std::make_error_code(std::errc::no_buffer_space);
      }
      ib_socket_t::ib_socket_info peer_info{};
      peer_info.qp_num = state_->qp_->qp_num;
      peer_info.lid = state_->device_->attr().lid;
      peer_info.buffer_size = buffer_pool()->buffer_size();
      memcpy(peer_info.gid, &state_->device_->gid(), sizeof(peer_info.gid));
      constexpr auto sz = struct_pack::get_needed_size(peer_info);
      char buffer[sz.size()];
      struct_pack::serialize_to((char*)buffer, sz, peer_info);
      auto [ec, len] = co_await async_write(state_->soc_, asio::buffer(buffer));
      if (ec) {
        co_return std::move(ec);
      }
      std::tie(ec, len) =
          co_await async_read(state_->soc_, asio::buffer(buffer));
      std::error_code ignore_ec;
      state_->soc_.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
      state_->soc_.close(ignore_ec);
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
                                        buffer_pool()->buffer_size());
      ELOG_TRACE << "Final buffer size = " << buffer_size_;
      if (buffer_size_ > buffer_pool()->buffer_size()) {
        ELOG_WARN << "Buffer size larger than buffer_pool limit: "
                  << buffer_size_;
        co_return std::make_error_code(std::errc::no_buffer_space);
      }
      modify_qp_to_rtr(peer_info.qp_num, peer_info.lid,
                       (uint8_t*)peer_info.gid);
      modify_qp_to_rts();
      ELOG_INFO << "start init fd";
      init_fd();
    } catch (std::system_error& err) {
      co_return err.code();
    }
    co_return std::error_code{};
  }

  void close() {
    if (state_) {
      if (!state_->has_close_.exchange(true)) {
        asio::dispatch(executor_->get_asio_executor(), [state = state_]() {
          state->close(false);
        });
      }
    }
  }
  auto get_executor() const { return executor_->get_asio_executor(); }
  auto get_coro_executor() const { return executor_; }
  auto cancel() const {
    if (state_) {
      asio::dispatch(executor_->get_asio_executor(), [state = state_]() {
        state->cancel();
      });
    }
  }

  async_simple::coro::Lazy<std::error_code> connect(
      const std::string& host, const std::string& port) noexcept {
    auto ec =
        co_await async_connect(get_coro_executor(), state_->soc_, host, port);
    if (ec) [[unlikely]] {
      co_return std::move(ec);
    }
    co_return co_await connect_impl();
  }

  template <typename EndPointSeq>
  async_simple::coro::Lazy<std::error_code> connect(
      const EndPointSeq& endpoint) noexcept {
    auto ec = co_await async_connect(state_->soc_, endpoint);
    if (ec) [[unlikely]] {
      co_return std::move(ec);
    }
    co_return co_await connect_impl();
  }

  ib_buffer_t get_send_buffer() noexcept {
    if (state_->send_queue_.empty()) {
      return buffer_pool()->get_buffer();
    }
    else {
      return state_->send_queue_.pop();
    }
  }

  std::shared_ptr<detail::ib_socket_shared_state_t> get_state() noexcept {
    return state_;
  }

 private:
  void init(const config_t& config) {
    conf_ = config;
    if (conf_.device == nullptr) {
      conf_.device = coro_io::get_global_ib_device();
    }
    ELOG_INFO << "device name: " << conf_.device->name();
    conf_.recv_buffer_cnt = std::max<uint32_t>(conf_.recv_buffer_cnt, 1);
    conf_.cap.max_recv_sge = std::max<uint32_t>(conf_.cap.max_recv_sge, 1);
    conf_.cap.max_send_sge = std::max<uint32_t>(conf_.cap.max_send_sge, 1);
    conf_.cap.max_send_wr = std::max<uint32_t>(conf_.cap.max_send_wr, 1);
    conf_.cap.max_recv_wr =
        std::max<uint32_t>(conf_.cap.max_recv_wr, conf_.recv_buffer_cnt);
    if (!conf_.device->is_support_inline_data()) {
      conf_.cap.max_inline_data = 0;
    }
    state_ = std::make_shared<detail::ib_socket_shared_state_t>(
        conf_.device, executor_, conf_.recv_buffer_cnt, conf_.cap.max_recv_wr);
    state_->channel_.reset(ibv_create_comp_channel(state_->device_->context()));
    if (!state_->channel_) [[unlikely]] {
      auto err_code = std::make_error_code(std::errc{errno});
      ELOG_ERROR << " ibv_channel init failed" << err_code.message();
      throw std::system_error(err_code);
    }
    state_->cq_.reset(ibv_create_cq(state_->device_->context(), conf_.cq_size,
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
    ibv_qp* qp;
    do {
      qp = ibv_create_qp(state_->device_->pd(), &qp_init_attr);
      if (qp == nullptr) {
        auto err_code = std::make_error_code(std::errc{errno});
        if (qp_init_attr.cap.max_inline_data > 0 &&
            err_code.value() == EINVAL) {
          qp_init_attr.cap.max_inline_data = 0;
        }
        else {
          ELOG_ERROR << "init qp failed: " << err_code.message();
          throw std::system_error(err_code);
        }
      }
      else {
        break;
      }
    } while (true);
    if (qp_init_attr.cap.max_inline_data == 0 &&
        conf_.cap.max_inline_data != 0) {
      get_device()->set_support_inline_data(false);
      conf_.cap.max_inline_data = 0;
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
    attr.min_rnr_timer = 8;
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
    attr.rnr_retry = 7;
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
      auto err_code = std::make_error_code(std::errc{r});
      ELOG_ERROR << "ibv_req_notify_cq failed: " << err_code.message();
      throw std::system_error(err_code);
    }

    state_->fd_ = std::make_unique<asio::posix::stream_descriptor>(
        executor_->get_asio_executor(), state_->channel_->fd);
    auto listen_event =
        [](std::shared_ptr<detail::ib_socket_shared_state_t> self)
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
          if (ec) {
            ELOG_DEBUG << "channel closed by poll_completion:" << ec.message();
          }
        }
        else {
          ELOG_DEBUG << "channel closed by channel:" << ec.message();
        }

        if (ec) {
          self->peer_close_ = true;
          self->close();
          detail::ib_socket_shared_state_t::resume(
              std::pair{ec, std::size_t{0}}, self->recv_cb_);
          while (!self->send_cb_.empty()) {
            detail::ib_socket_shared_state_t::resume(
                std::pair{ec, std::size_t{0}}, self->send_cb_.front());
            self->send_cb_.pop_front();
          }

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

  asio::ip::address remote_address_;
  uint32_t remote_qp_num_{};
  std::string_view remain_data_;
  std::shared_ptr<detail::ib_socket_shared_state_t> state_;
  coro_io::ExecutorWrapper<>* executor_;
  config_t conf_;
  uint32_t buffer_size_ = 0;
};
}  // namespace coro_io
