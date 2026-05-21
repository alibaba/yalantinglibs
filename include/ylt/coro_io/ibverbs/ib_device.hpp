#pragma once
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <netinet/in.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

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
 
 static int get_device_numa_node(std::string_view ibdev_name) {
  if (ibdev_name.empty()) return -1;
  auto path = "/sys/class/infiniband/"+std::string{ibdev_name}+"/device/numa_node";
 
  FILE *f = fopen(path.c_str(), "r");
  if (!f) {
      // 可能设备不存在，或非 PCI 设备
      return -1;
  }

  int numa_node = -1;
  if (fscanf(f, "%d", &numa_node) != 1) {
      fclose(f);
      return -1;
  }
  fclose(f);

  // 内核可能返回 -1（表示无效或未绑定）
  return (numa_node >= 0) ? numa_node : -1;
}
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
    auto buffer_config = conf.buffer_pool_config;
    if (buffer_config.numa_id == -1) {
      buffer_config.numa_id = get_device_numa_node(name_);
      ELOG_INFO << "IBDevice " << name_ << " at numa node: " << buffer_config.numa_id;
    }
    numa_id_ = buffer_config.numa_id;
    buffer_pool_ = ib_buffer_pool_t::create(*this, buffer_config);
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

  int get_numa_id() const noexcept {
    return numa_id_;
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

  void find_best_gid_index() {
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
    ELOG_DEBUG << this->name_ << " GID table has " << valid_entries.size()
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
      ELOG_DEBUG << "  gid[" << entry.gid_index << "] type=" << type_str
                 << " ndev_ifindex=" << entry.ndev_ifindex
                 << " addr=" << gid_str;
    }

    int best = select_best_gid(valid_entries);
    if (best >= 0) {
      ELOG_DEBUG << "Selected best GID index: " << best;
      gid_index_ = best;
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
  int numa_id_ = -1;

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
                                       ib_buffer_block data,
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