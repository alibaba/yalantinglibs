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
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <netinet/in.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "async_simple/Signal.h"
#include "async_simple/coro/LazyLocalBase.h"
#include "asio/dispatch.hpp"
#include "asio/ip/address.hpp"
#include "asio/posix/stream_descriptor.hpp"
#include "ib_buffer.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/util/type_traits.h"

namespace coro_io {
namespace detail {
class ib_devices_t {
 public:
  static auto& instance() {
    static ib_devices_t inst{};
    return inst;
  }
  int size() { return size_; }
  ibv_device* operator[](int i) {
    if (i >= size_ || i < 0) {
      throw std::out_of_range("out of range");
    }
    return dev_list_[i];
  }

  ibv_device* at(const std::string& dev_name) {
    if (dev_name.empty()) {
      return (*this)[0];
    }

    auto list = get_devices();

    auto it = std::find_if(list.begin(), list.end(), [&dev_name](auto dev) {
      return dev->name == dev_name;
    });
    size_t i = std::distance(list.begin(), it);
    return (*this)[i];
  }

  std::span<ibv_device*> get_devices() { return {dev_list_, (size_t)size_}; }

 private:
  ib_devices_t() {
    dev_list_ = ibv_get_device_list(&size_);
    if (dev_list_ == nullptr) {
      size_ = 0;
    }
  }

  ~ib_devices_t() {
    if (dev_list_ != nullptr) {
      ibv_free_device_list(dev_list_);
    }
  }

  int size_;
  ibv_device** dev_list_;
};
inline std::string gid_to_string(uint8_t (&a)[16]) noexcept {
  std::string ret;
  ret.resize(39);
  snprintf(ret.data(), ret.size() + 1,
           "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%"
           "02x%02x",
           a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10],
           a[11], a[12], a[13], a[14], a[15]);
  return ret;
}

inline std::string mtu_str(ibv_mtu mtu) {
  std::string str;
  switch (mtu) {
    case IBV_MTU_256:
      str = "IBV_MTU_256";
      break;
    case IBV_MTU_512:
      str = "IBV_MTU_512";
      break;
    case IBV_MTU_1024:
      str = "IBV_MTU_1024";
      break;
    case IBV_MTU_2048:
      str = "IBV_MTU_2048";
      break;
    case IBV_MTU_4096:
      str = "IBV_MTU_4096";
      break;
    default:
      break;
  }
  return str;
}

/* helper function to print the content of the async event */
inline ibv_qp* print_async_event(struct ibv_context* ctx,
                                 struct ibv_async_event* event) {
  ibv_qp* qp = nullptr;
  switch (event->event_type) {
    /* QP events */
    case IBV_EVENT_QP_FATAL:
      ELOG_WARN << "QP fatal event for QP number:" << event->element.qp->qp_num;
      qp = event->element.qp;
      break;
    case IBV_EVENT_QP_REQ_ERR:
      ELOG_WARN << "QP Requestor error for QP number:"
                << event->element.qp->qp_num;
      qp = event->element.qp;
      break;
    case IBV_EVENT_QP_ACCESS_ERR:
      ELOG_WARN << "QP access error event for QP number:"
                << event->element.qp->qp_num;
      qp = event->element.qp;
      break;
    case IBV_EVENT_COMM_EST:
      ELOG_INFO << "QP communication established event for QP number:"
                << event->element.qp->qp_num;
      break;
    case IBV_EVENT_SQ_DRAINED:
      ELOG_INFO << "QP Send Queue drained event for QP number:"
                << event->element.qp->qp_num;
      break;
    case IBV_EVENT_PATH_MIG:
      ELOG_INFO << "QP Path migration loaded event for QP number:"
                << event->element.qp->qp_num;
      break;
    case IBV_EVENT_PATH_MIG_ERR:
      ELOG_WARN << "QP Path migration error event for QP number:"
                << event->element.qp->qp_num;
      qp = event->element.qp;
      break;
    case IBV_EVENT_QP_LAST_WQE_REACHED:
      ELOG_INFO << "QP last WQE reached event for QP number:"
                << event->element.qp->qp_num;
      break;

    /* CQ events */
    case IBV_EVENT_CQ_ERR:
      ELOG_WARN << "CQ error for CQ with handle " << event->element.cq;
      break;

    /* SRQ events */
    case IBV_EVENT_SRQ_ERR:
      ELOG_WARN << "SRQ error for SRQ with handle " << event->element.srq;
      break;
    case IBV_EVENT_SRQ_LIMIT_REACHED:
      ELOG_INFO << "SRQ limit reached event for SRQ with handle "
                << event->element.srq;
      break;

    /* Port events */
    case IBV_EVENT_PORT_ACTIVE:
      ELOG_INFO << "Port active event for port number "
                << event->element.port_num;
      break;
    case IBV_EVENT_PORT_ERR:
      ELOG_WARN << "Port error event for port number "
                << event->element.port_num;
      break;
    case IBV_EVENT_LID_CHANGE:
      ELOG_INFO << "LID change event for port number "
                << event->element.port_num;
      break;
    case IBV_EVENT_PKEY_CHANGE:
      ELOG_INFO << "P_Key table change event for port number "
                << event->element.port_num;
      break;
    case IBV_EVENT_GID_CHANGE:
      ELOG_INFO << "GID table change event for port number "
                << event->element.port_num;
      break;
    case IBV_EVENT_SM_CHANGE:
      ELOG_INFO << "SM change event for port number "
                << event->element.port_num;
      break;
    case IBV_EVENT_CLIENT_REREGISTER:
      ELOG_INFO << "Client reregister event for port number "
                << event->element.port_num;
      break;

    /* RDMA device events */
    case IBV_EVENT_DEVICE_FATAL:
      ELOG_WARN << "Fatal error event for device "
                << ibv_get_device_name(ctx->device);
      break;

    default:
      ELOG_WARN << "Unknown event (" << event->event_type << ")";
  }
  return qp;
}

}  // namespace detail

class ib_device_t : public std::enable_shared_from_this<ib_device_t> {
 public:
  struct config_t {
    std::string dev_name;
    uint16_t port = 1;               // dev gid
    uint16_t gid_index = 0;          // dev gid_index
    bool use_best_gid_index = true;  // automatically find best gid index. If
                                     // failed, it will use gid_index.
    ib_buffer_pool_t::config_t buffer_pool_config;

    bool use_srq = true;  // auto-detect: falls back to per-QP mode if SRQ creation fails
    uint64_t max_srq_buffer_memory = 256 * 1024 * 1024;  // 256MB
    uint32_t srq_max_wr = 4096;
    uint32_t srq_idle_timeout_ms = 3000;  // delay before SRQ destruction when all QPs are gone
    uint32_t srq_buffer_block = 32;      // initial/minimum SRQ buffer count
  };
  static std::shared_ptr<ib_device_t> create(const config_t& conf,
                                             ibv_device* dev = nullptr) {
    auto ret =
        std::make_shared<ib_device_t>(private_construct_token{}, conf, dev);
    ret->start_async_event_watcher();
    return ret;
  }

 private:
  struct private_construct_token {};

  void poll_async_events() {
    ibv_async_event event;
    ibv_qp_attr attr{.qp_state = IBV_QPS_ERR};
    while (ibv_get_async_event(ctx_.get(), &event) == 0) {
      ELOG_INFO << "IBDevice(" << name()
                << ") get async event: " << event.event_type;
      auto qp = detail::print_async_event(ctx_.get(), &event);
      if (qp) {
        ibv_modify_qp(qp, &attr, IBV_QP_STATE);
      }
      ibv_ack_async_event(&event);
    }
    ELOG_WARN << "IBDevice(" << name() << ") poll async events thread exit.";
  }

 public:
  ib_device_t(private_construct_token, const config_t& conf,
              ibv_device* dev = nullptr) {
    auto& inst = detail::ib_devices_t::instance();
    port_ = conf.port;
    if (dev == nullptr) {
      dev = inst.at(conf.dev_name);
    }
    name_ = ibv_get_device_name(dev);
    ctx_.reset(ibv_open_device(dev));
    pd_.reset(ibv_alloc_pd(ctx_.get()));

    if (auto ret = ibv_query_port(ctx_.get(), conf.port, &attr_); ret != 0) {
      auto ec = std::make_error_code((std::errc)ret);
      ELOG_ERROR << "IBDevice failed to query port " << conf.port
                 << " of device " << name_ << " error msg: " << ec.message();
      throw std::system_error(ec);
    }

    gid_index_ = conf.use_best_gid_index ? find_best_gid_index(conf.gid_index)
                                         : conf.gid_index;

    ELOG_INFO << name_ << " Active MTU: " << detail::mtu_str(attr_.active_mtu)
              << ", "
              << "Max MTU: " << detail::mtu_str(attr_.max_mtu)
              << ", gid index: " << gid_index_;

    if (gid_index_ >= 0) {
      if (auto ec = ibv_query_gid(ctx_.get(), conf.port, gid_index_, &gid_);
          ec) {
        auto err_code = std::make_error_code(std::errc{errno});
        ELOG_ERROR << "IBDevice failed to query port " << conf.port
                   << " of device " << name_ << " by gid_index:" << gid_index_
                   << ", error msg: " << err_code.message();
        throw std::system_error(err_code);
      }
      std::error_code err_code;
      gid_address_ =
          asio::ip::make_address(detail::gid_to_string(gid_.raw), err_code);
      if (err_code) {
        ELOG_ERROR << "IBDevice failed to convert gid to ip address of device "
                   << name_ << " by gid_index:" << gid_index_
                   << ", error msg: " << err_code.message();
        throw std::system_error(err_code);
      }
    }
    else {
      ELOG_ERROR << "gid index should greater than zero, now is: "
                 << gid_index_;
      throw std::invalid_argument{
          "gid index should greater than zero, now is: " +
          std::to_string(gid_index_)};
    }
    ibv_device_attr attr;
    auto pool_config = conf.buffer_pool_config;
    if (pool_config.max_memory_usage == 0) {
      pool_config.max_memory_usage = std::numeric_limits<uint64_t>::max();
    }
    if (ibv_query_device(ctx_.get(), &attr) == 0) {
      ELOG_INFO << "max mr size of device:" << attr.max_mr_size
                << ", user config max memory usage:"
                << pool_config.max_memory_usage
                << ". we will use min value as limit";
      pool_config.max_memory_usage =
          std::min<uint64_t>(attr.max_mr_size, pool_config.max_memory_usage);
    }
    else {
      ELOG_WARN
          << "query device info failed! We dont know the max_mr_size of device";
    }
    buffer_pool_ = ib_buffer_pool_t::create(*this, pool_config);

    if (conf.use_srq) {
      srq_enabled_ = true;
      srq_conf_ = ib_srq_buffer_manager_t::config_t{
          .max_buffer_memory = conf.max_srq_buffer_memory,
          .srq_max_wr = conf.srq_max_wr,
          .srq_max_sge = 1,
          .buffer_size = pool_config.buffer_size,
          .buffer_block = conf.srq_buffer_block,
      };
      srq_watermark_ = std::min(
          static_cast<uint32_t>(srq_conf_.max_buffer_memory /
                                srq_conf_.buffer_size),
          srq_conf_.srq_max_wr);
      if (srq_watermark_ == 0) {
        srq_watermark_ = 1;
      }
      srq_idle_timeout_ms_ = conf.srq_idle_timeout_ms;
      try {
        srq_manager_ =
            std::make_shared<ib_srq_buffer_manager_t>(*this, srq_conf_);
      } catch (const std::system_error& e) {
        ELOG_WARN << "SRQ creation failed, falling back to per-QP mode: "
                  << e.what();
        srq_enabled_ = false;
        srq_manager_.reset();
      }
    }
  }

  std::string_view name() const noexcept { return name_; }

  uint16_t port() const noexcept { return port_; }

  ibv_context* context() const noexcept { return ctx_.get(); }
  ibv_pd* pd() const noexcept { return pd_.get(); }
  int gid_index() const noexcept { return gid_index_; }
  const ibv_gid& gid() const noexcept { return gid_; }
  asio::ip::address gid_address() const noexcept { return gid_address_; }
  const ibv_port_attr& attr() const noexcept { return attr_; }

  bool is_support_inline_data() const noexcept { return support_inline_data_; }
  void set_support_inline_data(bool flag) noexcept {
    support_inline_data_ = flag;
  }

  bool is_support_relaxed_ordering() const noexcept {
    return support_relaxed_ordering_.load(std::memory_order_acquire);
  }
  void set_support_relaxed_ordering(bool flag) noexcept {
    support_relaxed_ordering_.store(flag, std::memory_order_release);
  }

  std::shared_ptr<ib_buffer_pool_t> get_buffer_pool() const noexcept {
    return buffer_pool_;
  }

  std::shared_ptr<ib_srq_buffer_manager_t> get_srq_manager() const noexcept {
    return srq_manager_;
  }
  bool use_srq() const noexcept { return srq_enabled_; }
  uint32_t srq_watermark() const noexcept { return srq_watermark_; }

  void ensure_srq() {
    if (!srq_enabled_) return;
    if (srq_manager_) {
      srq_manager_->init();
      return;
    }
    srq_manager_ = std::make_shared<ib_srq_buffer_manager_t>(*this, srq_conf_);
    srq_manager_->init();
  }

  void on_qp_created() {
    if (!srq_enabled_) return;
    active_qp_count_.fetch_add(1, std::memory_order_acq_rel);
  }

  void on_qp_destroyed(coro_io::ExecutorWrapper<>* executor) {
    if (!srq_enabled_) return;
    uint32_t prev =
        active_qp_count_.fetch_sub(1, std::memory_order_acq_rel);
    if (prev == 1) {
      if (!destruction_pending_.exchange(true,
                                          std::memory_order_acq_rel)) {
        srq_destruction_lazy()
            .setLazyLocal(
                async_simple::coro::LazyLocalBase{signal_.get()})
            .directlyStart([](auto&&) {}, executor);
      }
    }
  }

  struct async_event_watcher_manager_t
      : public std::unordered_map<ib_device_t*, std::weak_ptr<ib_device_t>> {
    ~async_event_watcher_manager_t() {
      for (auto& [_, dev] : *this) {
        if (auto ptr = dev.lock(); ptr) {
          ptr->stop_async_event_watcher();
        }
      }
    }
  };

  ~ib_device_t() {
    signal_->emits(async_simple::SignalType::Terminate);
    stop_async_event_watcher();
  }

 private:
  // Address priority rank for GID selection (lower is better)
  enum class gid_addr_rank : int {
    global_unicast = 0,  // Normal routable IPv6 or IPv4-mapped global address
    link_local = 1,      // fe80::/10 or ::ffff:169.254.x.x
    loopback = 2,        // ::1 or ::ffff:127.x.x.x
  };

  // Device type priority (lower is better)
  static int gid_type_priority(uint32_t gid_type) {
    switch (gid_type) {
      case IBV_GID_TYPE_IB:
        return 0;
      case IBV_GID_TYPE_ROCE_V2:
        return 1;
      case IBV_GID_TYPE_ROCE_V1:
        return 2;
      default:
        return 3;
    }
  }

  static gid_addr_rank classify_gid_address(const union ibv_gid& gid) {
    // Check IPv4-mapped address (::ffff:x.x.x.x)
    bool is_v4_mapped =
        (gid.raw[0] == 0 && gid.raw[1] == 0 && gid.raw[2] == 0 &&
         gid.raw[3] == 0 && gid.raw[4] == 0 && gid.raw[5] == 0 &&
         gid.raw[6] == 0 && gid.raw[7] == 0 && gid.raw[8] == 0 &&
         gid.raw[9] == 0 && gid.raw[10] == 0xff && gid.raw[11] == 0xff);

    if (is_v4_mapped) {
      // ::ffff:127.x.x.x → loopback
      if (gid.raw[12] == 127) {
        return gid_addr_rank::loopback;
      }
      // ::ffff:169.254.x.x → link-local
      if (gid.raw[12] == 169 && gid.raw[13] == 254) {
        return gid_addr_rank::link_local;
      }
      return gid_addr_rank::global_unicast;
    }

    // IPv6 loopback ::1
    static constexpr uint8_t ipv6_loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 1};
    if (memcmp(gid.raw, ipv6_loopback, 16) == 0) {
      return gid_addr_rank::loopback;
    }

    // fe80::/10 → link-local
    if ((gid.raw[0] == 0xfe) && ((gid.raw[1] & 0xc0) == 0x80)) {
      return gid_addr_rank::link_local;
    }

    return gid_addr_rank::global_unicast;
  }

 public:
  // Select best GID from a list of entries. Returns gid_index or -1 if empty.
  // Exposed as public static for unit testing.
  static int select_best_gid(const std::vector<ibv_gid_entry>& entries) {
    if (entries.empty()) {
      return -1;
    }

    int best_index = -1;
    int best_type_prio = 4;
    gid_addr_rank best_addr_rank = gid_addr_rank::loopback;

    for (auto& entry : entries) {
      int type_prio = gid_type_priority(entry.gid_type);
      gid_addr_rank addr_rank = classify_gid_address(entry.gid);

      // Compare: type priority first, then address rank
      if (type_prio < best_type_prio ||
          (type_prio == best_type_prio && addr_rank < best_addr_rank)) {
        best_type_prio = type_prio;
        best_addr_rank = addr_rank;
        best_index = static_cast<int>(entry.gid_index);
      }
    }

    return best_index;
  }

  int find_best_gid_index(int default_gid_index) {
#ifndef YLT_IBVERBS_DONT_SUPPORT_FIND_GID_INDEX
    std::vector<ibv_gid_entry> valid_entries;
    ibv_gid_entry gid_entry;

    for (int i = 0; i < attr_.gid_tbl_len; i++) {
      if (ibv_query_gid_ex(ctx_.get(), port_, i, &gid_entry, 0)) {
        continue;
      }
      // For RoCE types, skip entries without an associated net device
      if (gid_entry.gid_type != IBV_GID_TYPE_IB &&
          gid_entry.ndev_ifindex == 0) {
        continue;
      }
      valid_entries.push_back(gid_entry);
    }

    // Debug: print all valid GID entries
    ELOG_INFO << this->name_ << " GID table has " << valid_entries.size()
              << " valid entries (total table size: " << attr_.gid_tbl_len
              << ")";
    for (auto& entry : valid_entries) {
      char gid_str[INET6_ADDRSTRLEN] = {};
      inet_ntop(AF_INET6, entry.gid.raw, gid_str, sizeof(gid_str));
      const char* type_str = entry.gid_type == IBV_GID_TYPE_IB        ? "IB"
                             : entry.gid_type == IBV_GID_TYPE_ROCE_V2 ? "RoCEv2"
                             : entry.gid_type == IBV_GID_TYPE_ROCE_V1
                                 ? "RoCEv1"
                                 : "Unknown";
      ELOG_INFO << "  gid[" << entry.gid_index << "] type=" << type_str
                << " ndev_ifindex=" << entry.ndev_ifindex
                << " addr=" << gid_str;
    }

    int best = select_best_gid(valid_entries);
    if (best >= 0) {
      ELOG_INFO << "Selected best GID index: " << best;
      return best;
    }
#endif
    ELOG_WARN << "select best GID failed, maybe the platform doesn't "
                 "support it. return default gid index: "
              << default_gid_index;
    return default_gid_index;
  }

  template <typename T>
  static bool weak_ptrs_equal(const std::weak_ptr<T>& a,
                              const std::weak_ptr<T>& b) {
    return !a.owner_before(b) && !b.owner_before(a);
  }

  void start_async_event_watcher() {
    if (!ctx_) {
      return;
    }
    int flags = fcntl(ctx_->async_fd, F_GETFL);
    int ret = fcntl(ctx_->async_fd, F_SETFL, flags | O_NONBLOCK);
    if (ret < 0) {
      ELOG_ERROR
          << "Error, failed to change file descriptor of async event queue\n";
      return;
    }
    auto executor = coro_io::get_global_executor();
    async_event_watcher_fd_ = std::unique_ptr<asio::posix::stream_descriptor>(
        new asio::posix::stream_descriptor(executor->get_asio_executor(),
                                           ctx_->async_fd));
    auto listen_event = [](std::weak_ptr<ib_device_t> dev,
                           coro_io::ExecutorWrapper<>* executor)
        -> async_simple::coro::Lazy<void> {
      std::error_code ec;
      auto self = dev.lock();
      if (self == nullptr) {
        co_return;
      }
      auto self_raw_ptr = self.get();
      (*executor->get_data_with_default<async_event_watcher_manager_t>(
          "ylt_ib_watch"))[self_raw_ptr] = dev;
      auto name = std::string{self->name()};
      ELOG_INFO << "start_async_event_watcher of device:" << name << " start";
      while (!ec) {
        coro_io::callback_awaitor<std::error_code> awaitor;
        ec = co_await awaitor.await_resume([&self](auto handler) {
          self->async_event_watcher_fd_->async_wait(
              asio::posix::stream_descriptor::wait_read,
              [handler](const std::error_code& ec) mutable {
                handler.set_value_then_resume(ec);
              });
          self = nullptr;
        });
        if (!ec) {
          self = dev.lock();
          if (!self) {
            ELOG_DEBUG
                << "ib_device async event stop listening by close device:"
                << name;
            break;
          }
          self->poll_async_events();
        }
        else {
          ELOG_DEBUG << "ib_device async event stop listening by error:"
                     << ec.message() << ",device:" << name;
        }
      }
      auto* table = executor->get_data<async_event_watcher_manager_t>(
          "ib_device_async_event_watcher");
      if (table) {
        auto iter = table->find(self_raw_ptr);
        if (iter != table->end() && weak_ptrs_equal(iter->second, dev)) {
          table->erase(iter);
        }
      }
    };
    listen_event(shared_from_this(), executor).via(executor).detach();
  }

  void stop_async_event_watcher() {
    std::error_code ec;
    [[maybe_unused]] auto _ = async_event_watcher_fd_->cancel(ec);
    async_event_watcher_fd_->release();
  }

  async_simple::coro::Lazy<void> srq_destruction_lazy() {
    auto result = co_await coro_io::sleep_for(
        std::chrono::milliseconds(srq_idle_timeout_ms_));
    destruction_pending_.store(false, std::memory_order_release);
    if (!result) {
      co_return;
    }
    if (active_qp_count_.load(std::memory_order_acquire) == 0) {
      ELOG_INFO << "SRQ idle timeout (" << srq_idle_timeout_ms_
                << "ms), destroying SRQ manager";
      srq_manager_.reset();
    }
    co_return;
  }

  std::string name_;
  std::unique_ptr<ibv_context, ib_deleter> ctx_;
  std::unique_ptr<ibv_pd, ib_deleter> pd_;
  std::unique_ptr<asio::posix::stream_descriptor> async_event_watcher_fd_;
  std::shared_ptr<ib_buffer_pool_t> buffer_pool_;
  std::shared_ptr<ib_srq_buffer_manager_t> srq_manager_;
  std::shared_ptr<async_simple::Signal> signal_ =
      async_simple::Signal::create();
  ib_srq_buffer_manager_t::config_t srq_conf_;
  std::atomic<uint32_t> active_qp_count_{0};
  std::atomic<bool> destruction_pending_{false};
  bool srq_enabled_ = false;
  uint32_t srq_watermark_ = 0;
  uint32_t srq_idle_timeout_ms_ = 3000;
  std::atomic<bool> support_inline_data_ = true;
  std::atomic<bool> support_relaxed_ordering_ = true;
  ibv_port_attr attr_;
  ibv_gid gid_;
  asio::ip::address gid_address_;
  int gid_index_;

  uint16_t port_;
};

inline std::unique_ptr<ibv_mr, ib_deleter> ib_buffer_t::regist(ib_device_t& dev,
                                                               void* ptr,
                                                               uint32_t size,
                                                               int ib_flags) {
  auto mr = ibv_reg_mr(dev.pd(), ptr, size, ib_flags);
  ELOG_TRACE << "ibv_reg_mr regist: " << mr << " with pd:" << dev.pd();
  if (mr != nullptr) [[likely]] {
    ELOG_TRACE << "regist sge.lkey: " << mr->lkey << ", sge.addr: " << mr->addr
               << ", sge.length: " << mr->length;
    return std::unique_ptr<ibv_mr, ib_deleter>{mr};
  }
  else {
    ELOG_WARN << "regist memory failed! "
              << std::make_error_code(std::errc{errno}).message();
    return nullptr;
  }
};

inline ib_buffer_t ib_buffer_t::regist(ib_buffer_pool_t& pool,
                                       memory_owner_t data, std::size_t size,
                                       int ib_flags) {
  auto mr = ibv_reg_mr(pool.device_.pd(), data.get(), size, ib_flags);
  if (mr != nullptr) [[likely]] {
    ELOG_DEBUG << "ibv_reg_mr regist: " << mr
               << " with pd:" << pool.device_.pd();
    pool.modify_memory_usage(size);
    return ib_buffer_t{std::unique_ptr<ibv_mr, ib_deleter>{mr}, std::move(data),
                       pool};
  }
  else {
    std::error_code ec = std::make_error_code(std::errc{errno});
    ELOG_WARN << "allocate ibverbs memory region failed! " << ec.message();
    return ib_buffer_t{};
  }
};

inline ib_srq_buffer_manager_t::ib_srq_buffer_manager_t(ib_device_t& device,
                                                         const config_t& conf)
    : device_(device), config_(conf) {
  watermark_ = std::min(
      static_cast<uint32_t>(conf.max_buffer_memory / conf.buffer_size),
      conf.srq_max_wr);
  if (watermark_ == 0) {
    watermark_ = 1;
  }
  buffer_block_ = (conf.buffer_block == 0) ? watermark_ : conf.buffer_block;
  if (buffer_block_ > watermark_) {
    buffer_block_ = watermark_;
  }
  ibv_srq_init_attr srq_init_attr{};
  srq_init_attr.attr.max_wr = conf.srq_max_wr;
  srq_init_attr.attr.max_sge = conf.srq_max_sge;
  srq_init_attr.srq_context = nullptr;
  srq_.reset(ibv_create_srq(device_.pd(), &srq_init_attr));
  if (!srq_) {
    auto err_code = std::make_error_code(std::errc{errno});
    ELOG_ERROR << "ibv_create_srq failed: " << err_code.message();
    throw std::system_error(err_code);
  }
  ELOG_INFO << "SRQ created: watermark=" << watermark_
            << ", buffer_block=" << buffer_block_
            << ", max_wr=" << conf.srq_max_wr
            << ", max_buffer_memory=" << conf.max_buffer_memory;
}

inline void ib_srq_buffer_manager_t::init() {
  bool expected = false;
  if (!initialized_.compare_exchange_strong(expected, true,
                                             std::memory_order_acq_rel)) {
    return;
  }
  int gpu_id = device_.get_buffer_pool()->get_config().gpu_id;
  uint32_t slot_capacity = watermark_ * 2;
  slots_.reserve(slot_capacity);
  for (uint32_t i = 0; i < slot_capacity; ++i) {
    slots_.push_back(std::make_unique<ib_srq_recv_entry_t>());
  }
  for (uint32_t i = 0; i < buffer_block_; ++i) {
    auto buffer = device_.get_buffer_pool()->get_buffer(gpu_id);
    if (!buffer) {
      ELOG_WARN << "SRQ init: pool exhausted at " << i << "/" << buffer_block_;
      break;
    }
    auto* entry = slots_[i].get();
    entry->buffer = std::move(buffer);
    entry->active.store(true, std::memory_order_release);
    if (!post_entry(entry)) {
      ELOG_WARN << "SRQ init: post_entry failed at " << i << "/"
                << buffer_block_;
      entry->active.store(false, std::memory_order_release);
      entry->buffer = {};
      break;
    }
    posted_count_.fetch_add(1, std::memory_order_release);
    total_count_.fetch_add(1, std::memory_order_release);
  }
  scan_pos_.store(buffer_block_, std::memory_order_release);
  ELOG_INFO << "SRQ init done: posted=" << posted_count_.load()
            << ", total=" << total_count_.load()
            << ", memory_usage=" << memory_usage();
}

inline bool ib_srq_buffer_manager_t::post_entry(ib_srq_recv_entry_t* entry) {
  ibv_sge sge = entry->buffer.subview();
  ibv_recv_wr wr{};
  wr.wr_id = reinterpret_cast<uintptr_t>(entry);
  wr.sg_list = &sge;
  wr.num_sge = 1;
  wr.next = nullptr;
  ibv_recv_wr* bad_wr = nullptr;
  if (ibv_post_srq_recv(srq_.get(), &wr, &bad_wr)) {
    auto err_code = std::make_error_code(std::errc{errno});
    ELOG_ERROR << "ibv_post_srq_recv failed: " << err_code.message();
    return false;
  }
  return true;
}

inline ib_srq_recv_entry_t* ib_srq_buffer_manager_t::find_free_slot() {
  uint32_t pos = scan_pos_.load(std::memory_order_acquire);
  uint32_t start = pos;
  do {
    auto& slot = slots_[pos];
    bool expected = false;
    if (slot->active.compare_exchange_strong(expected, true,
                                              std::memory_order_acq_rel)) {
      scan_pos_.store((pos + 1) % slots_.size(), std::memory_order_release);
      return slot.get();
    }
    pos = (pos + 1) % slots_.size();
  } while (pos != start);
  return nullptr;
}

inline void ib_srq_buffer_manager_t::expand(uint32_t count) {
  int gpu_id = device_.get_buffer_pool()->get_config().gpu_id;
  for (uint32_t i = 0; i < count; ++i) {
    uint32_t expected = total_count_.load(std::memory_order_acquire);
    while (expected < watermark_) {
      if (total_count_.compare_exchange_weak(expected, expected + 1,
                                             std::memory_order_acq_rel)) {
        break;
      }
    }
    if (expected >= watermark_) {
      break;
    }
    auto* entry = find_free_slot();
    if (!entry) {
      total_count_.fetch_sub(1, std::memory_order_acq_rel);
      break;
    }
    auto buffer = device_.get_buffer_pool()->get_buffer(gpu_id);
    if (!buffer) {
      entry->active.store(false, std::memory_order_release);
      total_count_.fetch_sub(1, std::memory_order_acq_rel);
      break;
    }
    entry->buffer = std::move(buffer);
    if (post_entry(entry)) {
      posted_count_.fetch_add(1, std::memory_order_acq_rel);
    }
    else {
      entry->active.store(false, std::memory_order_release);
      entry->buffer = {};
      total_count_.fetch_sub(1, std::memory_order_acq_rel);
      break;
    }
  }
}

inline void ib_srq_buffer_manager_t::replenish(
    ib_srq_recv_entry_t* entry, ib_buffer_t consumed_buffer) {
  uint32_t posted = posted_count_.load(std::memory_order_acquire);
  uint32_t total = total_count_.load(std::memory_order_acquire);

  if (posted > total * 2 / 3) {
    uint32_t expected = total;
    while (expected > buffer_block_) {
      if (total_count_.compare_exchange_weak(expected, expected - 1,
                                             std::memory_order_acq_rel)) {
        entry->active.store(false, std::memory_order_release);
        return;
      }
    }
  }

  entry->buffer = std::move(consumed_buffer);
  while (posted < watermark_) {
    if (posted_count_.compare_exchange_weak(posted, posted + 1,
                                            std::memory_order_acq_rel)) {
      if (post_entry(entry)) {
        break;
      }
      posted_count_.fetch_sub(1, std::memory_order_release);
      break;
    }
  }

  posted = posted_count_.load(std::memory_order_acquire);
  total = total_count_.load(std::memory_order_acquire);
  if (posted < total / 3 && total < watermark_) {
    uint32_t expand_count = std::min(total / 3, watermark_ - total);
    if (expand_count > 0) {
      expand(expand_count);
    }
  }
}

inline void ib_srq_buffer_manager_t::repost(ib_srq_recv_entry_t* entry) {
  if (post_entry(entry)) {
    posted_count_.fetch_add(1, std::memory_order_release);
  }
}

class ib_device_manager_t {
  ib_device_manager_t(const ib_device_manager_t&) = delete;
  ib_device_manager_t& operator=(const ib_device_manager_t&) = delete;
  std::unordered_map<std::string, std::shared_ptr<ib_device_t>> device_map_;
  std::shared_ptr<ib_device_t> default_device_;

 public:
  std::unordered_map<std::string, std::shared_ptr<ib_device_t>>&
  get_dev_list() {
    return device_map_;
  }
  ib_device_manager_t(const ib_device_t::config_t& conf = {}) {
    auto& devices = detail::ib_devices_t::instance();
    for (auto& native_dev : devices.get_devices()) {
      try {
        auto dev = ib_device_t::create(conf, native_dev);
        auto [iter, is_ok] = device_map_.emplace(dev->name(), dev);
        if (is_ok && default_device_ == nullptr) {
          default_device_ = iter->second;
        }
      } catch (const std::system_error& error) {
        // discard the exception if ib_device_t construct failed
      }
    }
    if (!default_device_) {
      ELOG_ERROR << "get rdma device failed, there is no available device";
    }
  }
  std::shared_ptr<ib_device_t> find_device(std::string name) {
    if (!default_device_) [[unlikely]] {
      ELOG_ERROR << "get rdma device failed, there is no available device";
      return nullptr;
    }
    if (name.empty()) {
      return default_device_;
    }
    else {
      auto iter = device_map_.find(name);
      if (iter != device_map_.end()) {
        return iter->second;
      }
      else {
        ELOG_ERROR << "get device failed, cant found device:" << name;
        return nullptr;
      }
    }
  }
};

inline std::shared_ptr<ib_device_manager_t> g_ib_device_manager(
    const ib_device_t::config_t& conf = {}) {
  static auto dev_map = std::make_shared<ib_device_manager_t>(conf);
  return dev_map;
}

inline std::shared_ptr<ib_device_t> get_global_ib_device(
    const ib_device_t::config_t& conf = {}) {
  static auto dev_map = g_ib_device_manager(conf);
  return dev_map->find_device(conf.dev_name);
}

}  // namespace coro_io
