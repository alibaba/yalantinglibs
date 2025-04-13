#pragma once

#include <ucp/api/ucp.h>

#include <utility>
#include <vector>

namespace ucxpp {

/**
 * @brief Represents a remote UCX address.
 *
 */
class remote_address {
  std::vector<char> address_;

 public:
  /**
   * @brief Construct a new remote address object
   *
   * @param address The received address buffer
   */
  remote_address(std::vector<char> const &address) : address_(address) {}

  /**
   * @brief Construct a new remote address object
   *
   * @param address Another remote address object to move from
   */
  remote_address(std::vector<char> &&address) : address_(std::move(address)) {}

  /**
   * @brief Move construct a new remote address object
   *
   * @param other Another remote address object to move from
   */
  remote_address(remote_address &&other) = default;

  /**
   * @brief Construct a new remote address object
   *
   * @param other Another remote address object to copy from
   */
  remote_address(remote_address const &other) = default;

  /**
   * @brief Copy assignment operator
   *
   * @param other Another remote address object to copy from
   * @return remote_address& This object
   */
  remote_address &operator=(remote_address const &other) = default;

  /**
   * @brief Move assignment operator
   *
   * @param other Another remote address object to move from
   * @return remote_address& This object
   */
  remote_address &operator=(remote_address &&other) = default;

  /**
   * @brief Get the UCP address
   *
   * @return const ucp_address_t* The UCP address
   */
  const ucp_address_t *get_address() const {
    return reinterpret_cast<const ucp_address_t *>(address_.data());
  }

  /**
   * @brief Get the length of the address
   *
   * @return size_t The length of the address
   */
  size_t get_length() const { return address_.size(); }
};

}  // namespace ucxpp