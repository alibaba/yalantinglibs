#pragma once
#include <infiniband/verbs.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ylt/easylog.hpp"

namespace ylt::coro_rdma {
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

  const std::vector<std::string>& dev_names() { return dev_names_; }

  ibv_device* at(const std::string& dev_name) {
    if (dev_name.empty()) {
      return (*this)[0];
    }

    auto it = std::find(dev_names_.begin(), dev_names_.end(), dev_name);
    size_t i = std::distance(dev_names_.begin(), it);
    return (*this)[i];
  }

 private:
  ib_devices_t() {
    dev_list_ = ibv_get_device_list(&size_);
    if (dev_list_ == nullptr) {
      size_ = 0;
    }
    else {
      for (int i = 0; i < size_; i++) {
        ELOG_INFO << "device name " << dev_list_[i]->name;
        dev_names_.push_back(dev_list_[i]->name);
      }
      std::sort(dev_names_.begin(), dev_names_.end());
    }
  }
  ~ib_devices_t() {
    if (dev_list_ != nullptr) {
      ibv_free_device_list(dev_list_);
    }
  }

  int size_;
  ibv_device** dev_list_;
  std::vector<std::string> dev_names_;
};

struct ib_config_t {
  int gid_index = 1;
  std::string dev_name;
  uint16_t port = 1;
};

struct ib_deleter {
  void operator()(ibv_pd* pd) const {
    auto ret = ibv_dealloc_pd(pd);
    if (ret != 0) {
      ELOG_ERROR << "ibv_dealloc_pd failed " << ret;
    }
  }
  void operator()(ibv_context* context) const {
    auto ret = ibv_close_device(context);
    if (ret != 0) {
      ELOG_ERROR << "ibv_close_device failed " << ret;
    }
  }
  void operator()(ibv_cq* cq) const {
    auto ret = ibv_destroy_cq(cq);
    if (ret != 0) {
      ELOG_ERROR << "ibv_destroy_cq failed " << ret;
    }
  }
  void operator()(ibv_qp* qp) const {
    auto ret = ibv_destroy_qp(qp);
    if (ret != 0) {
      ELOG_ERROR << "ibv_destroy_qp failed " << ret;
    }
  }
  void operator()(ibv_comp_channel* channel) const {
    auto ret = ibv_destroy_comp_channel(channel);
    if (ret != 0) {
      ELOG_ERROR << "ibv_destroy_comp_channel failed " << ret;
    }
  }
};

class ib_device_t {
 public:
  ib_device_t(const ib_config_t& conf) {
    auto& inst = ib_devices_t::instance();
    gid_index_ = conf.gid_index;
    port_ = conf.port;
    auto dev = inst.at(conf.dev_name);
    name_ = ibv_get_device_name(dev);
    ctx_.reset(ibv_open_device(dev));
    pd_.reset(ibv_alloc_pd(ctx_.get()));

    if (auto ret = ibv_query_port(ctx_.get(), conf.port, &attr_); ret != 0) {
      ELOG_ERROR << "IBDevice failed to query port " << conf.port
                 << " of device " << name_ << " error " << ret;
      throw std::domain_error("IBDevice failed to query");
    }
  }

  std::string_view name() const { return name_; }

  uint16_t port() const { return port_; }

  ibv_context* context() const { return ctx_.get(); }
  ibv_pd* pd() const { return pd_.get(); }
  int gid_index() const { return gid_index_; }
  const ibv_port_attr& attr() const { return attr_; }

  ibv_mr* reg_memory(void* addr, size_t length,
                     int access = IBV_ACCESS_LOCAL_WRITE |
                                  IBV_ACCESS_REMOTE_READ |
                                  IBV_ACCESS_REMOTE_WRITE) const {
    ELOG_INFO << "begin to reg_mr";
    auto* mr = ibv_reg_mr(pd_.get(), addr, length, access);
    if (mr == nullptr) {
      ELOG_ERROR << "IBDevice " << name_ << ", failed to reg_mr, addr " << addr
                 << ", length " << length << ", access " << access << ", error "
                 << errno;
      throw std::domain_error("IBDevice failed to reg_mr");
    }
    ELOG_INFO << "end reg_mr";
    return mr;
  }

  void dereg_memory(ibv_mr* mr) const {
    auto ret = ibv_dereg_mr(mr);
    if (ret) {
      ELOG_ERROR << "IBDevice " << name_ << ", failed to dereg_mr error "
                 << ret;
      throw std::domain_error("IBDevice failed to dereg_mr");
    }
  }

 private:
  std::string name_;
  std::unique_ptr<ibv_context, ib_deleter> ctx_;
  std::unique_ptr<ibv_pd, ib_deleter> pd_;
  ibv_port_attr attr_;
  ibv_device* device_;
  int gid_index_;
  uint16_t port_;
};
}  // namespace ylt::coro_rdma