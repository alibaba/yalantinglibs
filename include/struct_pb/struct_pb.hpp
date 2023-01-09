#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include "struct_pb/struct_pb/struct_pb_impl.hpp"
namespace struct_pb {

struct UnknownFields {
  [[nodiscard]] std::size_t total_size() const {
    std::size_t total = 0;
    for (const auto& f : fields) {
      total += f.len;
    }
    return total;
  }
  void serialize_to(char* data, std::size_t& pos, std::size_t size) const {
    for (const auto& f : fields) {
      assert(pos + f.len <= size);
      std::memcpy(data + pos, f.data, f.len);
      pos += f.len;
    }
  }
  void add_field(const char* data, std::size_t start, std::size_t end) {
    fields.push_back(Field{data + start, end - start});
  }
  struct Field {
    const char* data;
    std::size_t len;
  };
  std::vector<Field> fields;
};

inline bool deserialize_unknown(const char* data, std::size_t& pos,
                                std::size_t size, uint32_t tag,
                                UnknownFields& unknown_fields) {
  uint32_t field_number = tag >> 3;
  if (field_number == 0) {
    return false;
  }
  auto offset = internal::calculate_varint_size(tag);
  auto start = pos - offset;
  uint32_t wire_type = tag & 0b0000'0111;
  switch (wire_type) {
    case 0: {
      uint64_t t;
      auto ok = internal::deserialize_varint(data, pos, size, t);
      if (!ok) [[unlikely]] {
        return false;
      }
      unknown_fields.add_field(data, start, pos);
      break;
    }
    case 1: {
      static_assert(sizeof(double) == 8);
      if (pos + 8 > size) [[unlikely]] {
        return false;
      }
      pos += 8;
      unknown_fields.add_field(data, start, pos);
      break;
    }
    case 2: {
      uint64_t sz;
      auto ok = internal::deserialize_varint(data, pos, size, sz);
      if (!ok) [[unlikely]] {
        return false;
      }
      if (pos + sz > size) [[unlikely]] {
        return false;
      }
      pos += sz;
      unknown_fields.add_field(data, start, pos);
      break;
    }
    case 5: {
      static_assert(sizeof(float) == 4);
      if (pos + 4 > size) [[unlikely]] {
        return false;
      }
      pos += 4;
      unknown_fields.add_field(data, start, pos);
      break;
    }
    default: {
      assert(false && "error path");
    }
  }
  return true;
}

}  // namespace struct_pb
