#pragma once
// absl compat
#include "status.h"

namespace absl {
template <typename T>
class StatusOr {
 public:
  StatusOr(Status status) : status_(status) {}
  StatusOr(const T& data) : data_(data) {}
  Status status() const { return status_; }
  T* operator->() { return &data_; }
  bool ok() const { return status_.ok(); }
  const T& operator*() const& { return data_; }

 private:
  Status status_;
  T data_;
};
}  // namespace absl