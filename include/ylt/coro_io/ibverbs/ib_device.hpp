#pragma once
#include <infiniband/verbs.h>
#include <netinet/in.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

#include "asio/ip/address.hpp"
#include "ylt/easylog.hpp"

namespace coro_io {
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

struct ib_config_t {
  std::string dev_name;
  uint16_t port = 1;
};

struct ib_deleter {
  void operator()(ibv_pd* pd) const noexcept {
    if (pd) {
      auto ret = ibv_dealloc_pd(pd);
      if (ret != 0) {
        ELOG_ERROR << "ibv_dealloc_pd failed: "
                   << std::make_error_code(std::errc{ret}).message();
      }
    }
  }
  void operator()(ibv_context* context) const noexcept {
    if (context) {
      auto ret = ibv_close_device(context);
      if (ret != 0) {
        ELOG_ERROR << "ibv_close_device failed "
                   << std::make_error_code(std::errc{ret}).message();
      }
    }
  }
  void operator()(ibv_cq* cq) const noexcept {
    if (cq) {
      auto ret = ibv_destroy_cq(cq);
      if (ret != 0) {
        ELOG_ERROR << "ibv_destroy_cq failed "
                   << std::make_error_code(std::errc{ret}).message();
      }
    }
  }
  void operator()(ibv_qp* qp) const noexcept {
    if (qp) {
      auto ret = ibv_destroy_qp(qp);
      if (ret != 0) {
        ELOG_ERROR << "ibv_destroy_qp failed "
                   << std::make_error_code(std::errc{ret}).message();
      }
    }
  }
  void operator()(ibv_comp_channel* channel) const noexcept {
    if (channel) {
      auto ret = ibv_destroy_comp_channel(channel);
      if (ret != 0) {
        ELOG_ERROR << "ibv_destroy_comp_channel failed "
                   << std::make_error_code(std::errc{ret}).message();
      }
    }
  }
  void operator()(ibv_mr* ptr) const noexcept {
    if (ptr) {
      if (auto ret = ibv_dereg_mr(ptr); ret) [[unlikely]] {
        ELOG_ERROR << "ibv_dereg_mr failed: "
                   << std::make_error_code(std::errc{ret}).message();
      }
    }
  }
};

namespace detail {
inline std::string gid_to_string(uint8_t (&a)[16]) noexcept {
  std::string ret;
  ret.resize(40);
  sprintf(ret.data(),
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
}  // namespace detail
class ib_device_t {
 public:
  ib_device_t(const ib_config_t& conf) {
    auto& inst = ib_devices_t::instance();
    port_ = conf.port;
    auto dev = inst.at(conf.dev_name);
    name_ = ibv_get_device_name(dev);
    ctx_.reset(ibv_open_device(dev));
    pd_.reset(ibv_alloc_pd(ctx_.get()));

    if (auto ret = ibv_query_port(ctx_.get(), conf.port, &attr_); ret != 0) {
      auto ec = std::make_error_code((std::errc)ret);
      ELOG_ERROR << "IBDevice failed to query port " << conf.port
                 << " of device " << name_ << " error msg: " << ec.message();
      throw std::system_error(ec);
    }

    ELOG_TRACE << name_ << " Active MTU: " << detail::mtu_str(attr_.active_mtu)
               << ", "
               << "Max MTU: " << detail::mtu_str(attr_.max_mtu);

    find_best_gid_index();

    if (gid_index_ >= 0) {
      if (auto ec = ibv_query_gid(ctx_.get(), conf.port, gid_index_, &gid_);
          ec) {
        auto err_code = std::make_error_code(std::errc{ec});
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

 private:
  int ipv6_addr_v4mapped(const struct in6_addr* a) {
    return ((a->s6_addr32[0] | a->s6_addr32[1]) |
            (a->s6_addr32[2] ^ htonl(0x0000ffff))) == 0UL ||
           /* IPv4 encoded multicast addresses */
           (a->s6_addr32[0] == htonl(0xff0e0000) &&
            ((a->s6_addr32[1] | (a->s6_addr32[2] ^ htonl(0x0000ffff))) == 0UL));
  }

  void find_best_gid_index() {
    ibv_gid_entry gid_entry;

    for (int i = 0; i < attr_.gid_tbl_len; i++) {
      if (auto ret = ibv_query_gid_ex(ctx_.get(), port_, i, &gid_entry, 0)) {
        auto ec = std::make_error_code((std::errc)ret);
        ELOG_ERROR << "IBDevice failed to query gid ex " << port_
                   << " of device " << name_ << " error msg: " << ec.message();
        continue;
      }

      if ((ipv6_addr_v4mapped((struct in6_addr*)gid_entry.gid.raw) &&
           gid_entry.gid_type == IBV_GID_TYPE_ROCE_V2) ||
          gid_entry.gid_type == IBV_GID_TYPE_IB) {
        gid_index_ = i;
        break;
      }
    }
  }

  std::string name_;
  std::unique_ptr<ibv_pd, ib_deleter> pd_;
  std::unique_ptr<ibv_context, ib_deleter> ctx_;
  std::atomic<bool> support_inline_data_ = true;
  ibv_port_attr attr_;
  ibv_gid gid_;
  asio::ip::address gid_address_;
  int gid_index_;

  uint16_t port_;
};

}  // namespace coro_io