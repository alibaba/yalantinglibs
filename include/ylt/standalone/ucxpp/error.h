#pragma once
#include <ucp/api/ucp.h>
#include <ucs/type/status.h>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace ucxpp {

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

static inline void check_ucs_status(ucs_status_t status, const char *message) {
  if (status == UCS_OK) [[likely]] {
    return;
  }
  if (status == UCS_INPROGRESS) [[likely]] {
    return;
  }

  throw_with("%s: %s (status=%d)", message, ::ucs_status_string(status),
             status);
}

static inline void check_rc(int rc, const char *message) {
  if (rc != 0) [[unlikely]] {
    throw_with("%s: %s (rc=%d)", message, ::strerror(rc), rc);
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

}  // namespace ucxpp