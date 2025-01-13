#pragma once

#include <utility>

#include "ucxpp/context.h"

namespace ucxpp {

/**
 * @brief A serializable UCX memory handle ready to send to peer.
 *
 */
class packed_memory_rkey {
  friend class local_memory_handle;
  void *buffer_;
  size_t length_;
  packed_memory_rkey(void *buffer, size_t length)
      : buffer_(buffer), length_(length) {}

 public:
  packed_memory_rkey(packed_memory_rkey const &) = delete;
  packed_memory_rkey &operator=(packed_memory_rkey const &) = delete;

  packed_memory_rkey(packed_memory_rkey &&other)
      : buffer_(std::exchange(other.buffer_, nullptr)),
        length_(std::exchange(other.length_, 0)) {}
  void *get_buffer() { return buffer_; }
  void const *get_buffer() const { return buffer_; }
  size_t get_length() const { return length_; }
  ~packed_memory_rkey() {
    if (buffer_ != nullptr) {
      ::ucp_rkey_buffer_release(buffer_);
    }
  }
};

/**
 * @brief Represents a registered local memory region.
 *
 */
class local_memory_handle {
  context_ptr ctx_;
  ucp_mem_h mem_;

 public:
  local_memory_handle(local_memory_handle const &) = delete;
  local_memory_handle &operator=(local_memory_handle const &) = delete;

  /**
   * @brief Construct a new local memory handle object
   *
   * @param ctx UCX context
   * @param mem UCX memory handle
   */
  local_memory_handle(context_ptr ctx, ucp_mem_h mem) : ctx_(ctx), mem_(mem) {}

  /**
   * @brief Construct a new local memory handle object
   *
   * @param other Another local memory handle to move from
   */
  local_memory_handle(local_memory_handle &&other)
      : ctx_(std::move(other.ctx_)), mem_(std::exchange(other.mem_, nullptr)) {}

  /**
   * @brief Allocate and register a local memory region
   *
   * @param ctx UCX context
   * @param length Desired length of the memory region
   * @return std::pair<void *, local_memory_handle> A pair of pointer to the
   * allocated memory region and the local memory handle
   */
  static std::pair<void *, local_memory_handle> allocate_mem(context_ptr ctx,
                                                             size_t length) {
    ucp_mem_h mem;
    ucp_mem_attr_t attr;
    ucp_mem_map_params_t map_params;
    map_params.address = nullptr;
    map_params.length = length;
    map_params.flags = UCP_MEM_MAP_ALLOCATE;
    map_params.field_mask = UCP_MEM_MAP_PARAM_FIELD_ADDRESS |
                            UCP_MEM_MAP_PARAM_FIELD_LENGTH |
                            UCP_MEM_MAP_PARAM_FIELD_FLAGS;
    check_ucs_status(::ucp_mem_map(ctx->context_, &map_params, &mem),
                     "failed to map memory");
    attr.field_mask = UCP_MEM_ATTR_FIELD_ADDRESS;
    check_ucs_status(::ucp_mem_query(mem, &attr),
                     "failed to get memory address");
    return std::make_pair(attr.address, local_memory_handle(ctx, mem));
  }

  static local_memory_handle register_mem(context_ptr ctx, void *address,
                                          size_t length) {
    ucp_mem_h mem;
    ucp_mem_map_params_t map_params;
    map_params.address = address;
    map_params.length = length;
    map_params.field_mask =
        UCP_MEM_MAP_PARAM_FIELD_ADDRESS | UCP_MEM_MAP_PARAM_FIELD_LENGTH;
    check_ucs_status(::ucp_mem_map(ctx->context_, &map_params, &mem),
                     "failed to map memory");

    return local_memory_handle(ctx, mem);
  }

  /**
   * @brief Get the native UCX memory handle
   *
   * @return ucp_mem_h
   */
  ucp_mem_h handle() const { return mem_; }

  /**
   * @brief Pack the information needed for remote access to the memory region.
   * It is intended to be sent to the remote peer.
   *
   * @return packed_memory_rkey The packed memory handle
   */
  packed_memory_rkey pack_rkey() const {
    void *buffer;
    size_t length;
    check_ucs_status(::ucp_rkey_pack(ctx_->context_, mem_, &buffer, &length),
                     "failed to pack memory");
    return packed_memory_rkey(buffer, length);
  }

  /**
   * @brief Destroy the local memory handle object. The memory region will be
   * deregistered.
   *
   */
  ~local_memory_handle() {
    if (mem_ != nullptr) {
      (void)::ucp_mem_unmap(ctx_->context_, mem_);
    }
  }
};

}  // namespace ucxpp