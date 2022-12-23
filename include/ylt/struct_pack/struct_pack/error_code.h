#pragma once

#include <system_error>

namespace struct_pack {
enum class errc {
  ok = 0,
  no_buffer_space,
  invalid_argument,
  hash_conflict,
};

namespace detail {

class struct_pack_category : public std::error_category {
 public:
  virtual const char *name() const noexcept override {
    return "struct_pack::category";
  }

  virtual std::string message(int err_val) const override {
    switch (static_cast<errc>(err_val)) {
      case errc::ok:
        return "ok";
      case errc::no_buffer_space:
        return "no buffer space";
      case errc::invalid_argument:
        return "invalid argument";
      case errc::hash_conflict:
        return "hash conflict";

      default:
        return "(unrecognized error)";
    }
  }
};

inline const std::error_category &category() {
  static struct_pack::detail::struct_pack_category instance;
  return instance;
}
}  // namespace detail

}  // namespace struct_pack