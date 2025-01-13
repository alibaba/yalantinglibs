#pragma once

#include <infiniband/verbs.h>

#include "pd.h"

namespace rdmapp {

/**
 * @brief This class represents a Shared Receive Queue.
 *
 */
class srq {
  struct ibv_srq *srq_;
  pd_ptr pd_;
  friend class qp;

 public:
  /**
   * @brief Construct a new srq object
   *
   * @param pd The protection domain to use.
   * @param max_wr The maximum number of outstanding work requests.
   */
  srq(pd_ptr pd, size_t max_wr = 1024) : srq_(nullptr), pd_(pd) {
    struct ibv_srq_init_attr srq_init_attr;
    srq_init_attr.srq_context = this;
    srq_init_attr.attr.max_sge = 1;
    srq_init_attr.attr.max_wr = max_wr;
    srq_init_attr.attr.srq_limit = max_wr;

    srq_ = ::ibv_create_srq(pd_->pd_, &srq_init_attr);
    check_ptr(srq_, "failed to create srq");
  }

  /**
   * @brief Destroy the srq object and the associated shared receive queue.
   *
   */
  ~srq() {
    if (srq_ == nullptr) [[unlikely]] {
      return;
    }

    if (auto rc = ::ibv_destroy_srq(srq_); rc != 0) [[unlikely]] {
    }
    else {
    }
  }
};

typedef srq *srq_ptr;

}  // namespace rdmapp