#pragma once

#include <asio/buffer.hpp>
#include <asio/version.hpp>
#include <type_traits>

#if defined(ASIO_VERSION) && ASIO_VERSION >= 103600
namespace asio {
template <typename Pointer, typename Buffer>
inline Pointer buffer_cast(const Buffer &buffer) noexcept {
  static_assert(std::is_pointer_v<Pointer>);
  if constexpr (std::is_const_v<std::remove_pointer_t<Pointer>>) {
    return static_cast<Pointer>(buffer.data());
  }
  else {
    return static_cast<Pointer>(
        const_cast<void *>(static_cast<const void *>(buffer.data())));
  }
}
}  // namespace asio
#endif
