#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include "asio/error_code.hpp"
#include "asio/ip/address.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/ip/v6_only.hpp"

namespace coro_io::detail {

struct listen_address {
  std::string address;
  uint16_t port = 0;
};

inline listen_address parse_listen_address(std::string_view address,
                                           uint16_t default_port = 0) {
  listen_address parsed{std::string(address), default_port};
  if (address.empty()) {
    return parsed;
  }

  if (address.front() == '[') {
    auto close = address.find(']');
    if (close != std::string_view::npos) {
      parsed.address = std::string(address.substr(1, close - 1));
      if (close + 1 < address.size() && address[close + 1] == ':') {
        auto port_sv = address.substr(close + 2);
        uint16_t port = 0;
        auto [ptr, ec] = std::from_chars(port_sv.data(),
                                         port_sv.data() + port_sv.size(), port);
        if (ec == std::errc{}) {
          parsed.port = port;
        }
      }
      return parsed;
    }
  }

  asio::error_code ec;
  (void)asio::ip::make_address(address, ec);
  if (!ec) {
    parsed.address = std::string(address);
    return parsed;
  }

  if (size_t pos = address.rfind(':'); pos != std::string_view::npos) {
    auto port_sv = address.substr(pos + 1);
    uint16_t port = 0;
    auto [ptr, parse_ec] =
        std::from_chars(port_sv.data(), port_sv.data() + port_sv.size(), port);
    if (parse_ec == std::errc{}) {
      parsed.address = std::string(address.substr(0, pos));
      parsed.port = port;
    }
  }

  return parsed;
}

inline bool is_ipv6_any_endpoint(const asio::ip::tcp::endpoint& endpoint) {
  return endpoint.address().is_v6() &&
         endpoint.address().to_v6().is_unspecified();
}

inline bool should_create_dual_stack_acceptor(
    const asio::ip::tcp::endpoint& endpoint) {
#if defined(__linux__)
  return is_ipv6_any_endpoint(endpoint);
#else
  (void)endpoint;
  return false;
#endif
}

// Resolve a string address into a TCP endpoint.
// - If `address` is a numeric IP literal (e.g. "::", "0.0.0.0", "127.0.0.1"),
//   it is parsed directly via `asio::ip::make_address` to avoid the ambiguity
//   of `getaddrinfo()` on different glibc versions / container environments.
// - Otherwise (e.g. "localhost"), falls back to `asio::ip::tcp::resolver`.
//
// On success `ec` is cleared and the resulting endpoint is returned.
// On failure `ec` carries the resolver error and `std::nullopt` is returned.
template <typename Executor>
inline std::optional<asio::ip::tcp::endpoint> resolve_listen_endpoint(
    const Executor& executor, const std::string& address, uint16_t port,
    asio::error_code& ec) {
  using asio::ip::tcp;

  auto addr = asio::ip::make_address(address, ec);
  if (!ec) {
    return tcp::endpoint(addr, port);
  }

  ec.clear();
  tcp::resolver resolver(executor);
  auto it =
      resolver.resolve(tcp::resolver::query(address, std::to_string(port)), ec);
  if (ec || it == tcp::resolver::iterator{}) {
    return std::nullopt;
  }

  return it->endpoint();
}

// Disable IPV6_V6ONLY on a TCP acceptor so that an IPv6 socket also accepts
// IPv4-mapped connections (i.e. dual-stack listening).
//
// Only applied when the endpoint protocol is IPv6; for IPv4 endpoints this is
// a no-op. The returned `error_code` is non-zero only when the socket option
// cannot be set (e.g. on platforms that disallow toggling V6ONLY at runtime).
// Callers are expected to log a warning on failure rather than abort, since
// the listen socket can still serve IPv6 connections.
enum class ipv6_only_mode {
  keep,
  enable,
  disable,
};

inline asio::error_code set_ipv6_only(asio::ip::tcp::acceptor& acceptor,
                                      const asio::ip::tcp::endpoint& endpoint,
                                      ipv6_only_mode mode) {
  asio::error_code ec;
  if (endpoint.protocol() == asio::ip::tcp::v6() &&
      mode != ipv6_only_mode::keep) {
    acceptor.set_option(asio::ip::v6_only(mode == ipv6_only_mode::enable), ec);
  }
  return ec;
}

inline asio::error_code set_ipv6_only_false(
    asio::ip::tcp::acceptor& acceptor,
    const asio::ip::tcp::endpoint& endpoint) {
  return set_ipv6_only(acceptor, endpoint, ipv6_only_mode::disable);
}

inline void close_acceptor_now(asio::ip::tcp::acceptor& acceptor) {
  asio::error_code ec;
  acceptor.cancel(ec);
  acceptor.close(ec);
}

inline asio::error_code init_tcp_acceptor(
    asio::ip::tcp::acceptor& acceptor, const asio::ip::tcp::endpoint& endpoint,
    ipv6_only_mode mode) {
  using asio::ip::tcp;
  asio::error_code ec;

  acceptor.open(endpoint.protocol(), ec);
  if (ec) {
    return ec;
  }
#ifdef __GNUC__
  acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
#endif
  if (auto opt_ec = set_ipv6_only(acceptor, endpoint, mode); opt_ec) {
    close_acceptor_now(acceptor);
    return opt_ec;
  }
  acceptor.bind(endpoint, ec);
  if (ec) {
    close_acceptor_now(acceptor);
    return ec;
  }
#ifdef _MSC_VER
  acceptor.set_option(tcp::acceptor::reuse_address(true));
#endif
  acceptor.listen(asio::socket_base::max_listen_connections, ec);
  if (ec) {
    close_acceptor_now(acceptor);
  }
  return ec;
}

inline asio::ip::tcp::endpoint make_ipv4_any_endpoint(uint16_t port) {
  return {asio::ip::make_address_v4("0.0.0.0"), port};
}

}  // namespace coro_io::detail
