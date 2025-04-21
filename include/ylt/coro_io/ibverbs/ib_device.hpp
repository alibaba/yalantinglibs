#pragma once
#include <infiniband/verbs.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

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
  int gid_index = 1;
  std::string dev_name;
  uint16_t port = 1;
};

struct ib_deleter {
  void operator()(ibv_pd* pd) const noexcept{
    auto ret = ibv_dealloc_pd(pd);
    if (ret != 0) {
      ELOG_ERROR << "ibv_dealloc_pd failed: " << std::make_error_code(std::errc{ret}).message();
    }
  }
  void operator()(ibv_context* context) const noexcept{
    auto ret = ibv_close_device(context);
    if (ret != 0) {
      ELOG_ERROR << "ibv_close_device failed " << std::make_error_code(std::errc{ret}).message();
    }
  }
  void operator()(ibv_cq* cq) const noexcept{
    auto ret = ibv_destroy_cq(cq);
    if (ret != 0) {
      ELOG_ERROR << "ibv_destroy_cq failed " << std::make_error_code(std::errc{ret}).message();
    }
  }
  void operator()(ibv_qp* qp) const noexcept{
    auto ret = ibv_destroy_qp(qp);
    if (ret != 0) {
      ELOG_ERROR << "ibv_destroy_qp failed " << std::make_error_code(std::errc{ret}).message();
    }
  }
  void operator()(ibv_comp_channel* channel) const noexcept{
    auto ret = ibv_destroy_comp_channel(channel);
    if (ret != 0) {
      ELOG_ERROR << "ibv_destroy_comp_channel failed " << std::make_error_code(std::errc{ret}).message();
    }
  }
  void operator()(ibv_mr* ptr) const noexcept{
    if (auto ret = ibv_dereg_mr(ptr); ret)
        [[unlikely]] {
      ELOG_ERROR << "ibv_destroy_comp_channel failed: " << std::make_error_code(std::errc{ret}).message();
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
      auto ec = std::make_error_code((std::errc)ret);
      ELOG_ERROR << "IBDevice failed to query port " << conf.port
                 << " of device " << name_ << " error msg: " << ec.message();
      throw std::system_error(ec);
    }
    if (gid_index_ >= 0) {
      if (auto ec = ibv_query_gid(ctx_.get(), conf.port, gid_index_,
                          &gid_);ec) {
        auto err_code = std::make_error_code(std::errc{ec});  
        ELOG_ERROR << "IBDevice failed to query gid " << conf.port
              << " of device " << name_ << "by gid_index:"<<gid_index_ <<", error msg: " << err_code.message();
        throw std::system_error(err_code);          
      }
    }
    else {
      ELOG_ERROR << "gid index should greater than zero, now is: " << gid_index_;
      throw std::invalid_argument{"gid index should greater than zero, now is: " + std::to_string(gid_index_)};
    }
  }

  std::string_view name() const noexcept { return name_; }

  uint16_t port() const noexcept { return port_; }

  ibv_context* context() const noexcept { return ctx_.get(); }
  ibv_pd* pd() const noexcept { return pd_.get(); }
  int gid_index() const noexcept { return gid_index_; }
  const ibv_gid& gid() const noexcept {return gid_;}
  const ibv_port_attr& attr() const noexcept { return attr_; }

 private:
  std::string name_;
  std::unique_ptr<ibv_context, ib_deleter> ctx_;
  std::unique_ptr<ibv_pd, ib_deleter> pd_;
  ibv_port_attr attr_;
  ibv_gid gid_;
  ibv_device* device_;
  int gid_index_;

  uint16_t port_;
};


}  // namespace ylt::coro_rdma