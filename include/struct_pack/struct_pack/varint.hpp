#include <cstdint>
#include <ostream>
#include <system_error>
#include <type_traits>

#include "reflection.h"
#include "struct_pack/struct_pack/error_code.h"
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

template <typename T>
concept varintable_t =
    std::is_same_v<T, varint<int32_t>> || std::is_same_v<T, varint<int64_t>> ||
    std::is_same_v<T, varint<uint32_t>> || std::is_same_v<T, varint<uint64_t>>;
template <typename T>
concept sintable_t =
    std::is_same_v<T, sint<int32_t>> || std::is_same_v<T, sint<int64_t>>;

template <typename T>
concept varint_t = varintable_t<T> || sintable_t<T>;

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

template <typename charT, typename T>
STRUCT_PACK_INLINE void serialize_varint(charT* data, size_t& pos, const T& t) {
  uint64_t v;
  if constexpr (sintable_t<T>) {
    v = encode_zigzag(t.get());
  }
  else {
    v = t;
  }
  while (v >= 0x80) {
    data[pos++] = static_cast<uint8_t>(v | 0x80u);
    v >>= 7;
  }
  data[pos++] = static_cast<uint8_t>(v);
}
template <typename charT>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_varint_impl(
    charT* data, size_t& pos, size_t size, uint64_t& v) {
  if ((static_cast<uint64_t>(data[pos]) & 0x80U) == 0) {
    v = static_cast<uint64_t>(data[pos]);
    pos++;
    return {};
  }
  constexpr const int8_t max_varint_length = sizeof(uint64_t) * 8 / 7 + 1;
  uint64_t val = 0;
  if (size - pos >= max_varint_length) [[likely]] {
    do {
      // clang-format off
        int64_t b = data[pos++];
                           val  = ((uint64_t(b) & 0x7fU)       ); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x7fU) <<  7U); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x7fU) << 14U); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x7fU) << 21U); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x7fU) << 28U); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x7fU) << 35U); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x7fU) << 42U); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x7fU) << 49U); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x7fU) << 56U); if (b >= 0) { break; }
        b = data[pos++]; val |= ((uint64_t(b) & 0x01U) << 63U); if (b >= 0) { break; }
      // clang-format on
      return struct_pack::errc::invalid_argument;
    } while (false);
  }
  else {
    unsigned int shift = 0;
    while (pos != size && int64_t(data[pos]) < 0) {
      val |= (uint64_t(data[pos++]) & 0x7fU) << shift;
      shift += 7;
    }
    if (pos == size) {
      return struct_pack::errc::no_buffer_space;
    }
    val |= uint64_t(data[pos++]) << shift;
  }
  v = val;
  return struct_pack::errc{};
}
template <typename charT, typename T>
[[nodiscard]] STRUCT_PACK_INLINE struct_pack::errc deserialize_varint(
    charT* data, size_t& pos, size_t size, T& t) {
  uint64_t v;
  auto ec = deserialize_varint_impl(data, pos, size, v);
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
  return ec;
  // Variable-width integers, or varints,
  // are at the core of the wire format.
  // They allow encoding unsigned 64-bit integers using anywhere
  // between one and ten bytes, with small values using fewer bytes.
  // return decode_varint_v1(f);
}
}  // namespace detail

}  // namespace struct_pack
