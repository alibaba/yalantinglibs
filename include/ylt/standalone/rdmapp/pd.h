#pragma once

#include <infiniband/verbs.h>

#include "device.h"
#include "error.h"
#include "mr.h"

namespace rdmapp {

class qp;

/**
 * @brief This class is an abstraction of a Protection Domain.
 *
 */
class pd {
  device_ptr device_;
  struct ibv_pd *pd_;
  friend class qp;
  friend class srq;

 public:
  pd(pd const &) = delete;
  pd &operator=(pd const &) = delete;

  /**
   * @brief Construct a new pd object
   *
   * @param device The device to use.
   */
  pd(device_ptr device) : device_(device) {
    pd_ = ::ibv_alloc_pd(device->ctx_);
    check_ptr(pd_, "failed to alloc pd");
  }

  /**
   * @brief Get the device object pointer.
   *
   * @return device_ptr The device object pointer.
   */
  device_ptr device() const { return device_; }

  /**
   * @brief Register a local memory region.
   *
   * @param addr The address of the memory region.
   * @param length The length of the memory region.
   * @param flags The access flags to use.
   * @return local_mr The local memory region handle.
   */
  local_mr reg_mr(void *addr, size_t length,
                  int flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE |
                              IBV_ACCESS_REMOTE_READ |
                              IBV_ACCESS_REMOTE_ATOMIC) {
    auto mr = ::ibv_reg_mr(pd_, addr, length, flags);
    check_ptr(mr, "failed to reg mr");
    return rdmapp::local_mr(this, mr);
  }
  /**
   * @brief Destroy the pd object and the associated protection domain.
   *
   */
  ~pd() {
    if (pd_ == nullptr) [[unlikely]] {
      return;
    }
    if (auto rc = ::ibv_dealloc_pd(pd_); rc != 0) [[unlikely]] {
    }
    else {
    }
  }
};

typedef pd *pd_ptr;

}  // namespace rdmapp