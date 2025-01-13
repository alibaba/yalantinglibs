#pragma once
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <infiniband/verbs.h>

namespace rdmapp {

constexpr size_t kErrorStringBufferSize = 1024;

static inline void throw_with(const char *message) {
  throw std::runtime_error(message);
}

template <class... Args>
static inline void throw_with(const char *format, Args... args) {
  char buffer[kErrorStringBufferSize];
  ::snprintf(buffer, sizeof(buffer), format, args...);
  throw std::runtime_error(buffer);
}

static inline void check_rc(int rc, const char *message) {
  if (rc != 0) [[unlikely]] {
    throw_with("%s: %s (rc=%d)", message, ::strerror(rc), rc);
  }
}

static inline void check_wc_status(enum ibv_wc_status status,
                                   const char *message) {
  if (status != IBV_WC_SUCCESS) [[unlikely]] {
    auto errorstr = [status]() {
      switch (status) {
      case IBV_WC_SUCCESS:
        return "IBV_WC_SUCCESS";
      case IBV_WC_LOC_LEN_ERR:
        return "IBV_WC_LOC_LEN_ERR";
      case IBV_WC_LOC_QP_OP_ERR:
        return "IBV_WC_LOC_QP_OP_ERR";
      case IBV_WC_LOC_EEC_OP_ERR:
        return "IBV_WC_LOC_EEC_OP_ERR";
      case IBV_WC_LOC_PROT_ERR:
        return "IBV_WC_LOC_PROT_ERR";
      case IBV_WC_WR_FLUSH_ERR:
        return "IBV_WC_WR_FLUSH_ERR";
      case IBV_WC_MW_BIND_ERR:
        return "IBV_WC_MW_BIND_ERR";
      case IBV_WC_BAD_RESP_ERR:
        return "IBV_WC_BAD_RESP_ERR";
      case IBV_WC_LOC_ACCESS_ERR:
        return "IBV_WC_LOC_ACCESS_ERR";
      case IBV_WC_REM_INV_REQ_ERR:
        return "IBV_WC_REM_INV_REQ_ERR";
      case IBV_WC_REM_ACCESS_ERR:
        return "IBV_WC_REM_ACCESS_ERR";
      case IBV_WC_REM_OP_ERR:
        return "IBV_WC_REM_OP_ERR";
      case IBV_WC_RETRY_EXC_ERR:
        return "IBV_WC_RETRY_EXC_ERR";
      case IBV_WC_RNR_RETRY_EXC_ERR:
        return "IBV_WC_RNR_RETRY_EXC_ERR";
      case IBV_WC_LOC_RDD_VIOL_ERR:
        return "IBV_WC_LOC_RDD_VIOL_ERR";
      case IBV_WC_REM_INV_RD_REQ_ERR:
        return "IBV_WC_REM_INV_RD_REQ_ERR";
      case IBV_WC_REM_ABORT_ERR:
        return "IBV_WC_REM_ABORT_ERR";
      case IBV_WC_INV_EECN_ERR:
        return "IBV_WC_INV_EECN_ERR";
      case IBV_WC_INV_EEC_STATE_ERR:
        return "IBV_WC_INV_EEC_STATE_ERR";
      case IBV_WC_FATAL_ERR:
        return "IBV_WC_FATAL_ERR";
      case IBV_WC_RESP_TIMEOUT_ERR:
        return "IBV_WC_RESP_TIMEOUT_ERR";
      case IBV_WC_GENERAL_ERR:
        return "IBV_WC_GENERAL_ERR";
      case IBV_WC_TM_ERR:
        return "IBV_WC_TM_ERR";
      case IBV_WC_TM_RNDV_INCOMPLETE:
        return "IBV_WC_TM_RNDV_INCOMPLETE";
      }
      return "UNKNOWN_ERROR";
    }();
    throw_with("%s: %s (status=%d)", message, errorstr, status);
  }
}
static inline void check_ptr(void *ptr, const char *message) {
  if (ptr == nullptr) [[unlikely]] {
    throw_with("%s: %s (errno=%d)", message, ::strerror(errno), errno);
  }
}

static inline void check_errno(int rc, const char *message) {
  if (rc < 0) [[unlikely]] {
    throw_with("%s: %s (errno=%d)", message, ::strerror(errno), errno);
  }
}

} // namespace rdmapp