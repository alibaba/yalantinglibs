#pragma once
#include <infiniband/verbs.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

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
}  // namespace ylt::coro_rdma