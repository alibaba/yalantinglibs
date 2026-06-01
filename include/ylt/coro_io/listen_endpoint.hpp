#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "asio/error_code.hpp"
#include "asio/ip/address.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/ip/v6_only.hpp"

namespace coro_io::detail {

inline bool is_ipv6_any_address(const std::string& address) {
  return address == "::" || address == "::0" || address == "::0.0.0.0" ||
         address == "0:0:0:0:0:0:0:0" || address == "[::]";
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
inline asio::error_code set_ipv6_only_false(
    asio::ip::tcp::acceptor& acceptor,
    const asio::ip::tcp::endpoint& endpoint) {
  asio::error_code ec;
  if (endpoint.protocol() == asio::ip::tcp::v6()) {
    acceptor.set_option(asio::ip::v6_only(false), ec);
  }
  return ec;
}

}  // namespace coro_io::detail
