#include <struct_pack/struct_pack.hpp>
namespace coro_rpc::config {

struct struct_pack_protocol {
  template <typename T>
  static bool deserialize_to(T& t, std::string_view buffer) {
    return struct_pack::deserialize_to(t, buffer) == struct_pack::errc::ok;
  }
  template <typename T>
  static std::string serialize(const T& t) {
    return struct_pack::serialize<std::string>(t);
  }
  static std::string serialize() {
    return struct_pack::serialize<std::string>(std::monostate{});
  }
};
}  // namespace coro_rpc::config