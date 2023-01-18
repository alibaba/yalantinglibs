// protoc generate parameter
// clang-format off
// 
// =========================
#include "test_bad_identifiers.struct_pb.h"
#include "struct_pb/struct_pb/struct_pb_impl.hpp"
namespace struct_pb {
namespace internal {
// ::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::BuildDescriptors& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::TypeTraits
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::TypeTraits& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::TypeTraits& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::TypeTraits& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::TypeTraits& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::TypeTraits& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::TypeTraits&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::TypeTraits
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::TypeTraits& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::Data1
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data1& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.data.empty()) {

    std::size_t container_total = 0;
    for(const auto& e: t.data) {
      container_total += 1;
      container_total += calculate_varint_size(e);
    }
    total += container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data1& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data1& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<int32_t> data; // int32, field number = 1
    for(const auto& v: t.data) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(v));}
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data1& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data1& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<int32_t> data; // int32, field number = 1
      case 8: {
        int32_t e{};

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        e = varint_tmp;
        t.data.push_back(e);
        break;
      }
      case 10: {
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        while (pos < cur_max_size) {
          int32_t e{};

          uint64_t varint_tmp = 0;
          ok = deserialize_varint(data, pos, size, varint_tmp);
          if (!ok) {
            return false;
          }

          e = varint_tmp;
          t.data.push_back(e);
        }
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data1&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::Data1
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data1& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::Data2
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data2& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();

  if (!t.data.empty()) { // unpacked

  std::size_t sz = 0;
  for(const auto& v: t.data) {
    sz += 1;
    sz += calculate_varint_size(static_cast<uint64_t>(v));
  }

    total += sz;
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data2& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data2& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<::protobuf_unittest::TestConflictingSymbolNames::TestEnum> data; // enum, field number = 1

    if (!t.data.empty()) {

    for(const auto& v: t.data) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(v));
    }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data2& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data2& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<::protobuf_unittest::TestConflictingSymbolNames::TestEnum> data; // enum, field number = 1

      case 10: {
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_sz = pos + sz;
        if (cur_max_sz > size) {
          return false;
        }
        while (pos < cur_max_sz) {
          uint64_t enum_val_tmp{};
          ok = deserialize_varint(data, pos, cur_max_sz, enum_val_tmp);
          if (!ok) {
            return false;
          }
          t.data.push_back(static_cast<::protobuf_unittest::TestConflictingSymbolNames::TestEnum>(enum_val_tmp));
        }
        break;
      }

      case 8: {
        uint64_t enum_val_tmp{};
        ok = deserialize_varint(data, pos, size, enum_val_tmp);
        if (!ok) {
          return false;
        }
        t.data.push_back(static_cast<::protobuf_unittest::TestConflictingSymbolNames::TestEnum>(enum_val_tmp));
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data2&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::Data2
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data2& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::Data3
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data3& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  for (const auto& s: t.data) {
    total += 1 + calculate_varint_size(s.size()) + s.size();
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data3& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data3& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<std::string> data; // string, field number = 1
    if (!t.data.empty()) {
      for(const auto& s: t.data) {
        serialize_varint(data, pos, size, 10);
        serialize_varint(data, pos, size, s.size());
        std::memcpy(data + pos, s.data(), s.size());
        pos += s.size();
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data3& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data3& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<std::string> data; // string, field number = 1
      case 10: {
        std::string tmp_str;
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        tmp_str.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(tmp_str.data(), data+pos, sz);
        pos += sz;
        t.data.push_back(std::move(tmp_str));
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data3&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::Data3
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data3& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::Data4
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data4& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.data.empty()) {

    for(const auto& e: t.data) {
      std::size_t sz = get_needed_size(e);
      total += 1 + calculate_varint_size(sz) + sz;
    }
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data4& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data4& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<::protobuf_unittest::TestConflictingSymbolNames::Data4> data; // message, field number = 1
    if (!t.data.empty()) {

      for(const auto& e: t.data) {
        serialize_varint(data, pos, size, 10);
        std::size_t sz = get_needed_size(e);
        serialize_varint(data, pos, size, sz);
        serialize_to(data+pos, sz, e);
        pos += sz;
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data4& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data4& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<::protobuf_unittest::TestConflictingSymbolNames::Data4> data; // message, field number = 1
      case 10: {
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.data.emplace_back();
        ok = deserialize_to(t.data.back(), data + pos, sz);
        if (!ok) {
          t.data.pop_back();
          return false;
        }
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data4&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::Data4
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data4& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::Data5
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data5& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  for (const auto& s: t.data) {
    total += 1 + calculate_varint_size(s.size()) + s.size();
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data5& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data5& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<std::string> data; // string, field number = 1
    if (!t.data.empty()) {
      for(const auto& s: t.data) {
        serialize_varint(data, pos, size, 10);
        serialize_varint(data, pos, size, s.size());
        std::memcpy(data + pos, s.data(), s.size());
        pos += s.size();
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data5& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data5& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<std::string> data; // string, field number = 1
      case 10: {
        std::string tmp_str;
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        tmp_str.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(tmp_str.data(), data+pos, sz);
        pos += sz;
        t.data.push_back(std::move(tmp_str));
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data5&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::Data5
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data5& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::Data6
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data6& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  for (const auto& s: t.data) {
    total += 1 + calculate_varint_size(s.size()) + s.size();
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Data6& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data6& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<std::string> data; // string, field number = 1
    if (!t.data.empty()) {
      for(const auto& s: t.data) {
        serialize_varint(data, pos, size, 10);
        serialize_varint(data, pos, size, s.size());
        std::memcpy(data + pos, s.data(), s.size());
        pos += s.size();
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Data6& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data6& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<std::string> data; // string, field number = 1
      case 10: {
        std::string tmp_str;
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        tmp_str.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(tmp_str.data(), data+pos, sz);
        pos += sz;
        t.data.push_back(std::move(tmp_str));
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data6&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::Data6
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Data6& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::Cord
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Cord& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::Cord& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Cord& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::Cord& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Cord& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Cord&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::Cord
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::Cord& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::StringPiece
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::StringPiece& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::StringPiece& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::StringPiece& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::StringPiece& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::StringPiece& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::StringPiece&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::StringPiece
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::StringPiece& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames::DO
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::DO& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames::DO& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::DO& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames::DO& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::DO& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::DO&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames::DO
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames::DO& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNames
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.input.has_value()) {
    total += 1 + calculate_varint_size(t.input.value());
  }
  if (t.output.has_value()) {
    total += 1 + calculate_varint_size(t.output.value());
  }
  if (t.length.has_value()) {
    total += 1 + calculate_varint_size(t.length->size()) + t.length->size();
  }
  if (!t.i.empty()) {

    std::size_t container_total = 0;
    for(const auto& e: t.i) {
      container_total += 1;
      container_total += calculate_varint_size(e);
    }
    total += container_total;
  }
  for (const auto& s: t.new_element) {
    total += 1 + calculate_varint_size(s.size()) + s.size();
  }
  if (t.total_size.has_value()) {
    total += 1 + calculate_varint_size(t.total_size.value());
  }
  if (t.tag.has_value()) {
    total += 1 + calculate_varint_size(t.tag.value());
  }
  if (t.source.has_value()) {
    total += 1 + calculate_varint_size(t.source.value());
  }
  if (t.value.has_value()) {
    total += 1 + calculate_varint_size(t.value.value());
  }
  if (t.file.has_value()) {
    total += 1 + calculate_varint_size(t.file.value());
  }
  if (t.from.has_value()) {
    total += 1 + calculate_varint_size(t.from.value());
  }
  if (t.handle_uninterpreted.has_value()) {
    total += 1 + calculate_varint_size(t.handle_uninterpreted.value());
  }
  if (!t.index.empty()) {

    std::size_t container_total = 0;
    for(const auto& e: t.index) {
      container_total += 1;
      container_total += calculate_varint_size(e);
    }
    total += container_total;
  }
  if (t.controller.has_value()) {
    total += 1 + calculate_varint_size(t.controller.value());
  }
  if (t.already_here.has_value()) {
    total += 1 + calculate_varint_size(t.already_here.value());
  }
  if (t.uint32.has_value()) {
    total += 2 + calculate_varint_size(t.uint32.value());
  }
  if (t.uint32_t_.has_value()) {
    total += 2 + calculate_varint_size(t.uint32_t_.value());
  }
  if (t.uint64.has_value()) {
    total += 2 + calculate_varint_size(t.uint64.value());
  }
  if (t.uint64_t_.has_value()) {
    total += 2 + calculate_varint_size(t.uint64_t_.value());
  }
  if (t.string.has_value()) {
    total += 2 + calculate_varint_size(t.string->size()) + t.string->size();
  }
  if (t.memset_.has_value()) {
    total += 2 + calculate_varint_size(t.memset_.value());
  }
  if (t.int32.has_value()) {
    total += 2 + calculate_varint_size(t.int32.value());
  }
  if (t.int32_t_.has_value()) {
    total += 2 + calculate_varint_size(t.int32_t_.value());
  }
  if (t.int64.has_value()) {
    total += 2 + calculate_varint_size(t.int64.value());
  }
  if (t.int64_t_.has_value()) {
    total += 2 + calculate_varint_size(t.int64_t_.value());
  }
  if (t.size_t_.has_value()) {
    total += 2 + calculate_varint_size(t.size_t_.value());
  }
  if (t.cached_size.has_value()) {
    total += 2 + calculate_varint_size(t.cached_size.value());
  }
  if (t.extensions.has_value()) {
    total += 2 + calculate_varint_size(t.extensions.value());
  }
  if (t.bit.has_value()) {
    total += 2 + calculate_varint_size(t.bit.value());
  }
  if (t.bits.has_value()) {
    total += 2 + calculate_varint_size(t.bits.value());
  }
  if (t.offsets.has_value()) {
    total += 2 + calculate_varint_size(t.offsets.value());
  }
  if (t.reflection.has_value()) {
    total += 2 + calculate_varint_size(t.reflection.value());
  }
  if (t.some_cord.has_value()) {
    total += 2 + calculate_varint_size(t.some_cord->size()) + t.some_cord->size();
  }
  if (t.some_string_piece.has_value()) {
    total += 2 + calculate_varint_size(t.some_string_piece->size()) + t.some_string_piece->size();
  }
  if (t.int_.has_value()) {
    total += 2 + calculate_varint_size(t.int_.value());
  }
  if (t.friend_.has_value()) {
    total += 2 + calculate_varint_size(t.friend_.value());
  }
  if (t.class_.has_value()) {
    total += 2 + calculate_varint_size(t.class_.value());
  }
  if (t.typedecl.has_value()) {
    total += 2 + calculate_varint_size(t.typedecl.value());
  }
  if (t.auto_.has_value()) {
    total += 2 + calculate_varint_size(t.auto_.value());
  }

  if (t.do_) {
    auto sz = get_needed_size(*t.do_);
    total += 2 + calculate_varint_size(sz) + sz;
  }
  if (t.field_type.has_value()) {
    total += 2 + calculate_varint_size(t.field_type.value());
  }
  if (t.is_packed.has_value()) {
    total += 2 + calculate_varint_size(static_cast<uint64_t>(t.is_packed.value()));
  }
  if (t.release_length.has_value()) {
    total += 2 + calculate_varint_size(t.release_length->size()) + t.release_length->size();
  }

  if (t.release_do) {
    auto sz = get_needed_size(*t.release_do);
    total += 2 + calculate_varint_size(sz) + sz;
  }
  if (t.target.has_value()) {
    total += 2 + calculate_varint_size(t.target->size()) + t.target->size();
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNames& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::optional<int32_t> input; // int32, field number = 1
    if (t.input.has_value()) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.input.value()));
    }
  }
  {
    // std::optional<int32_t> output; // int32, field number = 2
    if (t.output.has_value()) {
      serialize_varint(data, pos, size, 16);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.output.value()));
    }
  }
  {
    // std::optional<std::string> length; // string, field number = 3
    if (t.length.has_value()) {
      serialize_varint(data, pos, size, 26);
      serialize_varint(data, pos, size, t.length.value().size());
      std::memcpy(data + pos, t.length.value().data(), t.length.value().size());
      pos += t.length.value().size();
    }
  }
  {
    // std::vector<int32_t> i; // int32, field number = 4
    for(const auto& v: t.i) {
      serialize_varint(data, pos, size, 32);
      serialize_varint(data, pos, size, static_cast<uint64_t>(v));}
  }
  {
    // std::vector<std::string> new_element; // string, field number = 5
    if (!t.new_element.empty()) {
      for(const auto& s: t.new_element) {
        serialize_varint(data, pos, size, 42);
        serialize_varint(data, pos, size, s.size());
        std::memcpy(data + pos, s.data(), s.size());
        pos += s.size();
      }
    }
  }
  {
    // std::optional<int32_t> total_size; // int32, field number = 6
    if (t.total_size.has_value()) {
      serialize_varint(data, pos, size, 48);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.total_size.value()));
    }
  }
  {
    // std::optional<int32_t> tag; // int32, field number = 7
    if (t.tag.has_value()) {
      serialize_varint(data, pos, size, 56);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.tag.value()));
    }
  }
  {
    // std::optional<int32_t> source; // int32, field number = 8
    if (t.source.has_value()) {
      serialize_varint(data, pos, size, 64);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.source.value()));
    }
  }
  {
    // std::optional<int32_t> value; // int32, field number = 9
    if (t.value.has_value()) {
      serialize_varint(data, pos, size, 72);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.value.value()));
    }
  }
  {
    // std::optional<int32_t> file; // int32, field number = 10
    if (t.file.has_value()) {
      serialize_varint(data, pos, size, 80);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.file.value()));
    }
  }
  {
    // std::optional<int32_t> from; // int32, field number = 11
    if (t.from.has_value()) {
      serialize_varint(data, pos, size, 88);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.from.value()));
    }
  }
  {
    // std::optional<int32_t> handle_uninterpreted; // int32, field number = 12
    if (t.handle_uninterpreted.has_value()) {
      serialize_varint(data, pos, size, 96);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.handle_uninterpreted.value()));
    }
  }
  {
    // std::vector<int32_t> index; // int32, field number = 13
    for(const auto& v: t.index) {
      serialize_varint(data, pos, size, 104);
      serialize_varint(data, pos, size, static_cast<uint64_t>(v));}
  }
  {
    // std::optional<int32_t> controller; // int32, field number = 14
    if (t.controller.has_value()) {
      serialize_varint(data, pos, size, 112);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.controller.value()));
    }
  }
  {
    // std::optional<int32_t> already_here; // int32, field number = 15
    if (t.already_here.has_value()) {
      serialize_varint(data, pos, size, 120);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.already_here.value()));
    }
  }
  {
    // std::optional<uint32_t> uint32; // uint32, field number = 16
    if (t.uint32.has_value()) {
      serialize_varint(data, pos, size, 128);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.uint32.value()));
    }
  }
  {
    // std::optional<uint64_t> uint64; // uint64, field number = 17
    if (t.uint64.has_value()) {
      serialize_varint(data, pos, size, 136);
      serialize_varint(data, pos, size, t.uint64.value());
    }
  }
  {
    // std::optional<std::string> string; // string, field number = 18
    if (t.string.has_value()) {
      serialize_varint(data, pos, size, 146);
      serialize_varint(data, pos, size, t.string.value().size());
      std::memcpy(data + pos, t.string.value().data(), t.string.value().size());
      pos += t.string.value().size();
    }
  }
  {
    // std::optional<int32_t> memset_; // int32, field number = 19
    if (t.memset_.has_value()) {
      serialize_varint(data, pos, size, 152);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.memset_.value()));
    }
  }
  {
    // std::optional<int32_t> int32; // int32, field number = 20
    if (t.int32.has_value()) {
      serialize_varint(data, pos, size, 160);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.int32.value()));
    }
  }
  {
    // std::optional<int64_t> int64; // int64, field number = 21
    if (t.int64.has_value()) {
      serialize_varint(data, pos, size, 168);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.int64.value()));
    }
  }
  {
    // std::optional<uint32_t> cached_size; // uint32, field number = 22
    if (t.cached_size.has_value()) {
      serialize_varint(data, pos, size, 176);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.cached_size.value()));
    }
  }
  {
    // std::optional<uint32_t> extensions; // uint32, field number = 23
    if (t.extensions.has_value()) {
      serialize_varint(data, pos, size, 184);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.extensions.value()));
    }
  }
  {
    // std::optional<uint32_t> bit; // uint32, field number = 24
    if (t.bit.has_value()) {
      serialize_varint(data, pos, size, 192);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.bit.value()));
    }
  }
  {
    // std::optional<uint32_t> bits; // uint32, field number = 25
    if (t.bits.has_value()) {
      serialize_varint(data, pos, size, 200);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.bits.value()));
    }
  }
  {
    // std::optional<uint32_t> offsets; // uint32, field number = 26
    if (t.offsets.has_value()) {
      serialize_varint(data, pos, size, 208);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.offsets.value()));
    }
  }
  {
    // std::optional<uint32_t> reflection; // uint32, field number = 27
    if (t.reflection.has_value()) {
      serialize_varint(data, pos, size, 216);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.reflection.value()));
    }
  }
  {
    // std::optional<std::string> some_cord; // string, field number = 28
    if (t.some_cord.has_value()) {
      serialize_varint(data, pos, size, 226);
      serialize_varint(data, pos, size, t.some_cord.value().size());
      std::memcpy(data + pos, t.some_cord.value().data(), t.some_cord.value().size());
      pos += t.some_cord.value().size();
    }
  }
  {
    // std::optional<std::string> some_string_piece; // string, field number = 29
    if (t.some_string_piece.has_value()) {
      serialize_varint(data, pos, size, 234);
      serialize_varint(data, pos, size, t.some_string_piece.value().size());
      std::memcpy(data + pos, t.some_string_piece.value().data(), t.some_string_piece.value().size());
      pos += t.some_string_piece.value().size();
    }
  }
  {
    // std::optional<uint32_t> int_; // uint32, field number = 30
    if (t.int_.has_value()) {
      serialize_varint(data, pos, size, 240);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.int_.value()));
    }
  }
  {
    // std::optional<uint32_t> friend_; // uint32, field number = 31
    if (t.friend_.has_value()) {
      serialize_varint(data, pos, size, 248);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.friend_.value()));
    }
  }
  {
    // std::unique_ptr<::protobuf_unittest::TestConflictingSymbolNames::DO> do_; // message, field number = 32

    if (t.do_) {
      serialize_varint(data, pos, size, 258);
      auto sz = get_needed_size(*t.do_);
      serialize_varint(data, pos, size, sz);
      serialize_to(data + pos, sz, *t.do_);
      pos += sz;
    }
  }
  {
    // std::optional<int32_t> field_type; // int32, field number = 33
    if (t.field_type.has_value()) {
      serialize_varint(data, pos, size, 264);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.field_type.value()));
    }
  }
  {
    // std::optional<bool> is_packed; // bool, field number = 34
    if (t.is_packed.has_value()) {
      serialize_varint(data, pos, size, 272);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.is_packed.value()));
    }
  }
  {
    // std::optional<std::string> release_length; // string, field number = 35
    if (t.release_length.has_value()) {
      serialize_varint(data, pos, size, 282);
      serialize_varint(data, pos, size, t.release_length.value().size());
      std::memcpy(data + pos, t.release_length.value().data(), t.release_length.value().size());
      pos += t.release_length.value().size();
    }
  }
  {
    // std::unique_ptr<::protobuf_unittest::TestConflictingSymbolNames::DO> release_do; // message, field number = 36

    if (t.release_do) {
      serialize_varint(data, pos, size, 290);
      auto sz = get_needed_size(*t.release_do);
      serialize_varint(data, pos, size, sz);
      serialize_to(data + pos, sz, *t.release_do);
      pos += sz;
    }
  }
  {
    // std::optional<uint32_t> class_; // uint32, field number = 37
    if (t.class_.has_value()) {
      serialize_varint(data, pos, size, 296);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.class_.value()));
    }
  }
  {
    // std::optional<std::string> target; // string, field number = 38
    if (t.target.has_value()) {
      serialize_varint(data, pos, size, 306);
      serialize_varint(data, pos, size, t.target.value().size());
      std::memcpy(data + pos, t.target.value().data(), t.target.value().size());
      pos += t.target.value().size();
    }
  }
  {
    // std::optional<uint32_t> typedecl; // uint32, field number = 39
    if (t.typedecl.has_value()) {
      serialize_varint(data, pos, size, 312);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.typedecl.value()));
    }
  }
  {
    // std::optional<uint32_t> auto_; // uint32, field number = 40
    if (t.auto_.has_value()) {
      serialize_varint(data, pos, size, 320);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.auto_.value()));
    }
  }
  {
    // std::optional<uint32_t> uint32_t_; // uint32, field number = 41
    if (t.uint32_t_.has_value()) {
      serialize_varint(data, pos, size, 328);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.uint32_t_.value()));
    }
  }
  {
    // std::optional<uint32_t> uint64_t_; // uint32, field number = 42
    if (t.uint64_t_.has_value()) {
      serialize_varint(data, pos, size, 336);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.uint64_t_.value()));
    }
  }
  {
    // std::optional<int32_t> int32_t_; // int32, field number = 43
    if (t.int32_t_.has_value()) {
      serialize_varint(data, pos, size, 344);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.int32_t_.value()));
    }
  }
  {
    // std::optional<int64_t> int64_t_; // int64, field number = 44
    if (t.int64_t_.has_value()) {
      serialize_varint(data, pos, size, 352);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.int64_t_.value()));
    }
  }
  {
    // std::optional<int64_t> size_t_; // int64, field number = 45
    if (t.size_t_.has_value()) {
      serialize_varint(data, pos, size, 360);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.size_t_.value()));
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNames& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::optional<int32_t> input; // int32, field number = 1
      case 8: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.input = varint_tmp;
        break;
      }
      // std::optional<int32_t> output; // int32, field number = 2
      case 16: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.output = varint_tmp;
        break;
      }
      // std::optional<std::string> length; // string, field number = 3
      case 26: {
        if (!t.length.has_value()) {
          t.length = std::string();
        }
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.length.value().resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.length.value().data(), data+pos, sz);
        pos += sz;
        break;
      }
      // std::vector<int32_t> i; // int32, field number = 4
      case 32: {
        int32_t e{};

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        e = varint_tmp;
        t.i.push_back(e);
        break;
      }
      case 34: {
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        while (pos < cur_max_size) {
          int32_t e{};

          uint64_t varint_tmp = 0;
          ok = deserialize_varint(data, pos, size, varint_tmp);
          if (!ok) {
            return false;
          }

          e = varint_tmp;
          t.i.push_back(e);
        }
        break;
      }
      // std::vector<std::string> new_element; // string, field number = 5
      case 42: {
        std::string tmp_str;
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        tmp_str.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(tmp_str.data(), data+pos, sz);
        pos += sz;
        t.new_element.push_back(std::move(tmp_str));
        break;
      }
      // std::optional<int32_t> total_size; // int32, field number = 6
      case 48: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.total_size = varint_tmp;
        break;
      }
      // std::optional<int32_t> tag; // int32, field number = 7
      case 56: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.tag = varint_tmp;
        break;
      }
      // std::optional<int32_t> source; // int32, field number = 8
      case 64: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.source = varint_tmp;
        break;
      }
      // std::optional<int32_t> value; // int32, field number = 9
      case 72: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.value = varint_tmp;
        break;
      }
      // std::optional<int32_t> file; // int32, field number = 10
      case 80: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.file = varint_tmp;
        break;
      }
      // std::optional<int32_t> from; // int32, field number = 11
      case 88: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.from = varint_tmp;
        break;
      }
      // std::optional<int32_t> handle_uninterpreted; // int32, field number = 12
      case 96: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.handle_uninterpreted = varint_tmp;
        break;
      }
      // std::vector<int32_t> index; // int32, field number = 13
      case 104: {
        int32_t e{};

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        e = varint_tmp;
        t.index.push_back(e);
        break;
      }
      case 106: {
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        while (pos < cur_max_size) {
          int32_t e{};

          uint64_t varint_tmp = 0;
          ok = deserialize_varint(data, pos, size, varint_tmp);
          if (!ok) {
            return false;
          }

          e = varint_tmp;
          t.index.push_back(e);
        }
        break;
      }
      // std::optional<int32_t> controller; // int32, field number = 14
      case 112: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.controller = varint_tmp;
        break;
      }
      // std::optional<int32_t> already_here; // int32, field number = 15
      case 120: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.already_here = varint_tmp;
        break;
      }
      // std::optional<uint32_t> uint32; // uint32, field number = 16
      case 128: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.uint32 = varint_tmp;
        break;
      }
      // std::optional<uint32_t> uint32_t_; // uint32, field number = 41
      case 328: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.uint32_t_ = varint_tmp;
        break;
      }
      // std::optional<uint64_t> uint64; // uint64, field number = 17
      case 136: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.uint64 = varint_tmp;
        break;
      }
      // std::optional<uint32_t> uint64_t_; // uint32, field number = 42
      case 336: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.uint64_t_ = varint_tmp;
        break;
      }
      // std::optional<std::string> string; // string, field number = 18
      case 146: {
        if (!t.string.has_value()) {
          t.string = std::string();
        }
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.string.value().resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.string.value().data(), data+pos, sz);
        pos += sz;
        break;
      }
      // std::optional<int32_t> memset_; // int32, field number = 19
      case 152: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.memset_ = varint_tmp;
        break;
      }
      // std::optional<int32_t> int32; // int32, field number = 20
      case 160: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.int32 = varint_tmp;
        break;
      }
      // std::optional<int32_t> int32_t_; // int32, field number = 43
      case 344: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.int32_t_ = varint_tmp;
        break;
      }
      // std::optional<int64_t> int64; // int64, field number = 21
      case 168: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.int64 = varint_tmp;
        break;
      }
      // std::optional<int64_t> int64_t_; // int64, field number = 44
      case 352: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.int64_t_ = varint_tmp;
        break;
      }
      // std::optional<int64_t> size_t_; // int64, field number = 45
      case 360: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.size_t_ = varint_tmp;
        break;
      }
      // std::optional<uint32_t> cached_size; // uint32, field number = 22
      case 176: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.cached_size = varint_tmp;
        break;
      }
      // std::optional<uint32_t> extensions; // uint32, field number = 23
      case 184: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.extensions = varint_tmp;
        break;
      }
      // std::optional<uint32_t> bit; // uint32, field number = 24
      case 192: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.bit = varint_tmp;
        break;
      }
      // std::optional<uint32_t> bits; // uint32, field number = 25
      case 200: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.bits = varint_tmp;
        break;
      }
      // std::optional<uint32_t> offsets; // uint32, field number = 26
      case 208: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.offsets = varint_tmp;
        break;
      }
      // std::optional<uint32_t> reflection; // uint32, field number = 27
      case 216: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.reflection = varint_tmp;
        break;
      }
      // std::optional<std::string> some_cord; // string, field number = 28
      case 226: {
        if (!t.some_cord.has_value()) {
          t.some_cord = std::string();
        }
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.some_cord.value().resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.some_cord.value().data(), data+pos, sz);
        pos += sz;
        break;
      }
      // std::optional<std::string> some_string_piece; // string, field number = 29
      case 234: {
        if (!t.some_string_piece.has_value()) {
          t.some_string_piece = std::string();
        }
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.some_string_piece.value().resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.some_string_piece.value().data(), data+pos, sz);
        pos += sz;
        break;
      }
      // std::optional<uint32_t> int_; // uint32, field number = 30
      case 240: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.int_ = varint_tmp;
        break;
      }
      // std::optional<uint32_t> friend_; // uint32, field number = 31
      case 248: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.friend_ = varint_tmp;
        break;
      }
      // std::optional<uint32_t> class_; // uint32, field number = 37
      case 296: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.class_ = varint_tmp;
        break;
      }
      // std::optional<uint32_t> typedecl; // uint32, field number = 39
      case 312: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.typedecl = varint_tmp;
        break;
      }
      // std::optional<uint32_t> auto_; // uint32, field number = 40
      case 320: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.auto_ = varint_tmp;
        break;
      }
      // std::unique_ptr<::protobuf_unittest::TestConflictingSymbolNames::DO> do_; // message, field number = 32
      case 258: {
        if (!t.do_) {
          t.do_ = std::make_unique<::protobuf_unittest::TestConflictingSymbolNames::DO>();
        }
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        ok = deserialize_to(*t.do_, data + pos, sz);
        if (!ok) {
          return false;
        }
        pos += sz;
        break;
      }
      // std::optional<int32_t> field_type; // int32, field number = 33
      case 264: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.field_type = varint_tmp;
        break;
      }
      // std::optional<bool> is_packed; // bool, field number = 34
      case 272: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.is_packed = static_cast<bool>(varint_tmp);
        break;
      }
      // std::optional<std::string> release_length; // string, field number = 35
      case 282: {
        if (!t.release_length.has_value()) {
          t.release_length = std::string();
        }
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.release_length.value().resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.release_length.value().data(), data+pos, sz);
        pos += sz;
        break;
      }
      // std::unique_ptr<::protobuf_unittest::TestConflictingSymbolNames::DO> release_do; // message, field number = 36
      case 290: {
        if (!t.release_do) {
          t.release_do = std::make_unique<::protobuf_unittest::TestConflictingSymbolNames::DO>();
        }
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        ok = deserialize_to(*t.release_do, data + pos, sz);
        if (!ok) {
          return false;
        }
        pos += sz;
        break;
      }
      // std::optional<std::string> target; // string, field number = 38
      case 306: {
        if (!t.target.has_value()) {
          t.target = std::string();
        }
        uint64_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.target.value().resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.target.value().data(), data+pos, sz);
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNames
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNames& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingSymbolNamesExtension
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNamesExtension& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingSymbolNamesExtension& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNamesExtension& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingSymbolNamesExtension& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNamesExtension& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNamesExtension&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingSymbolNamesExtension
bool deserialize_to(::protobuf_unittest::TestConflictingSymbolNamesExtension& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TestConflictingEnumNames
std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingEnumNames& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.conflicting_enum.has_value()) {
    total += 1 + calculate_varint_size(static_cast<uint64_t>(t.conflicting_enum.value()));
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestConflictingEnumNames& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingEnumNames& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::optional<::protobuf_unittest::TestConflictingEnumNames::while_> conflicting_enum; // enum, field number = 1

    if (t.conflicting_enum.has_value()) {

    serialize_varint(data, pos, size, 8);
    serialize_varint(data, pos, size, static_cast<uint64_t>(t.conflicting_enum.value()));

    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestConflictingEnumNames& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestConflictingEnumNames& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::optional<::protobuf_unittest::TestConflictingEnumNames::while_> conflicting_enum; // enum, field number = 1

      case 8: {

      uint64_t enum_val_tmp{};
      ok = deserialize_varint(data, pos, size, enum_val_tmp);
      if (!ok) {
        return false;
      }
      t.conflicting_enum = static_cast<::protobuf_unittest::TestConflictingEnumNames::while_>(enum_val_tmp);

        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestConflictingEnumNames&, const char*, std::size_t)
// end of ::protobuf_unittest::TestConflictingEnumNames
bool deserialize_to(::protobuf_unittest::TestConflictingEnumNames& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::DummyMessage
std::size_t get_needed_size(const ::protobuf_unittest::DummyMessage& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::DummyMessage& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::DummyMessage& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::DummyMessage& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::DummyMessage& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::DummyMessage&, const char*, std::size_t)
// end of ::protobuf_unittest::DummyMessage
bool deserialize_to(::protobuf_unittest::DummyMessage& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::NULL_
std::size_t get_needed_size(const ::protobuf_unittest::NULL_& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.int_.has_value()) {
    total += 1 + calculate_varint_size(t.int_.value());
  }
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::NULL_& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::NULL_& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::optional<int32_t> int_; // int32, field number = 1
    if (t.int_.has_value()) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.int_.value()));
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::NULL_& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::NULL_& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::optional<int32_t> int_; // int32, field number = 1
      case 8: {

        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }

        t.int_ = varint_tmp;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::NULL_&, const char*, std::size_t)
// end of ::protobuf_unittest::NULL_
bool deserialize_to(::protobuf_unittest::NULL_& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::Shutdown
std::size_t get_needed_size(const ::protobuf_unittest::Shutdown& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::Shutdown& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::Shutdown& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::Shutdown& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::Shutdown& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::Shutdown&, const char*, std::size_t)
// end of ::protobuf_unittest::Shutdown
bool deserialize_to(::protobuf_unittest::Shutdown& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::protobuf_unittest::TableStruct
std::size_t get_needed_size(const ::protobuf_unittest::TableStruct& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TableStruct& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TableStruct& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TableStruct& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TableStruct& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TableStruct&, const char*, std::size_t)
// end of ::protobuf_unittest::TableStruct
bool deserialize_to(::protobuf_unittest::TableStruct& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

} // internal
} // struct_pb
