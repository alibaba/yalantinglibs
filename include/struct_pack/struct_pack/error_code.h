#pragma once

#include <system_error>

namespace struct_pack {
enum class errc {
  ok = 0,
  no_buffer_space,
  invalid_argument,
  address_in_use,
  hash_conflict,
};

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
      case errc::address_in_use:
        return "address in use";
      case errc::hash_conflict:
        return "hash conflict";

      default:
        return "(unrecognized error)";
    }
  }
};

inline const std::error_category &category() {
  static struct_pack::struct_pack_category instance;
  return instance;
}

inline std::error_code make_error_code(struct_pack::errc err) {
  return std::error_code((std::underlying_type_t<errc> &)err,
                         struct_pack::category());
}
}  // namespace struct_pack