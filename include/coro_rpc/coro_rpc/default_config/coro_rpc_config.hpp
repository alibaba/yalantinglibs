#pragma once

#include <chrono>
#include <thread>

#include "struct_pack_protocol.hpp"

namespace coro_rpc::config {

struct coro_rpc_config_base {
  uint16_t port = 8801;
  unsigned thread_num = std::thread::hardware_concurrency();
  std::chrono::steady_clock::duration conn_timeout_duration =
      std::chrono::seconds{0};
};

struct coro_rpc_default_config : public coro_rpc_config_base {
  using serialize_proto = struct_pack_protocol;
};

}  // namespace coro_rpc::config
