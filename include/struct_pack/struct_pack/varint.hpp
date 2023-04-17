#pragma once
#include <cstdint>
#include <ostream>
#include <system_error>
#include <type_traits>

#include "struct_pack/struct_pack/reflection.hpp"
namespace struct_pack {

namespace detail {

template <typename T>
class varint {
 public:
  using value_type = T;
  varint() noexcept = default;
  varint(T t) noexcept : val(t) {}
  [[nodiscard]] operator T() const noexcept { return val; }
  auto& operator=(T t) noexcept {
    val = t;
    return *this;
  }
  [[nodiscard]] auto operator<=>(const varint&) const noexcept = default;
  [[nodiscard]] bool operator==(const varint<T>& t) const noexcept {
    return val == t.val;
  }
  [[nodiscard]] bool operator==(T t) const noexcept { return val == t; }
  [[nodiscard]] auto operator&(uint8_t mask) const noexcept {
    T new_val = val & mask;
    return varint(new_val);
  }
  template <std::unsigned_integral U>
  [[nodiscard]] auto operator<<(U shift) const noexcept {
    T new_val = val << shift;
    return varint(new_val);
  }
  template <typename U>
  [[nodiscard]] auto operator|=(U shift) noexcept {
    if constexpr (std::same_as<U, varint<T>>) {
      val |= shift.val;
    }
    else {
      val |= shift;
    }
    return *this;
  }
  friend std::ostream& operator<<(std::ostream& os, const varint& varint) {
    os << varint.val;
    return os;
  }

  const T& get() const noexcept { return val; }
  T& get() noexcept { return val; }

 private:
  T val;
};

template <typename T>
class sint {
 public:
  using value_type = T;
  sint() noexcept = default;
  sint(T t) noexcept : val(t) {}
  [[nodiscard]] operator T() const noexcept { return val; }
  auto& operator=(T t) noexcept {
    val = t;
    return *this;
  }
  [[nodiscard]] auto operator<=>(const sint<T>&) const noexcept = default;
  [[nodiscard]] bool operator==(T t) const noexcept { return val == t; }
  [[nodiscard]] bool operator==(const sint& t) const noexcept {
    return val == t.val;
  }
  [[nodiscard]] auto operator&(uint8_t mask) const noexcept {
    T new_val = val & mask;
    return sint(new_val);
  }
  template <std::unsigned_integral U>
  auto operator<<(U shift) const noexcept {
    T new_val = val << shift;
    return sint(new_val);
  }
  const T& get() const noexcept { return val; }
  T& get() noexcept { return val; }
  friend std::ostream& operator<<(std::ostream& os, const sint& t) {
    os << t.val;
    return os;
  }

 private:
  T val;
};

template <typename T, typename U>
[[nodiscard]] STRUCT_PACK_INLINE T decode_zigzag(U u) {
  return static_cast<T>((u >> 1U)) ^ static_cast<U>(-static_cast<T>(u & 1U));
}

template <typename U, typename T, unsigned Shift>
[[nodiscard]] STRUCT_PACK_INLINE U encode_zigzag(T t) {
  return (static_cast<U>(t) << 1U) ^
         static_cast<U>(-static_cast<T>(static_cast<U>(t) >> Shift));
}
template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE auto encode_zigzag(T t) {
  if constexpr (std::is_same_v<T, int32_t>) {
    return encode_zigzag<uint32_t, int32_t, 31U>(t);
  }
  else if constexpr (std::is_same_v<T, int64_t>) {
    return encode_zigzag<uint64_t, int64_t, 63U>(t);
  }
  else {
    static_assert(!sizeof(T), "error zigzag type");
  }
}

template <typename T>
[[nodiscard]] STRUCT_PACK_INLINE constexpr int calculate_varint_size(T t) {
  if constexpr (!varint_t<T>) {
    int ret = 0;
    if constexpr (std::is_unsigned_v<T>) {
      do {
        ret++;
        t >>= 7;
      } while (t != 0);
    }
    else {
      uint64_t temp = t;
      do {
        ret++;
        temp >>= 7;
      } while (temp != 0);
    }
    return ret;
  }
  else if constexpr (varintable_t<T>) {
    return calculate_varint_size(t.get());
  }
  else if constexpr (sintable_t<T>) {
    return calculate_varint_size(encode_zigzag(t.get()));
  }
  else {
    static_assert(!sizeof(T), "error type");
  }
}

template <writer_t writer, typename T>
STRUCT_PACK_INLINE void serialize_varint(writer& writer_, const T& t) {
  uint64_t v;
  if constexpr (sintable_t<T>) {
    v = encode_zigzag(t.get());
  }
  else {
    v = t;
  }
  while (v >= 0x80) {
    uint8_t temp = v | 0x80u;
    writer_.write((char*)&temp, sizeof(temp));
    v >>= 7;
  }
  writer_.write((char*)&v, sizeof(char));
}
template <reader_t Reader>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_varint_impl(
    Reader& reader, uint64_t& v) {
  uint8_t now;
  constexpr const int8_t max_varint_length = sizeof(uint64_t) * 8 / 7 + 1;
  for (int8_t i = 0; i < max_varint_length; ++i) {
    if (!reader.read((char*)&now, sizeof(char))) [[unlikely]] {
      return struct_pack::errc::no_buffer_space;
    }
    v |= (1ull * (now & 0x7fu)) << (i * 7);
    if ((now & 0x80U) == 0) {
      return {};
    }
  }
  return struct_pack::errc::invalid_buffer;
}
template <bool NotSkip = true, reader_t Reader, typename T>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_varint(
    Reader& reader, T& t) {
  uint64_t v = 0;
  auto ec = deserialize_varint_impl(reader, v);
  if constexpr (NotSkip) {
    if (ec == struct_pack::errc{}) [[likely]] {
      if constexpr (sintable_t<T>) {
        t = decode_zigzag<int64_t>(v);
      }
      else if constexpr (std::is_enum_v<T>) {
        t = static_cast<T>(v);
      }
      else {
        t = v;
      }
    }
  }
  return ec;
  // Variable-width integers, or varints,
  // are at the core of the wire format.
  // They allow encoding unsigned 64-bit integers using anywhere
  // between one and ten bytes, with small values using fewer bytes.
  // return decode_varint_v1(f);
}
}  // namespace detail

}  // namespace struct_pack
