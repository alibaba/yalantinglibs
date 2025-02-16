#pragma once

#include <infiniband/verbs.h>

#include <cstdint>
#include <utility>
#include <vector>

#include "detail/serdes.h"

namespace rdmapp {

namespace tags {
namespace mr {
struct local {};
struct remote {};
}  // namespace mr
}  // namespace tags

class pd;

/**
 * @brief A remote or local memory region.
 *
 * @tparam Tag Either `tags::mr::local` or `tags::mr::remote`.
 */
template <class Tag>
class mr;

/**
 * @brief Represents a local memory region.
 *
 */
template <>
class mr<tags::mr::local> {
  struct ibv_mr *mr_;
  pd *pd_;

 public:
  mr(mr const &) = delete;
  mr &operator=(mr const &) = delete;

  /**
   * @brief Construct a new mr object
   *
   * @param pd The protection domain to use.
   * @param mr The ibverbs memory region handle.
   */
  mr(pd *pd, struct ibv_mr *mr) : mr_(mr), pd_(pd) {}

  /**
   * @brief Move construct a new mr object
   *
   * @param other The other mr object to move from.
   */
  mr(mr<tags::mr::local> &&other)
      : mr_(std::exchange(other.mr_, nullptr)), pd_(std::move(other.pd_)) {}

  /**
   * @brief Move assignment operator.
   *
   * @param other The other mr to move from.
   * @return mr<tags::mr::local>& This mr.
   */
  mr<tags::mr::local> &operator=(mr<tags::mr::local> &&other) {
    mr_ = other.mr_;
    pd_ = std::move(other.pd_);
    other.mr_ = nullptr;
    return *this;
  }

  /**
   * @brief Destroy the mr object and deregister the memory region.
   *
   */
  ~mr() {
    if (mr_ == nullptr) [[unlikely]] {
      // This mr is moved.
      return;
    }
    if (auto rc = ::ibv_dereg_mr(mr_); rc != 0) [[unlikely]] {
    }
    else {
    }
  }

  /**
   * @brief Serialize the memory region handle to be sent to a remote peer.
   *
   * @return std::vector<uint8_t> The serialized memory region handle.
   */
  std::vector<uint8_t> serialize() const {
    std::vector<uint8_t> buffer;
    auto it = std::back_inserter(buffer);
    detail::serialize(reinterpret_cast<uint64_t>(mr_->addr), it);
    detail::serialize(mr_->length, it);
    detail::serialize(mr_->rkey, it);
    return buffer;
  }

  /**
   * @brief Get the address of the memory region.
   *
   * @return void* The address of the memory region.
   */
  void *addr() const { return mr_->addr; }

  /**
   * @brief Get the length of the memory region.
   *
   * @return size_t The length of the memory region.
   */
  size_t length() const { return mr_->length; }

  /**
   * @brief Get the remote key of the memory region.
   *
   * @return uint32_t The remote key of the memory region.
   */
  uint32_t rkey() const { return mr_->rkey; }

  /**
   * @brief Get the local key of the memory region.
   *
   * @return uint32_t The local key of the memory region.
   */
  uint32_t lkey() const { return mr_->lkey; }
};

/**
 * @brief Represents a remote memory region.
 *
 */
template <>
class mr<tags::mr::remote> {
  void *addr_;
  size_t length_;
  uint32_t rkey_;

 public:
  /**
   * @brief The size of a serialized remote memory region.
   *
   */
  static constexpr size_t kSerializedSize =
      sizeof(addr_) + sizeof(length_) + sizeof(rkey_);

  mr() = default;

  /**
   * @brief Construct a new remote mr object
   *
   * @param addr The address of the remote memory region.
   * @param length The length of the remote memory region.
   * @param rkey The remote key of the remote memory region.
   */
  mr(void *addr, uint32_t length, uint32_t rkey)
      : addr_(addr), length_(length), rkey_(rkey) {}

  /**
   * @brief Construct a new remote mr object copied from another
   *
   * @param other The other remote mr object to copy from.
   */
  mr(mr<tags::mr::remote> const &other) = default;

  /**
   * @brief Get the address of the remote memory region.
   *
   * @return void* The address of the remote memory region.
   */
  void *addr() const { return addr_; }

  /**
   * @brief Get the length of the remote memory region.
   *
   * @return uint32_t The length of the remote memory region.
   */
  uint32_t length() const { return length_; }

  /**
   * @brief Get the remote key of the memory region.
   *
   * @return uint32_t The remote key of the memory region.
   */
  uint32_t rkey() const { return rkey_; }

  /**
   * @brief Deserialize a remote memory region handle.
   *
   * @tparam It The iterator type.
   * @param it The iterator to deserialize from.
   * @return mr<tags::mr::remote> The deserialized remote memory region handle.
   */
  template <class It>
  static mr<tags::mr::remote> deserialize(It it) {
    mr<tags::mr::remote> remote_mr;
    detail::deserialize(it, remote_mr.addr_);
    detail::deserialize(it, remote_mr.length_);
    detail::deserialize(it, remote_mr.rkey_);
    return remote_mr;
  }
};

using local_mr = mr<tags::mr::local>;
using remote_mr = mr<tags::mr::remote>;

typedef local_mr *local_mr_ptr;
typedef remote_mr *remote_mr_ptr;

}  // namespace rdmapp