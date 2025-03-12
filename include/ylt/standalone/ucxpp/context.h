#pragma once

#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ucp/api/ucp.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ucxpp/config.h"
#include "ucxpp/error.h"

namespace ucxpp {

class context;
using context_ptr = context *;

/**
 * @brief Abstraction of a UCX context
 *
 */
class context {
  friend class worker;
  friend class local_memory_handle;
  ucp_context_h context_;
  uint64_t features_;

 public:
  /**
   * @brief Context builder
   *
   */
  class builder {
    uint64_t features_;
    bool print_config_;
    bool enable_mt_;

   public:
    builder() : features_(0), print_config_(false), enable_mt_(false) {};
    /**
     * @brief Build and return a context object
     *
     * @return context_ptr The built context object
     */
    context_ptr build() {
      return new context(features_, print_config_, enable_mt_);
    }

    /**
     * @brief Print the config to stdout when building context
     *
     * @return builder
     */
    builder &enable_print_config() {
      print_config_ = true;
      return *this;
    }

    /**
     * @brief Enable the wakeup feature
     *
     * @return builder&
     */
    builder &enable_wakeup() {
      features_ |= UCP_FEATURE_WAKEUP;
      return *this;
    }

    /**
     * @brief Enable tag-related operations
     *
     * @return builder&
     */
    builder &enable_tag() {
      features_ |= UCP_FEATURE_TAG;
      return *this;
    }

    /**
     * @brief Enable stream-related operations
     *
     * @return builder&
     */
    builder &enable_stream() {
      features_ |= UCP_FEATURE_STREAM;
      return *this;
    }

    /**
     * @brief Enable active message feature
     *
     * @return builder&
     */
    builder &enable_am() {
      features_ |= UCP_FEATURE_AM;
      return *this;
    }

    /**
     * @brief Enable remote memory access feature
     *
     * @return builder&
     */
    builder &enable_rma() {
      features_ |= UCP_FEATURE_RMA;
      return *this;
    }

    /**
     * @brief Enable atomic memory operations with 32-bit operands
     *
     * @return builder&
     */
    builder &enable_amo32() {
      features_ |= UCP_FEATURE_AMO32;
      return *this;
    }

    /**
     * @brief Enable atomic memory operations with 64-bit operands
     *
     * @return builder&
     */
    builder &enable_amo64() {
      features_ |= UCP_FEATURE_AMO64;
      return *this;
    }

    /**
     * @brief Enable multi-threading
     *
     * @return builder&
     */
    builder &enable_mt() {
      enable_mt_ = true;
      return *this;
    }
  };

  /**
   * @brief Construct a new context object
   *
   * @param features Feature flags
   * @param print_config Print the config to stdout
   * @param enable_mt Enable multi-threading
   */
  context(uint64_t features, bool print_config, bool enable_mt)
      : features_(features) {
    config config;
    ucp_params_t ucp_params;
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
    ucp_params.features = features;
    if (enable_mt) {
      ucp_params.field_mask |= UCP_PARAM_FIELD_MT_WORKERS_SHARED;
      ucp_params.mt_workers_shared = 1;
    }
    check_ucs_status(::ucp_init(&ucp_params, config.handle(), &context_),
                     "failed to init ucp");
    if (print_config) {
      config.print();
    }
  }

  /**
   * @brief Get the features of the context
   *
   * @return uint64_t Feature flags
   */
  uint64_t features() const { return features_; }

  /**
   * @brief Get the native UCX handle of the context
   *
   * @return ucp_context_h The native UCX handle
   */
  ucp_context_h handle() const { return context_; }

  /**
   * @brief Destroy the context object and release resources
   *
   */
  ~context() { ::ucp_cleanup(context_); }
};

}  // namespace ucxpp