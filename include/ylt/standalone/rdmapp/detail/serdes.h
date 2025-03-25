#pragma once

#include <algorithm>
#include <cstdint>
#include <endian.h>
#include <netinet/in.h>
#include <type_traits>

#include <infiniband/verbs.h>

namespace rdmapp {
namespace detail {

static inline uint16_t ntoh(uint16_t const &value) { return ::be16toh(value); }

static inline uint32_t ntoh(uint32_t const &value) { return ::be32toh(value); }

static inline uint64_t ntoh(uint64_t const &value) { return ::be64toh(value); }

static inline uint16_t hton(uint16_t const &value) { return ::htobe16(value); }

static inline uint32_t hton(uint32_t const &value) { return ::htobe32(value); }

static inline uint64_t hton(uint64_t const &value) { return ::htobe64(value); }

template <class T, class It>
typename std::enable_if<std::is_integral<T>::value>::type
serialize(T const &value, It &it) {
  T nvalue = hton(value);
  std::copy_n(reinterpret_cast<uint8_t *>(&nvalue), sizeof(T), it);
}

template <class T, class It>
typename std::enable_if<std::is_same<T, union ibv_gid>::value>::type
serialize(T const &value, It &it) {
  std::copy_n(reinterpret_cast<uint8_t const *>(&value), sizeof(T), it);
}

template <class T, class It>
typename std::enable_if<std::is_integral<T>::value>::type
deserialize(It &it, T &value) {
  std::copy_n(it, sizeof(T), reinterpret_cast<uint8_t *>(&value));
  it += sizeof(T);
  value = ntoh(value);
}

template <class T, class It>
typename std::enable_if<std::is_same<T, void *>::value>::type
deserialize(It &it, T &value) {
  std::copy_n(it, sizeof(T), reinterpret_cast<uint8_t *>(&value));
  it += sizeof(T);
  value = reinterpret_cast<void *>(ntoh(reinterpret_cast<uint64_t>(value)));
}

template <class T, class It>
typename std::enable_if<std::is_same<T, union ibv_gid>::value>::type
deserialize(It &it, T &value) {
  std::copy_n(it, sizeof(T), reinterpret_cast<uint8_t *>(&value));
  it += sizeof(T);
}

} // namespace detail
} // namespace rdmapp