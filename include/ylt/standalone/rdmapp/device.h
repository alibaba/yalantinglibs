#pragma once

#include <infiniband/verbs.h>

#include <cassert>
#include <cstdint>
#include <iterator>
#include <string>

#include "error.h"

namespace rdmapp {

/**
 * @brief This class holds a list of devices available on the system.
 *
 */
class device_list {
  struct ibv_device **devices_;
  size_t nr_devices_;

 public:
  class iterator {
    friend class device_list;
    size_t i_;
    struct ibv_device **devices_;
    iterator(struct ibv_device **devices, size_t i)
        : i_(i), devices_(devices) {}

   public:
    // iterator traits
    using difference_type = long;
    using value_type = long;
    using pointer = const long *;
    using reference = const long &;
    using iterator_category = std::forward_iterator_tag;

    struct ibv_device *&operator*() { return devices_[i_]; }

    bool operator==(device_list::iterator const &other) const {
      return i_ == other.i_;
    }

    bool operator!=(device_list::iterator const &other) const {
      return i_ != other.i_;
    }

    device_list::iterator &operator++() {
      i_++;
      return *this;
    }

    device_list::iterator &operator++(int) {
      i_++;
      return *this;
    }
  };

  device_list(device_list const &) = delete;
  device_list &operator=(device_list const &) = delete;

  device_list() : devices_(nullptr), nr_devices_(0) {
    int32_t nr_devices = -1;
    devices_ = ::ibv_get_device_list(&nr_devices);
    if (nr_devices == 0) {
      ::ibv_free_device_list(devices_);
      throw std::runtime_error("no Infiniband devices found");
    }
    check_ptr(devices_, "failed to get Infiniband devices");
    nr_devices_ = nr_devices;
  }

  ~device_list() {
    if (devices_ != nullptr) {
      ::ibv_free_device_list(devices_);
    }
  }

  size_t size() { return nr_devices_; }

  struct ibv_device *at(size_t i) {
    if (i >= nr_devices_) {
      throw std::out_of_range("out of range");
    }
    return devices_[i];
  }

  iterator begin() { return iterator(devices_, 0); }

  iterator end() { return iterator(devices_, nr_devices_); }
};

/**
 * @brief This class is an abstraction of an Infiniband device.
 *
 */
class device {
  struct ibv_device *device_;
  struct ibv_context *ctx_;
  struct ibv_port_attr port_attr_;
  struct ibv_device_attr_ex device_attr_ex_;
  union ibv_gid gid_;

  int gid_index_;
  uint16_t port_num_;
  friend class pd;
  friend class cq;
  friend class qp;
  friend class srq;
  void open_device(struct ibv_device *target, uint16_t port_num) {
    device_ = target;
    port_num_ = port_num;
    ctx_ = ::ibv_open_device(device_);
    check_ptr(ctx_, "failed to open device");
    check_rc(::ibv_query_port(ctx_, port_num_, &port_attr_),
             "failed to query port");
    struct ibv_query_device_ex_input query = {};
    check_rc(::ibv_query_device_ex(ctx_, &query, &device_attr_ex_),
             "failed to query extended attributes");

    gid_index_ = 0;
    check_rc(::ibv_query_gid(ctx_, port_num, gid_index_, &gid_),
             "failed to query gid");

    auto link_layer = [&]() {
      switch (port_attr_.link_layer) {
        case IBV_LINK_LAYER_ETHERNET:
          return "ethernet";
        case IBV_LINK_LAYER_INFINIBAND:
          return "infiniband";
      }
      return "unspecified";
    }();
    auto const gid_str = gid_hex_string(gid_);
    // RDMAPP_LOG_DEBUG("opened Infiniband device gid=%s lid=%d link_layer=%s",
    //                  gid_str.c_str(), port_attr_.lid, link_layer);
  }

 public:
  device(device const &) = delete;
  device &operator=(device const &) = delete;

  /**
   * @brief Construct a new device object.
   *
   * @param target The target device.
   * @param port_num The port number of the target device.
   */
  device(struct ibv_device *target, uint16_t port_num = 1) {
    assert(target != nullptr);
    open_device(target, port_num);
  }

  /**
   * @brief Construct a new device object.
   *
   * @param device_name The name of the target device.
   * @param port_num The port number of the target device.
   */
  device(std::string const &device_name, uint16_t port_num = 1)
      : device_(nullptr), port_num_(0) {
    auto devices = device_list();
    for (auto target : devices) {
      if (::ibv_get_device_name(target) == device_name) {
        open_device(target, port_num);
        return;
      }
    }
    throw_with("no device named %s found", device_name.c_str());
  }

  /**
   * @brief Construct a new device object.
   *
   * @param device_num The index of the target device.
   * @param port_num The port number of the target device.
   */
  device(uint16_t device_num = 0, uint16_t port_num = 1)
      : device_(nullptr), port_num_(0) {
    auto devices = device_list();
    if (device_num >= devices.size()) {
      char buffer[kErrorStringBufferSize] = {0};
      ::snprintf(
          buffer, sizeof(buffer),
          "requested device number %d out of range, %lu devices available",
          device_num, devices.size());
      throw std::invalid_argument(buffer);
    }
    open_device(devices.at(device_num), port_num);
  }

  /**
   * @brief Get the device port number.
   *
   * @return uint16_t The port number.
   */
  uint16_t port_num() const { return port_num_; }

  /**
   * @brief Get the lid of the device.
   *
   * @return uint16_t The lid.
   */
  uint16_t lid() const { return port_attr_.lid; }

  /**
   * @brief Get the gid of the device.
   *
   * @return union ibv_gid The gid.
   */
  union ibv_gid gid() const {
    union ibv_gid gid_copied;
    ::memcpy(&gid_copied, &gid_, sizeof(union ibv_gid));
    return gid_copied;
  }

  /**
   * @brief Checks if the device supports fetch and add.
   *
   * @return true Supported.
   * @return false Not supported.
   */
  bool is_fetch_and_add_supported() const {
    return device_attr_ex_.orig_attr.atomic_cap != IBV_ATOMIC_NONE;
  }

  /**
   * @brief Checks if the device supports compare and swap.
   *
   * @return true Supported.
   * @return false Not supported.
   */
  bool is_compare_and_swap_supported() const {
    return device_attr_ex_.orig_attr.atomic_cap != IBV_ATOMIC_NONE;
  }

  int gid_index() const { return gid_index_; }

  struct ibv_context *ctx() const { return ctx_; }
  static std::string gid_hex_string(union ibv_gid const &gid) {
    std::string gid_str;
    char buf[16] = {0};
    const static size_t kGidLength = 16;
    for (size_t i = 0; i < kGidLength; ++i) {
      ::snprintf(buf, 16, "%02x", gid.raw[i]);
      gid_str += i == 0 ? buf : std::string(":") + buf;
    }

    return gid_str;
  }

  ~device() {
    if (ctx_ == nullptr) [[unlikely]] {
      return;
    }

    auto const gid_str = gid_hex_string(gid_);

    if (auto rc = ::ibv_close_device(ctx_); rc != 0) [[unlikely]] {
      // RDMAPP_LOG_ERROR("failed to close device gid=%s lid=%d: %s",
      //                  gid_str.c_str(), port_attr_.lid, ::strerror(rc));
    }
    else {
      // RDMAPP_LOG_DEBUG("closed device gid=%s lid=%d", gid_str.c_str(),
      //                  port_attr_.lid);
    }
  }
};

typedef device *device_ptr;

}  // namespace rdmapp