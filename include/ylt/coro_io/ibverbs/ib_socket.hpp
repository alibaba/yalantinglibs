#pragma once
#include <infiniband/verbs.h>
#include <cstdint>
#include <memory>
#include <system_error>

#include "ib_device.hpp"

namespace coro_io {
  class ib_socket_t {
  public:
    struct config_t {
      uint32_t cq_size;
      ibv_qp_init_attr qp_config;
    };
    ib_socket_t(ib_device_t& device, const config_t& config= {}) {
      channel_.reset(ibv_create_comp_channel(device.context()));
      if (!channel_) [[unlikely]] {
        throw std::system_error(std::make_error_code(std::errc{errno}));
      }
      cq_.reset(ibv_create_cq(device.context(), config.cq_size, NULL, channel_.get(), 0));
      if (!cq_) [[unlikely]] {
        throw std::system_error(std::make_error_code(std::errc{errno}));
      }
    }
    struct ib_connect_info {
      uint64_t addr;    // buffer address
      uint32_t rkey;    // remote key
      uint32_t qp_num;  // QP number
      uint16_t lid;     // LID of the IB port
      uint8_t gid[16];  // GID
    };
    bool has_closed() const noexcept {
      return qp_==nullptr;
    }
  private:
    std::unique_ptr<ibv_comp_channel, ib_deleter> channel_;
    std::unique_ptr<ibv_cq, ib_deleter> cq_;
    std::unique_ptr<ibv_qp, ib_deleter> qp_;
    ib_connect_info peer_info_;
    size_t recv_op_id_ = 0;
    size_t send_op_id_ = 0;
  public:
    void close(std::error_code& ec);
  };
}
