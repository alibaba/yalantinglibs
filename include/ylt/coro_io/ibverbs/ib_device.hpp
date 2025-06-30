#pragma once
#include <infiniband/verbs.h>
#include <netinet/in.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>

#include "asio/ip/address.hpp"
#include "ib_buffer.hpp"
#include "ylt/easylog.hpp"

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
  friend class ib_device_manager_t;
  struct config_t {
    std::string dev_name;
    uint16_t port = 1;
    ib_buffer_pool_t::config_t buffer_pool_config;
  };
  static std::shared_ptr<ib_device_t> create(const config_t& conf) {
    return std::make_shared<ib_device_t>(private_construct_token{}, conf);
  }

 private:
  struct private_construct_token {};

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

    find_best_gid_index();

    ELOG_INFO << name_ << " Active MTU: " << detail::mtu_str(attr_.active_mtu)
              << ", "
              << "Max MTU: " << detail::mtu_str(attr_.max_mtu)
              << ", best gid index: " << gid_index_;

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
    buffer_pool_ = ib_buffer_pool_t::create(*this, conf.buffer_pool_config);
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

  std::shared_ptr<ib_buffer_pool_t> get_buffer_pool() const noexcept {
    return buffer_pool_;
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
  std::unique_ptr<ibv_context, ib_deleter> ctx_;
  std::unique_ptr<ibv_pd, ib_deleter> pd_;
  std::shared_ptr<ib_buffer_pool_t> buffer_pool_;
  std::atomic<bool> support_inline_data_ = true;
  ibv_port_attr attr_;
  ibv_gid gid_;
  asio::ip::address gid_address_;
  int gid_index_;

  uint16_t port_;
};

inline std::unique_ptr<ibv_mr> ib_buffer_t::regist(ib_device_t& dev, void* ptr,
                                                   uint32_t size,
                                                   int ib_flags) {
  auto mr = ibv_reg_mr(dev.pd(), ptr, size, ib_flags);
  ELOG_TRACE << "ibv_reg_mr regist: " << mr << " with pd:" << dev.pd();
  if (mr != nullptr) [[unlikely]] {
    ELOG_TRACE << "regist sge.lkey: " << mr->lkey << ", sge.addr: " << mr->addr
               << ", sge.length: " << mr->length;
    return std::unique_ptr<ibv_mr>{mr};
  }
  else {
    throw std::make_error_code(std::errc{errno});
  }
};

inline ib_buffer_t ib_buffer_t::regist(ib_buffer_pool_t& pool,
                                       std::unique_ptr<char[]> data,
                                       std::size_t size, int ib_flags) {
  auto mr = ibv_reg_mr(pool.device_.pd(), data.get(), size, ib_flags);
  if (mr != nullptr) [[unlikely]] {
    ELOG_DEBUG << "ibv_reg_mr regist: " << mr
               << " with pd:" << pool.device_.pd();
    pool.modify_memory_usage(size);
    return ib_buffer_t{std::unique_ptr<ibv_mr, ib_deleter>{mr}, std::move(data),
                       pool};
  }
  else {
    throw std::make_error_code(std::errc{errno});
  }
};
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
        auto dev = std::make_shared<ib_device_t>(
            ib_device_t::private_construct_token{}, conf, native_dev);
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