/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <filesystem>
#include <ylt/easylog.hpp>

#ifdef YLT_ENABLE_SSL
#include <asio/ssl.hpp>
#endif

namespace coro_rpc {
/*!
 * \file common_service.hpp
 */

/*!
 * SSL config
 */
struct ssl_configure {
  std::string base_path;  //!< all config files base path
  std::string cert_file;  //!< relative path of certificate chain file
  std::string key_file;   //!< relative path of private key file
  std::string dh_file;    //!< relative path of tmp dh file (optional)
};

/*!
 * Check file (not a folder) exist
 *
 * just a helper function
 *
 * @param path
 * @return true if file exist, otherwise false
 */
inline bool file_exists(const auto &path) {
  std::error_code ec;
  if (!std::filesystem::is_directory(path, ec) &&
      std::filesystem::exists(path, ec)) {
    return true;
  }
  return false;
};

#ifdef YLT_ENABLE_SSL
/*!
 * Initialize SSL Context `context` with SSL Config `conf`
 *
 * If init fail, a log reported and return false.
 *
 * @param context instance of asio::ssl::context
 * @param conf object of ssl_configure
 * @return true if init success, otherwise false
 */
inline bool init_ssl_context_helper(asio::ssl::context &context,
                                    const ssl_configure &conf) {
  namespace fs = std::filesystem;
  try {
    context.set_options(asio::ssl::context::default_workarounds |
                        asio::ssl::context::no_sslv2 |
                        asio::ssl::context::single_dh_use);
    context.set_password_callback(
        [](std::size_t size,
           asio::ssl::context_base::password_purpose purpose) {
          return "test";
        });
    auto cert_file = fs::path(conf.base_path).append(conf.cert_file);
    auto key_file = fs::path(conf.base_path).append(conf.key_file);
    auto dh_file = fs::path(conf.base_path).append(conf.dh_file);

    ELOGV(INFO, "current path %s", fs::current_path().string().data());
    if (file_exists(cert_file)) {
      ELOGV(INFO, "load %s", cert_file.string().data());
      context.use_certificate_chain_file(cert_file);
    }
    else {
      ELOGV(ERROR, "no certificate file %s", cert_file.string().data());
      return false;
    }

    if (file_exists(key_file)) {
      ELOGV(INFO, "load %s", key_file.string().data());
      context.use_private_key_file(key_file, asio::ssl::context::pem);
    }
    else {
      ELOGV(ERROR, "no private key file %s", key_file.string().data());
      return false;
    }

    if (file_exists(dh_file)) {
      ELOGV(INFO, "load %s", dh_file.string().data());
      context.use_tmp_dh_file(dh_file);
    }
    else {
      ELOGV(INFO, "no temp dh file %s", dh_file.string().data());
    }

    return true;
  } catch (std::exception &e) {
    ELOGV(INFO, "%s", e.what());
    return false;
  }
}
#endif
}  // namespace coro_rpc