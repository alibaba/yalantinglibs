#pragma once
#include <infiniband/verbs.h>

#include <string>
#include <system_error>

namespace coro_io {
class ib_error_code_category : public std::error_category {
 public:
  const char* name() const noexcept override {
    return "ibverbs_error_code";  // 错误类别的名称
  }

  std::string message(int ev) const override {
    using namespace std::string_literals;
    switch (static_cast<ibv_wc_status>(ev)) {
      case ibv_wc_status::IBV_WC_SUCCESS:
        return "IBV_WC_OK"s;
      default:
        return "Unknown error"s;
    }
  }
  static auto& instance() {
    static ib_error_code_category instance;
    return instance;
  }
};

inline std::error_code make_error_code(ibv_wc_status e) {
  return {static_cast<int>(e), ib_error_code_category::instance()};
}
}  // namespace coro_io