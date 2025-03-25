#pragma once

#include <ucp/api/ucp.h>

#include "ucxpp/error.h"

namespace ucxpp {

/**
 * @brief Stores UCX configuration options.
 *
 */
class config {
  ucp_config_t *config_;

 public:
  config(config const &) = delete;
  config &operator=(config const &) = delete;

  /**
   * @brief Construct a new config object
   *
   * @param env_prefix If non-NULL, the routine searches for the environment
   * variables that start with <env_prefix>_UCX_ prefix. Otherwise, the routine
   * searches for the environment variables that start with UCX_ prefix.
   * @param filename 	If non-NULL, read configuration from the file defined by
   * filename. If the file does not exist, it will be ignored and no error
   * reported to the application.
   */
  config(char const *env_prefix = nullptr, char const *filename = nullptr) {
    check_ucs_status(::ucp_config_read(env_prefix, filename, &config_),
                     "failed to read ucp config");
  }

  /**
   * @brief Destroy the config object and release memory
   *
   */
  ~config() { ::ucp_config_release(config_); }

  /**
   * @brief Get the native UCX configuration handle
   *
   * @return ucp_config_t* The native UCX configuration handle
   */
  ucp_config_t *handle() const { return config_; }

  /**
   * @brief Modify a configuration option
   *
   * @param name The name of the configuration option to modify
   * @param value The new value of the configuration option
   */
  void modify(char const *name, char const *value) {
    check_ucs_status(::ucp_config_modify(config_, name, value),
                     "failed to modify ucp config");
  }

  /**
   * @brief Print the configuration to the standard output
   *
   */
  void print() const {
    ::ucp_config_print(config_, stdout, nullptr, UCS_CONFIG_PRINT_CONFIG);
  }
};

}  // namespace ucxpp