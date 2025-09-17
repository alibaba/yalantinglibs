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
#ifndef OPENSSL_NO_NTLS
///*!
// * NTLS config for Tongsuo support
// */
// struct ntls_configure {
//  std::string base_path;           //!< all config files base path
//  std::string sign_cert_file;      //!< relative path of SM2 signing
//  certificate file std::string sign_key_file;       //!< relative path of SM2
//  signing private key file std::string enc_cert_file;       //!< relative path
//  of SM2 encryption certificate file std::string enc_key_file;        //!<
//  relative path of SM2 encryption private key file std::string ca_cert_file;
//  //!< relative path of CA certificate file (optional) bool
//  enable_client_verify = false; //!< enable client certificate verification
//  bool enable_ntls = true;         //!< enable NTLS mode (国密SSL)
//};

/*!
 * Extended SSL config that supports both SSL and NTLS
 */
struct ssl_ntls_configure {
  std::string base_path;  //!< all config files base path

  // Traditional SSL/TLS configuration
  std::string cert_file;  //!< relative path of certificate chain file
  std::string key_file;   //!< relative path of private key file
  std::string dh_file;    //!< relative path of tmp dh file (optional)

#ifndef OPENSSL_NO_NTLS
  // NTLS configuration for Tongsuo
  std::string
      sign_cert_file;         //!< relative path of SM2 signing certificate file
  std::string sign_key_file;  //!< relative path of SM2 signing private key file
  std::string
      enc_cert_file;  //!< relative path of SM2 encryption certificate file
  std::string
      enc_key_file;  //!< relative path of SM2 encryption private key file
  std::string
      ca_cert_file;  //!< relative path of CA certificate file (optional)
  std::string cipher_suites;  //!< NTLS cipher suites (e.g.,
                              //!< "ECC-SM2-SM4-GCM-SM3:ECC-SM2-SM4-CBC-SM3")

  bool enable_ntls = false;  //!< enable NTLS mode
  bool enable_client_verify =
      false;                      //!< enable client certificate verification
  bool enable_dual_mode = false;  //!< enable both SSL and NTLS support
#endif
};
#endif OPENSSL_NO_NTLS

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

    ELOG_INFO << "current path " << fs::current_path().string();
    if (file_exists(cert_file)) {
      ELOG_INFO << "load " << cert_file.string();
      context.use_certificate_chain_file(cert_file.string());
    }
    else {
      ELOG_ERROR << "no certificate file " << cert_file.string();
      return false;
    }

    if (file_exists(key_file)) {
      ELOG_INFO << "load " << key_file.string();
      context.use_private_key_file(key_file.string(), asio::ssl::context::pem);
    }
    else {
      ELOG_ERROR << "no private file " << key_file.string();
      return false;
    }

    if (file_exists(dh_file)) {
      ELOG_INFO << "load " << dh_file.string();
      context.use_tmp_dh_file(dh_file.string());
    }
    else {
      ELOG_INFO << "no temp dh file " << dh_file.string();
    }

    return true;
  } catch (std::exception &e) {
    ELOG_INFO << e.what();
    return false;
  }
}

#ifndef OPENSSL_NO_NTLS
/*!
 * Initialize SSL Context for NTLS with Tongsuo
 *
 * @param context instance of asio::ssl::context
 * @param conf object of ssl_ntls_configure
 * @return true if init success, otherwise false
 */
inline bool init_ntls_context_helper(asio::ssl::context &context,
                                     const ssl_ntls_configure &conf) {
  namespace fs = std::filesystem;
  try {
    // Set context options via asio
    context.set_options(asio::ssl::context::default_workarounds |
                        asio::ssl::context::no_sslv2 |
                        asio::ssl::context::single_dh_use);
    
    // Configure NTLS via Tongsuo native APIs
    SSL_CTX* ctx = context.native_handle();
    if (!ctx) {
      ELOG_ERROR << "SSL_CTX native_handle is null";
      return false;
    }

    SSL_CTX_enable_ntls(ctx);
    ELOG_INFO << "NTLS mode enabled successfully";
    // Set NTLS cipher suites (SM2/SM3/SM4)
    std::string cipher_suites = conf.cipher_suites.empty()
                                    ? "ECC-SM2-SM4-GCM-SM3:ECC-SM2-SM4-CBC-SM3"
                                    : conf.cipher_suites;
    if (SSL_CTX_set_cipher_list(ctx, cipher_suites.c_str()) != 1) {
      unsigned long err = ::ERR_get_error();
      ELOG_WARN << "Failed to set NTLS cipher suites '" << cipher_suites
                << "': " << ::ERR_error_string(err, nullptr);
    } else {
      ELOG_INFO << "NTLS cipher suites set to: " << cipher_suites;
    }
    context.set_password_callback(
        [](std::size_t size,
           asio::ssl::context_base::password_purpose purpose) {
          return "test";
        });
    auto sign_cert_file = fs::path(conf.base_path).append(conf.sign_cert_file);
    auto sign_key_file  = fs::path(conf.base_path).append(conf.sign_key_file);
    auto enc_cert_file  = fs::path(conf.base_path).append(conf.enc_cert_file);
    auto enc_key_file   = fs::path(conf.base_path).append(conf.enc_key_file);

    ELOG_INFO << "current path " << fs::current_path().string();

    // Load SM2 signing certificate and key
    if (file_exists(sign_cert_file)) {
      if (SSL_CTX_use_sign_certificate_file(ctx, sign_cert_file.string().c_str(),
                                            SSL_FILETYPE_PEM) != 1) {
        unsigned long err = ::ERR_get_error();
        ELOG_ERROR << "failed to load SM2 signing certificate: "
                   << ::ERR_error_string(err, nullptr);
        return false;
      }
    } else {
      ELOG_ERROR << "no SM2 signing certificate file "
                 << sign_cert_file.string();
      return false;
    }

    if (file_exists(sign_key_file)) {
      if (SSL_CTX_use_sign_PrivateKey_file(ctx, sign_key_file.string().c_str(),
                                           SSL_FILETYPE_PEM) != 1) {
        unsigned long err = ::ERR_get_error();
        ELOG_ERROR << "failed to load SM2 signing private key: "
                   << ::ERR_error_string(err, nullptr);
        return false;
      }
    } else {
      ELOG_ERROR << "no SM2 signing key file "
                 << sign_key_file.string();
      return false;
    }

    // Load SM2 encryption certificate and key
    if (file_exists(enc_cert_file)) {
      if (SSL_CTX_use_enc_certificate_file(ctx, enc_cert_file.string().c_str(),
                                           SSL_FILETYPE_PEM) != 1) {
        unsigned long err = ::ERR_get_error();
        ELOG_ERROR << "failed to load SM2 encryption certificate: "
                   << ::ERR_error_string(err, nullptr);
        return false;
      }
    } else {
      ELOG_ERROR << "no SM2 encryption certificate file "
                 << enc_cert_file.string();
      return false;
    }

    if (file_exists(enc_key_file)) {
      if (SSL_CTX_use_enc_PrivateKey_file(ctx, enc_key_file.string().c_str(),
                                          SSL_FILETYPE_PEM) != 1) {
        unsigned long err = ::ERR_get_error();
        ELOG_ERROR << "failed to load SM2 encryption private key: "
                   << ::ERR_error_string(err, nullptr);
        return false;
      }
    } else {
      ELOG_ERROR << "no SM2 encryption key file "
                 << enc_key_file.string();
      return false;
    }

    // Load CA certificate if provided (ASIO wrapper is fine)
    asio::error_code ec;
    if (!conf.ca_cert_file.empty()) {
      auto ca_cert_file = fs::path(conf.base_path).append(conf.ca_cert_file);
      if (file_exists(ca_cert_file)) {
        context.load_verify_file(ca_cert_file.string(), ec);
        if (ec) {
          ELOG_WARN << "failed to load CA certificate: "
                    << ec.message() << ", continuing without it";
        }
      }
    }

    // Set verification mode
    if (conf.enable_client_verify) {
      context.set_verify_mode(asio::ssl::verify_peer, ec);
    } else {
      context.set_verify_mode(asio::ssl::verify_none, ec);
    }
    if (ec) {
      ELOG_WARN << "failed to set verify mode: " << ec.message();
    }

    ELOG_INFO << "NTLS server context initialized successfully";
    return true;
  } catch (std::exception &e) {
    ELOG_ERROR << "NTLS context init error: " << e.what();
    return false;
  }
}

///*!
// * Initialize SSL Context with extended SSL/NTLS config
// *
// * @param context instance of asio::ssl::context
// * @param conf object of ssl_ntls_configure
// * @return true if init success, otherwise false
// */
// inline bool init_ssl_ntls_context_helper(asio::ssl::context &context,
//                                         const ssl_ntls_configure &conf) {
//  if (conf.enable_ntls) {
//    // Convert to ntls_configure and use NTLS initialization
//    ntls_configure ntls_conf;
//    ntls_conf.base_path = conf.base_path;
//    ntls_conf.sign_cert_file = conf.sign_cert_file;
//    ntls_conf.sign_key_file = conf.sign_key_file;
//    ntls_conf.enc_cert_file = conf.enc_cert_file;
//    ntls_conf.enc_key_file = conf.enc_key_file;
//    ntls_conf.ca_cert_file = conf.ca_cert_file;
//    ntls_conf.enable_client_verify = conf.enable_client_verify;
//    ntls_conf.enable_ntls = conf.enable_ntls;
//
//    return init_ntls_context_helper(context, ntls_conf);
//  } else {
//    // Use traditional SSL initialization
//    ssl_configure ssl_conf;
//    ssl_conf.base_path = conf.base_path;
//    ssl_conf.cert_file = conf.cert_file;
//    ssl_conf.key_file = conf.key_file;
//    ssl_conf.dh_file = conf.dh_file;
//
//    return init_ssl_context_helper(context, ssl_conf);
//  }
//}
//
///*!
// * Initialize SSL Context with TLCP method for NTLS
// *
// * @param conf object of ntls_configure
// * @return initialized asio::ssl::context for NTLS
// */
// inline std::unique_ptr<asio::ssl::context> create_ntls_context(const
// ntls_configure &conf) {
//  try {
//    // Create SSL context with TLCP server method
//    auto context =
//    std::make_unique<asio::ssl::context>(asio::ssl::context::tlcp_server);
//
//    if (init_ntls_context_helper(*context, conf)) {
//      return context;
//    }
//  } catch (const std::exception &e) {
//    ELOG_ERROR << "Failed to create NTLS context: " << e.what();
//  }
//  return nullptr;
//}
//
///*!
// * Initialize SSL Context with TLCP client method for NTLS
// *
// * @param conf object of ntls_configure
// * @return initialized asio::ssl::context for NTLS client
// */
// inline std::unique_ptr<asio::ssl::context> create_ntls_client_context(const
// ntls_configure &conf) {
//  try {
//    // Create SSL context with TLCP client method
//    auto context =
//    std::make_unique<asio::ssl::context>(asio::ssl::context::tlcp_client);
//
//    if (init_ntls_context_helper(*context, conf)) {
//      return context;
//    }
//  } catch (const std::exception &e) {
//    ELOG_ERROR << "Failed to create NTLS client context: " << e.what();
//  }
//  return nullptr;
//}
#endif  // OPENSSL_NO_NTLS
#endif
}  // namespace coro_rpc