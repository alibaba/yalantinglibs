#include <optional>
#include <variant>

#include "coro_rpc/coro_rpc/rpc_protocol.h"
#include "struct_pack_protocol.hpp"
namespace coro_rpc::config {
struct coro_rpc_protocol {
  using resp_header = coro_rpc::resp_header;
  using req_header = coro_rpc::req_header;
  using rpc_error = coro_rpc::rpc_error;

  using supported_serialize_protocols = std::variant<struct_pack_protocol>;

  static std::optional<supported_serialize_protocols> get_serialize_protocol(
      req_header& req_header) {
    if (req_header.serialize_type == 0) [[likely]] {
      return struct_pack_protocol{};
    }
    else {
      return std::nullopt;
    }
  };
};
}  // namespace coro_rpc::config