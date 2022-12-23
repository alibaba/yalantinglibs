/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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
#include <asio.hpp>

namespace asio_util {
template <typename Socket, typename AsioBuffer>
std::pair<asio::error_code, size_t> read_some(Socket &sock,
                                              AsioBuffer &&buffer) {
  asio::error_code error;
  size_t length = sock.read_some(std::forward<AsioBuffer>(buffer), error);
  return std::make_pair(error, length);
}

template <typename Socket, typename AsioBuffer>
std::pair<asio::error_code, size_t> read(Socket &sock, AsioBuffer &&buffer) {
  asio::error_code error;
  size_t length = asio::read(sock, buffer, error);
  return std::make_pair(error, length);
}

template <typename Socket, typename AsioBuffer>
std::pair<asio::error_code, size_t> write(Socket &sock, AsioBuffer &&buffer) {
  asio::error_code error;
  auto length = asio::write(sock, std::forward<AsioBuffer>(buffer), error);
  return std::make_pair(error, length);
}

inline std::error_code accept(asio::ip::tcp::acceptor &a,
                              asio::ip::tcp::socket &socket) {
  std::error_code error;
  a.accept(socket, error);
  return error;
}

inline std::error_code connect(asio::io_context &io_context,
                               asio::ip::tcp::socket &socket,
                               const std::string &host,
                               const std::string &port) {
  asio::ip::tcp::resolver resolver(io_context);
  auto endpoints = resolver.resolve(host, port);
  std::error_code error;
  asio::connect(socket, endpoints, error);
  return error;
}
}  // namespace asio_util
