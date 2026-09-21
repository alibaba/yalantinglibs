#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>

#include "cinatra_log_wrapper.hpp"

namespace cinatra {
namespace detail {

inline constexpr uint64_t rotate_left(uint64_t value, int bits) noexcept {
  return (value << bits) | (value >> (64 - bits));
}

inline uint64_t load_little_endian(const unsigned char *data) noexcept {
  uint64_t value = 0;
  for (size_t i = 0; i < 8; ++i) {
    value |= static_cast<uint64_t>(data[i]) << (i * 8);
  }
  return value;
}

inline uint64_t sip_hash_2_4(std::string_view value, uint64_t key0,
                             uint64_t key1) noexcept {
  uint64_t v0 = UINT64_C(0x736f6d6570736575) ^ key0;
  uint64_t v1 = UINT64_C(0x646f72616e646f6d) ^ key1;
  uint64_t v2 = UINT64_C(0x6c7967656e657261) ^ key0;
  uint64_t v3 = UINT64_C(0x7465646279746573) ^ key1;

  auto sip_round = [&]() {
    v0 += v1;
    v1 = rotate_left(v1, 13);
    v1 ^= v0;
    v0 = rotate_left(v0, 32);
    v2 += v3;
    v3 = rotate_left(v3, 16);
    v3 ^= v2;
    v0 += v3;
    v3 = rotate_left(v3, 21);
    v3 ^= v0;
    v2 += v1;
    v1 = rotate_left(v1, 17);
    v1 ^= v2;
    v2 = rotate_left(v2, 32);
  };

  const auto *data = reinterpret_cast<const unsigned char *>(value.data());
  size_t remaining = value.size();
  while (remaining >= 8) {
    uint64_t message = load_little_endian(data);
    v3 ^= message;
    sip_round();
    sip_round();
    v0 ^= message;
    data += 8;
    remaining -= 8;
  }

  uint64_t tail = static_cast<uint64_t>(value.size()) << 56;
  for (size_t i = 0; i < remaining; ++i) {
    tail |= static_cast<uint64_t>(data[i]) << (i * 8);
  }

  v3 ^= tail;
  sip_round();
  sip_round();
  v0 ^= tail;
  v2 ^= 0xff;
  sip_round();
  sip_round();
  sip_round();
  sip_round();
  return v0 ^ v1 ^ v2 ^ v3;
}

inline uint64_t splitmix64(uint64_t &state) noexcept {
  uint64_t value = (state += UINT64_C(0x9e3779b97f4a7c15));
  value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31);
}

inline std::array<uint64_t, 2> make_sip_hash_key() noexcept {
  static int address_entropy;
  uint64_t seed = static_cast<uint64_t>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  seed ^=
      static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(&address_entropy));

  try {
    std::random_device random;
    for (size_t i = 0; i < 8; ++i) {
      seed ^= static_cast<uint64_t>(random()) << ((i % 2) * 32);
      splitmix64(seed);
    }
  } catch (...) {
    CINATRA_LOG_ERROR
        << "std::random_device failed; using process-specific SipHash key";
  }

  return {splitmix64(seed), splitmix64(seed)};
}

inline const std::array<uint64_t, 2> &sip_hash_key() noexcept {
  static const auto key = make_sip_hash_key();
  return key;
}

}  // namespace detail

struct secure_string_hash {
  using is_transparent = void;

  secure_string_hash() noexcept
      : key0_(detail::sip_hash_key()[0]), key1_(detail::sip_hash_key()[1]) {}

  size_t operator()(std::string_view value) const noexcept {
    uint64_t hash = detail::sip_hash_2_4(value, key0_, key1_);
    if constexpr (sizeof(size_t) < sizeof(hash)) {
      return static_cast<size_t>(hash ^ (hash >> 32));
    }
    else {
      return static_cast<size_t>(hash);
    }
  }

  size_t operator()(const std::string &value) const noexcept {
    return (*this)(std::string_view(value));
  }

 private:
  uint64_t key0_;
  uint64_t key1_;
};

}  // namespace cinatra
