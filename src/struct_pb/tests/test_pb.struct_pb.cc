// protoc generate parameter
// generate_eq_op=true,namespace=test_struct_pb
// =========================
#include "test_pb.struct_pb.h"
#include "struct_pb/struct_pb/struct_pb_impl.hpp"
namespace struct_pb {
namespace internal {
// ::test_struct_pb::Test1
std::size_t get_needed_size(const ::test_struct_pb::Test1& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + calculate_varint_size(t.a);
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::Test1& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::Test1& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int32_t a; // int32, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.a));}
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::Test1& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::Test1& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int32_t a; // int32, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.a = varint_tmp;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::Test1&, const char*, std::size_t)
// end of ::test_struct_pb::Test1
bool deserialize_to(::test_struct_pb::Test1& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::Test2
std::size_t get_needed_size(const ::test_struct_pb::Test2& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.b.empty()) {
    total += 1 + calculate_varint_size(t.b.size()) + t.b.size();
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::Test2& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::Test2& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::string b; // string, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      serialize_varint(data, pos, size, t.b.size());
      std::memcpy(data + pos, t.b.data(), t.b.size());
      pos += t.b.size();
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::Test2& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::Test2& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::string b; // string, field number = 2
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.b.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.b.data(), data+pos, sz);
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::Test2&, const char*, std::size_t)
// end of ::test_struct_pb::Test2
bool deserialize_to(::test_struct_pb::Test2& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::Test3
std::size_t get_needed_size(const ::test_struct_pb::Test3& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.c) {
    auto sz = get_needed_size(*t.c);
    total += 1 + calculate_varint_size(sz) + sz;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::Test3& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::Test3& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::unique_ptr<::test_struct_pb::Test1> c; // message, field number = 3
    if (t.c) {
      serialize_varint(data, pos, size, 26);
      auto sz = get_needed_size(*t.c);
      serialize_varint(data, pos, size, sz);
      serialize_to(data + pos, sz, *t.c);
      pos += sz;
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::Test3& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::Test3& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::unique_ptr<::test_struct_pb::Test1> c; // message, field number = 3
      case 26: {
        if (!t.c) {
          t.c = std::make_unique<::test_struct_pb::Test1>();
        }
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        ok = deserialize_to(*t.c, data + pos, sz);
        if (!ok) {
          return false;
        }
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::Test3&, const char*, std::size_t)
// end of ::test_struct_pb::Test3
bool deserialize_to(::test_struct_pb::Test3& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::Test4
std::size_t get_needed_size(const ::test_struct_pb::Test4& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.d.empty()) {
    total += 1 + calculate_varint_size(t.d.size()) + t.d.size();
  }
  if (!t.e.empty()) {
    std::size_t container_total = 0;
    for(const auto& e: t.e) {
      container_total += calculate_varint_size(e);
    }

    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::Test4& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::Test4& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::string d; // string, field number = 4
    if (!t.d.empty()) {
      serialize_varint(data, pos, size, 34);
      serialize_varint(data, pos, size, t.d.size());
      std::memcpy(data + pos, t.d.data(), t.d.size());
      pos += t.d.size();
    }
  }
  {
    // std::vector<int32_t> e; // int32, field number = 5
    if (!t.e.empty()) {
      serialize_varint(data, pos, size, 42);
      std::size_t container_total = 0;
      for(const auto& e: t.e) {
        container_total += calculate_varint_size(e);
      }

      serialize_varint(data, pos, size, container_total);
      for(const auto& v: t.e) {
        serialize_varint(data, pos, size, static_cast<uint64_t>(v));}
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::Test4& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::Test4& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::string d; // string, field number = 4
      case 34: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.d.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.d.data(), data+pos, sz);
        pos += sz;
        break;
      }
      // std::vector<int32_t> e; // int32, field number = 5
      case 40: {
        int32_t e{};
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        e = varint_tmp;
        t.e.push_back(e);
        break;
      }
      case 42: {
        std::size_t sz = 0;
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
          t.e.push_back(e);
        }

        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::Test4&, const char*, std::size_t)
// end of ::test_struct_pb::Test4
bool deserialize_to(::test_struct_pb::Test4& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestDouble
std::size_t get_needed_size(const ::test_struct_pb::MyTestDouble& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + 8;
  }
  if (t.b != 0) {
    total += 1 + 8;
  }
  if (t.c != 0) {
    total += 1 + 8;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestDouble& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestDouble& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // double a; // double, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 9);
      std::memcpy(data + pos, &t.a, 8);
      pos += 8;
    }
  }
  {
    // double b; // double, field number = 2
    if (t.b != 0) {
      serialize_varint(data, pos, size, 17);
      std::memcpy(data + pos, &t.b, 8);
      pos += 8;
    }
  }
  {
    // double c; // double, field number = 3
    if (t.c != 0) {
      serialize_varint(data, pos, size, 25);
      std::memcpy(data + pos, &t.c, 8);
      pos += 8;
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestDouble& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestDouble& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // double a; // double, field number = 1
      case 9: {
        if (pos + 8 > size) {
          return false;
        }
        double fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.a = fixed_tmp;
        break;
      }
      // double b; // double, field number = 2
      case 17: {
        if (pos + 8 > size) {
          return false;
        }
        double fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.b = fixed_tmp;
        break;
      }
      // double c; // double, field number = 3
      case 25: {
        if (pos + 8 > size) {
          return false;
        }
        double fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.c = fixed_tmp;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestDouble&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestDouble
bool deserialize_to(::test_struct_pb::MyTestDouble& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestFloat
std::size_t get_needed_size(const ::test_struct_pb::MyTestFloat& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + 4;
  }
  if (t.b != 0) {
    total += 1 + 4;
  }
  if (t.c != 0) {
    total += 1 + 4;
  }
  if (t.d != 0) {
    total += 1 + 4;
  }
  if (t.e != 0) {
    total += 1 + 4;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestFloat& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestFloat& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // float a; // float, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 13);
      std::memcpy(data + pos, &t.a, 4);
      pos += 4;
    }
  }
  {
    // float b; // float, field number = 2
    if (t.b != 0) {
      serialize_varint(data, pos, size, 21);
      std::memcpy(data + pos, &t.b, 4);
      pos += 4;
    }
  }
  {
    // float c; // float, field number = 3
    if (t.c != 0) {
      serialize_varint(data, pos, size, 29);
      std::memcpy(data + pos, &t.c, 4);
      pos += 4;
    }
  }
  {
    // float d; // float, field number = 4
    if (t.d != 0) {
      serialize_varint(data, pos, size, 37);
      std::memcpy(data + pos, &t.d, 4);
      pos += 4;
    }
  }
  {
    // float e; // float, field number = 5
    if (t.e != 0) {
      serialize_varint(data, pos, size, 45);
      std::memcpy(data + pos, &t.e, 4);
      pos += 4;
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestFloat& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestFloat& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // float a; // float, field number = 1
      case 13: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.a = fixed_tmp;
        break;
      }
      // float b; // float, field number = 2
      case 21: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.b = fixed_tmp;
        break;
      }
      // float c; // float, field number = 3
      case 29: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.c = fixed_tmp;
        break;
      }
      // float d; // float, field number = 4
      case 37: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.d = fixed_tmp;
        break;
      }
      // float e; // float, field number = 5
      case 45: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.e = fixed_tmp;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestFloat&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestFloat
bool deserialize_to(::test_struct_pb::MyTestFloat& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestInt32
std::size_t get_needed_size(const ::test_struct_pb::MyTestInt32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + calculate_varint_size(t.a);
  }
  if (!t.b.empty()) {
    std::size_t container_total = 0;
    for(const auto& e: t.b) {
      container_total += calculate_varint_size(e);
    }

    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestInt32& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestInt32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int32_t a; // int32, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.a));}
  }
  {
    // std::vector<int32_t> b; // int32, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      std::size_t container_total = 0;
      for(const auto& e: t.b) {
        container_total += calculate_varint_size(e);
      }

      serialize_varint(data, pos, size, container_total);
      for(const auto& v: t.b) {
        serialize_varint(data, pos, size, static_cast<uint64_t>(v));}
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestInt32& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestInt32& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int32_t a; // int32, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.a = varint_tmp;
        break;
      }
      // std::vector<int32_t> b; // int32, field number = 2
      case 16: {
        int32_t e{};
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        e = varint_tmp;
        t.b.push_back(e);
        break;
      }
      case 18: {
        std::size_t sz = 0;
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
          t.b.push_back(e);
        }

        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestInt32&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestInt32
bool deserialize_to(::test_struct_pb::MyTestInt32& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestInt64
std::size_t get_needed_size(const ::test_struct_pb::MyTestInt64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + calculate_varint_size(t.a);
  }
  if (!t.b.empty()) {
    std::size_t container_total = 0;
    for(const auto& e: t.b) {
      container_total += calculate_varint_size(e);
    }

    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestInt64& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestInt64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int64_t a; // int64, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.a));}
  }
  {
    // std::vector<int64_t> b; // int64, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      std::size_t container_total = 0;
      for(const auto& e: t.b) {
        container_total += calculate_varint_size(e);
      }

      serialize_varint(data, pos, size, container_total);
      for(const auto& v: t.b) {
        serialize_varint(data, pos, size, static_cast<uint64_t>(v));}
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestInt64& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestInt64& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int64_t a; // int64, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.a = varint_tmp;
        break;
      }
      // std::vector<int64_t> b; // int64, field number = 2
      case 16: {
        int64_t e{};
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        e = varint_tmp;
        t.b.push_back(e);
        break;
      }
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        while (pos < cur_max_size) {
          int64_t e{};
          uint64_t varint_tmp = 0;
          ok = deserialize_varint(data, pos, size, varint_tmp);
          if (!ok) {
            return false;
          }
          e = varint_tmp;
          t.b.push_back(e);
        }

        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestInt64&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestInt64
bool deserialize_to(::test_struct_pb::MyTestInt64& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestUint32
std::size_t get_needed_size(const ::test_struct_pb::MyTestUint32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + calculate_varint_size(t.a);
  }
  if (!t.b.empty()) {
    std::size_t container_total = 0;
    for(const auto& e: t.b) {
      container_total += calculate_varint_size(e);
    }

    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestUint32& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestUint32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // uint32_t a; // uint32, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.a));}
  }
  {
    // std::vector<uint32_t> b; // uint32, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      std::size_t container_total = 0;
      for(const auto& e: t.b) {
        container_total += calculate_varint_size(e);
      }

      serialize_varint(data, pos, size, container_total);
      for(const auto& v: t.b) {
        serialize_varint(data, pos, size, static_cast<uint64_t>(v));}
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestUint32& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestUint32& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // uint32_t a; // uint32, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.a = varint_tmp;
        break;
      }
      // std::vector<uint32_t> b; // uint32, field number = 2
      case 16: {
        uint32_t e{};
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        e = varint_tmp;
        t.b.push_back(e);
        break;
      }
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        while (pos < cur_max_size) {
          uint32_t e{};
          uint64_t varint_tmp = 0;
          ok = deserialize_varint(data, pos, size, varint_tmp);
          if (!ok) {
            return false;
          }
          e = varint_tmp;
          t.b.push_back(e);
        }

        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestUint32&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestUint32
bool deserialize_to(::test_struct_pb::MyTestUint32& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestUint64
std::size_t get_needed_size(const ::test_struct_pb::MyTestUint64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + calculate_varint_size(t.a);
  }
  if (!t.b.empty()) {
    std::size_t container_total = 0;
    for(const auto& e: t.b) {
      container_total += calculate_varint_size(e);
    }

    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestUint64& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestUint64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // uint64_t a; // uint64, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, t.a);}
  }
  {
    // std::vector<uint64_t> b; // uint64, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      std::size_t container_total = 0;
      for(const auto& e: t.b) {
        container_total += calculate_varint_size(e);
      }

      serialize_varint(data, pos, size, container_total);
      for(const auto& v: t.b) {
        serialize_varint(data, pos, size, v);}
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestUint64& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestUint64& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // uint64_t a; // uint64, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.a = varint_tmp;
        break;
      }
      // std::vector<uint64_t> b; // uint64, field number = 2
      case 16: {
        uint64_t e{};
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        e = varint_tmp;
        t.b.push_back(e);
        break;
      }
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        while (pos < cur_max_size) {
          uint64_t e{};
          uint64_t varint_tmp = 0;
          ok = deserialize_varint(data, pos, size, varint_tmp);
          if (!ok) {
            return false;
          }
          e = varint_tmp;
          t.b.push_back(e);
        }

        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestUint64&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestUint64
bool deserialize_to(::test_struct_pb::MyTestUint64& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestEnum
std::size_t get_needed_size(const ::test_struct_pb::MyTestEnum& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();

  if (t.color != ::test_struct_pb::MyTestEnum::Color::Red) {
   total += 1 + calculate_varint_size(static_cast<uint64_t>(t.color));
  }

  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestEnum& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestEnum& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // ::test_struct_pb::MyTestEnum::Color color; // enum, field number = 6
    if (t.color != ::test_struct_pb::MyTestEnum::Color::Red) {
      serialize_varint(data, pos, size, 48);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.color));
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestEnum& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestEnum& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // ::test_struct_pb::MyTestEnum::Color color; // enum, field number = 6
      case 48: {
        uint64_t enum_val_tmp{};
        ok = deserialize_varint(data, pos, size, enum_val_tmp);
        if (!ok) {
          return false;
        }
        t.color = static_cast<::test_struct_pb::MyTestEnum::Color>(enum_val_tmp);
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestEnum&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestEnum
bool deserialize_to(::test_struct_pb::MyTestEnum& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestRepeatedMessage
std::size_t get_needed_size(const ::test_struct_pb::MyTestRepeatedMessage& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.fs.empty()) {
    for(const auto& e: t.fs) {
      std::size_t sz = get_needed_size(e);
      total += 1 + calculate_varint_size(sz) + sz;
    }
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestRepeatedMessage& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestRepeatedMessage& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<::test_struct_pb::MyTestFloat> fs; // message, field number = 1
    if (!t.fs.empty()) {
      for(const auto& e: t.fs) {
        serialize_varint(data, pos, size, 10);
        std::size_t sz = get_needed_size(e);
        serialize_varint(data, pos, size, sz);
        serialize_to(data+pos, sz, e);
        pos += sz;
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestRepeatedMessage& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestRepeatedMessage& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<::test_struct_pb::MyTestFloat> fs; // message, field number = 1
      case 10: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.fs.emplace_back();
        ok = deserialize_to(t.fs.back(), data + pos, sz);
        if (!ok) {
          t.fs.pop_back();
          return false;
        }
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestRepeatedMessage&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestRepeatedMessage
bool deserialize_to(::test_struct_pb::MyTestRepeatedMessage& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestSint32
std::size_t get_needed_size(const ::test_struct_pb::MyTestSint32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + calculate_varint_size(encode_zigzag(t.a));
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestSint32& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestSint32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int32_t a; // sint32, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, encode_zigzag(t.a));}
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestSint32& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestSint32& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int32_t a; // sint32, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.a = static_cast<int32_t>(decode_zigzag(uint32_t(varint_tmp)));
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestSint32&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestSint32
bool deserialize_to(::test_struct_pb::MyTestSint32& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestSint64
std::size_t get_needed_size(const ::test_struct_pb::MyTestSint64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.b != 0) {
    total += 1 + calculate_varint_size(encode_zigzag(t.b));
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestSint64& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestSint64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int64_t b; // sint64, field number = 2
    if (t.b != 0) {
      serialize_varint(data, pos, size, 16);
      serialize_varint(data, pos, size, encode_zigzag(t.b));}
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestSint64& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestSint64& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int64_t b; // sint64, field number = 2
      case 16: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.b = static_cast<int64_t>(decode_zigzag(varint_tmp));
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestSint64&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestSint64
bool deserialize_to(::test_struct_pb::MyTestSint64& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestMap
std::size_t get_needed_size(const ::test_struct_pb::MyTestMap& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  for(const auto& e: t.e) {
    std::size_t map_entry_sz = 0;
    map_entry_sz += 1 + calculate_varint_size(e.first.size()) + e.first.size();
    map_entry_sz += 1 + calculate_varint_size(e.second);
    total += 1 + calculate_varint_size(map_entry_sz) + map_entry_sz;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestMap& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestMap& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::map<std::string, int32_t> e; // message, field number = 3
    for(const auto& e: t.e) {
      serialize_varint(data, pos, size, 26);
      std::size_t map_entry_sz = 0;
      {
        map_entry_sz += 1 + calculate_varint_size(e.first.size()) + e.first.size();
      }
      {
        map_entry_sz += 1 + calculate_varint_size(e.second);
      }
      serialize_varint(data, pos, size, map_entry_sz);
      {
        serialize_varint(data, pos, size, 10);
        serialize_varint(data, pos, size, e.first.size());
        std::memcpy(data + pos, e.first.data(), e.first.size());
        pos += e.first.size();
      }
      {
        serialize_varint(data, pos, size, 16);
        serialize_varint(data, pos, size, static_cast<uint64_t>(e.second));}
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestMap& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestMap& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::map<std::string, int32_t> e; // message, field number = 3
      case 26: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        std::string key_tmp_val {};
        int32_t value_tmp_val {};
        while (pos < cur_max_size) {
          ok = read_tag(data, pos, size, tag);
          if (!ok) {
            return false;
          }
          switch(tag) {
            case 10: {
              std::size_t str_sz = 0;
              ok = deserialize_varint(data, pos, cur_max_size, str_sz);
              if (!ok) {
                return false;
              }
              key_tmp_val.resize(str_sz);
              if (pos + str_sz > cur_max_size) {
                return false;
              }
              std::memcpy(key_tmp_val.data(), data+pos, str_sz);
              pos += str_sz;
              break;
            }
            case 16: {
              uint64_t varint_tmp = 0;
              ok = deserialize_varint(data, pos, size, varint_tmp);
              if (!ok) {
                return false;
              }
              value_tmp_val = varint_tmp;
              break;
            }

            default: {
              return false;
            }
          }
        }
        t.e[key_tmp_val] = std::move(value_tmp_val);
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestMap&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestMap
bool deserialize_to(::test_struct_pb::MyTestMap& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestFixed32
std::size_t get_needed_size(const ::test_struct_pb::MyTestFixed32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + 4;
  }
  if (!t.b.empty()) {
    std::size_t container_total = 4 * t.b.size();
    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestFixed32& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestFixed32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // uint32_t a; // fixed32, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 13);
      std::memcpy(data + pos, &t.a, 4);
      pos += 4;
    }
  }
  {
    // std::vector<uint32_t> b; // fixed32, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      std::size_t container_total = 4 * t.b.size();
      serialize_varint(data, pos, size, container_total);
      std::memcpy(data + pos, t.b.data(), container_total);
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestFixed32& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestFixed32& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // uint32_t a; // fixed32, field number = 1
      case 13: {
        if (pos + 4 > size) {
          return false;
        }
        uint32_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.a = fixed_tmp;
        break;
      }
      // std::vector<uint32_t> b; // fixed32, field number = 2
      case 21: {
        uint32_t e{};
        if (pos + 4 > size) {
          return false;
        }
        uint32_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        e = fixed_tmp;
        t.b.push_back(e);
        break;
      }
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        int count = sz / 4;
        if (4 * count != sz) {
          return false;
        }
        if (pos + sz > size) {
          return false;
        }
        t.b.resize(count);
        std::memcpy(t.b.data(), data + pos, sz);
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestFixed32&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestFixed32
bool deserialize_to(::test_struct_pb::MyTestFixed32& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestFixed64
std::size_t get_needed_size(const ::test_struct_pb::MyTestFixed64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + 8;
  }
  if (!t.b.empty()) {
    std::size_t container_total = 8 * t.b.size();
    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestFixed64& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestFixed64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // uint64_t a; // fixed64, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 9);
      std::memcpy(data + pos, &t.a, 8);
      pos += 8;
    }
  }
  {
    // std::vector<uint64_t> b; // fixed64, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      std::size_t container_total = 8 * t.b.size();
      serialize_varint(data, pos, size, container_total);
      std::memcpy(data + pos, t.b.data(), container_total);
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestFixed64& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestFixed64& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // uint64_t a; // fixed64, field number = 1
      case 9: {
        if (pos + 8 > size) {
          return false;
        }
        uint64_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.a = fixed_tmp;
        break;
      }
      // std::vector<uint64_t> b; // fixed64, field number = 2
      case 17: {
        uint64_t e{};
        if (pos + 8 > size) {
          return false;
        }
        uint64_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        e = fixed_tmp;
        t.b.push_back(e);
        break;
      }
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        int count = sz / 8;
        if (8 * count != sz) {
          return false;
        }
        if (pos + sz > size) {
          return false;
        }
        t.b.resize(count);
        std::memcpy(t.b.data(), data + pos, sz);
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestFixed64&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestFixed64
bool deserialize_to(::test_struct_pb::MyTestFixed64& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestSfixed32
std::size_t get_needed_size(const ::test_struct_pb::MyTestSfixed32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + 4;
  }
  if (!t.b.empty()) {
    std::size_t container_total = 4 * t.b.size();
    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestSfixed32& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestSfixed32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int32_t a; // sfixed32, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 13);
      std::memcpy(data + pos, &t.a, 4);
      pos += 4;
    }
  }
  {
    // std::vector<int32_t> b; // sfixed32, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      std::size_t container_total = 4 * t.b.size();
      serialize_varint(data, pos, size, container_total);
      std::memcpy(data + pos, t.b.data(), container_total);
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestSfixed32& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestSfixed32& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int32_t a; // sfixed32, field number = 1
      case 13: {
        if (pos + 4 > size) {
          return false;
        }
        int32_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.a = fixed_tmp;
        break;
      }
      // std::vector<int32_t> b; // sfixed32, field number = 2
      case 21: {
        int32_t e{};
        if (pos + 4 > size) {
          return false;
        }
        int32_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        e = fixed_tmp;
        t.b.push_back(e);
        break;
      }
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        int count = sz / 4;
        if (4 * count != sz) {
          return false;
        }
        if (pos + sz > size) {
          return false;
        }
        t.b.resize(count);
        std::memcpy(t.b.data(), data + pos, sz);
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestSfixed32&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestSfixed32
bool deserialize_to(::test_struct_pb::MyTestSfixed32& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestSfixed64
std::size_t get_needed_size(const ::test_struct_pb::MyTestSfixed64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + 8;
  }
  if (!t.b.empty()) {
    std::size_t container_total = 8 * t.b.size();
    total += 1 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestSfixed64& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestSfixed64& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int64_t a; // sfixed64, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 9);
      std::memcpy(data + pos, &t.a, 8);
      pos += 8;
    }
  }
  {
    // std::vector<int64_t> b; // sfixed64, field number = 2
    if (!t.b.empty()) {
      serialize_varint(data, pos, size, 18);
      std::size_t container_total = 8 * t.b.size();
      serialize_varint(data, pos, size, container_total);
      std::memcpy(data + pos, t.b.data(), container_total);
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestSfixed64& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestSfixed64& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int64_t a; // sfixed64, field number = 1
      case 9: {
        if (pos + 8 > size) {
          return false;
        }
        int64_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.a = fixed_tmp;
        break;
      }
      // std::vector<int64_t> b; // sfixed64, field number = 2
      case 17: {
        int64_t e{};
        if (pos + 8 > size) {
          return false;
        }
        int64_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        e = fixed_tmp;
        t.b.push_back(e);
        break;
      }
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        int count = sz / 8;
        if (8 * count != sz) {
          return false;
        }
        if (pos + sz > size) {
          return false;
        }
        t.b.resize(count);
        std::memcpy(t.b.data(), data + pos, sz);
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestSfixed64&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestSfixed64
bool deserialize_to(::test_struct_pb::MyTestSfixed64& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestFieldNumberRandom
std::size_t get_needed_size(const ::test_struct_pb::MyTestFieldNumberRandom& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + calculate_varint_size(t.a);
  }
  if (t.b != 0) {
    total += 1 + calculate_varint_size(encode_zigzag(t.b));
  }
  if (!t.c.empty()) {
    total += 1 + calculate_varint_size(t.c.size()) + t.c.size();
  }
  if (t.d != 0) {
    total += 1 + 8;
  }
  if (t.e != 0) {
    total += 1 + 4;
  }
  if (!t.f.empty()) {
    std::size_t container_total = 4 * t.f.size();
    total += 2 + calculate_varint_size(container_total) + container_total;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestFieldNumberRandom& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestFieldNumberRandom& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // float e; // float, field number = 1
    if (t.e != 0) {
      serialize_varint(data, pos, size, 13);
      std::memcpy(data + pos, &t.e, 4);
      pos += 4;
    }
  }
  {
    // int64_t b; // sint64, field number = 3
    if (t.b != 0) {
      serialize_varint(data, pos, size, 24);
      serialize_varint(data, pos, size, encode_zigzag(t.b));}
  }
  {
    // std::string c; // string, field number = 4
    if (!t.c.empty()) {
      serialize_varint(data, pos, size, 34);
      serialize_varint(data, pos, size, t.c.size());
      std::memcpy(data + pos, t.c.data(), t.c.size());
      pos += t.c.size();
    }
  }
  {
    // double d; // double, field number = 5
    if (t.d != 0) {
      serialize_varint(data, pos, size, 41);
      std::memcpy(data + pos, &t.d, 8);
      pos += 8;
    }
  }
  {
    // int32_t a; // int32, field number = 6
    if (t.a != 0) {
      serialize_varint(data, pos, size, 48);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.a));}
  }
  {
    // std::vector<uint32_t> f; // fixed32, field number = 128
    if (!t.f.empty()) {
      serialize_varint(data, pos, size, 1026);
      std::size_t container_total = 4 * t.f.size();
      serialize_varint(data, pos, size, container_total);
      std::memcpy(data + pos, t.f.data(), container_total);
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestFieldNumberRandom& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestFieldNumberRandom& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int32_t a; // int32, field number = 6
      case 48: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.a = varint_tmp;
        break;
      }
      // int64_t b; // sint64, field number = 3
      case 24: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.b = static_cast<int64_t>(decode_zigzag(varint_tmp));
        break;
      }
      // std::string c; // string, field number = 4
      case 34: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.c.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.c.data(), data+pos, sz);
        pos += sz;
        break;
      }
      // double d; // double, field number = 5
      case 41: {
        if (pos + 8 > size) {
          return false;
        }
        double fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.d = fixed_tmp;
        break;
      }
      // float e; // float, field number = 1
      case 13: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.e = fixed_tmp;
        break;
      }
      // std::vector<uint32_t> f; // fixed32, field number = 128
      case 1029: {
        uint32_t e{};
        if (pos + 4 > size) {
          return false;
        }
        uint32_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        e = fixed_tmp;
        t.f.push_back(e);
        break;
      }
      case 1026: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        std::size_t cur_max_size = pos + sz;
        int count = sz / 4;
        if (4 * count != sz) {
          return false;
        }
        if (pos + sz > size) {
          return false;
        }
        t.f.resize(count);
        std::memcpy(t.f.data(), data + pos, sz);
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestFieldNumberRandom&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestFieldNumberRandom
bool deserialize_to(::test_struct_pb::MyTestFieldNumberRandom& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::MyTestAll
std::size_t get_needed_size(const ::test_struct_pb::MyTestAll& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.a != 0) {
    total += 1 + 8;
  }
  if (t.b != 0) {
    total += 1 + 4;
  }
  if (t.c != 0) {
    total += 1 + calculate_varint_size(t.c);
  }
  if (t.d != 0) {
    total += 1 + calculate_varint_size(t.d);
  }
  if (t.e != 0) {
    total += 1 + calculate_varint_size(t.e);
  }
  if (t.f != 0) {
    total += 1 + calculate_varint_size(t.f);
  }
  if (t.g != 0) {
    total += 1 + calculate_varint_size(encode_zigzag(t.g));
  }
  if (t.h != 0) {
    total += 1 + calculate_varint_size(encode_zigzag(t.h));
  }
  if (t.i != 0) {
    total += 1 + 4;
  }
  if (t.j != 0) {
    total += 1 + 8;
  }
  if (t.k != 0) {
    total += 1 + 4;
  }
  if (t.l != 0) {
    total += 1 + 8;
  }
  if (t.m != 0) {
    total += 1 + calculate_varint_size(static_cast<uint64_t>(t.m));
  }
  if (!t.n.empty()) {
    total += 1 + calculate_varint_size(t.n.size()) + t.n.size();
  }
  if (!t.o.empty()) {
    total += 1 + calculate_varint_size(t.o.size()) + t.o.size();
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::MyTestAll& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestAll& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // double a; // double, field number = 1
    if (t.a != 0) {
      serialize_varint(data, pos, size, 9);
      std::memcpy(data + pos, &t.a, 8);
      pos += 8;
    }
  }
  {
    // float b; // float, field number = 2
    if (t.b != 0) {
      serialize_varint(data, pos, size, 21);
      std::memcpy(data + pos, &t.b, 4);
      pos += 4;
    }
  }
  {
    // int32_t c; // int32, field number = 3
    if (t.c != 0) {
      serialize_varint(data, pos, size, 24);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.c));}
  }
  {
    // int64_t d; // int64, field number = 4
    if (t.d != 0) {
      serialize_varint(data, pos, size, 32);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.d));}
  }
  {
    // uint32_t e; // uint32, field number = 5
    if (t.e != 0) {
      serialize_varint(data, pos, size, 40);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.e));}
  }
  {
    // uint64_t f; // uint64, field number = 6
    if (t.f != 0) {
      serialize_varint(data, pos, size, 48);
      serialize_varint(data, pos, size, t.f);}
  }
  {
    // int32_t g; // sint32, field number = 7
    if (t.g != 0) {
      serialize_varint(data, pos, size, 56);
      serialize_varint(data, pos, size, encode_zigzag(t.g));}
  }
  {
    // int64_t h; // sint64, field number = 8
    if (t.h != 0) {
      serialize_varint(data, pos, size, 64);
      serialize_varint(data, pos, size, encode_zigzag(t.h));}
  }
  {
    // uint32_t i; // fixed32, field number = 9
    if (t.i != 0) {
      serialize_varint(data, pos, size, 77);
      std::memcpy(data + pos, &t.i, 4);
      pos += 4;
    }
  }
  {
    // uint64_t j; // fixed64, field number = 10
    if (t.j != 0) {
      serialize_varint(data, pos, size, 81);
      std::memcpy(data + pos, &t.j, 8);
      pos += 8;
    }
  }
  {
    // int32_t k; // sfixed32, field number = 11
    if (t.k != 0) {
      serialize_varint(data, pos, size, 93);
      std::memcpy(data + pos, &t.k, 4);
      pos += 4;
    }
  }
  {
    // int64_t l; // sfixed64, field number = 12
    if (t.l != 0) {
      serialize_varint(data, pos, size, 97);
      std::memcpy(data + pos, &t.l, 8);
      pos += 8;
    }
  }
  {
    // bool m; // bool, field number = 13
    if (t.m != 0) {
      serialize_varint(data, pos, size, 104);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.m));}
  }
  {
    // std::string n; // string, field number = 14
    if (!t.n.empty()) {
      serialize_varint(data, pos, size, 114);
      serialize_varint(data, pos, size, t.n.size());
      std::memcpy(data + pos, t.n.data(), t.n.size());
      pos += t.n.size();
    }
  }
  {
    // std::string o; // bytes, field number = 15
    if (!t.o.empty()) {
      serialize_varint(data, pos, size, 122);
      serialize_varint(data, pos, size, t.o.size());
      std::memcpy(data + pos, t.o.data(), t.o.size());
      pos += t.o.size();
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::MyTestAll& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::MyTestAll& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // double a; // double, field number = 1
      case 9: {
        if (pos + 8 > size) {
          return false;
        }
        double fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.a = fixed_tmp;
        break;
      }
      // float b; // float, field number = 2
      case 21: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.b = fixed_tmp;
        break;
      }
      // int32_t c; // int32, field number = 3
      case 24: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.c = varint_tmp;
        break;
      }
      // int64_t d; // int64, field number = 4
      case 32: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.d = varint_tmp;
        break;
      }
      // uint32_t e; // uint32, field number = 5
      case 40: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.e = varint_tmp;
        break;
      }
      // uint64_t f; // uint64, field number = 6
      case 48: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.f = varint_tmp;
        break;
      }
      // int32_t g; // sint32, field number = 7
      case 56: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.g = static_cast<int32_t>(decode_zigzag(uint32_t(varint_tmp)));
        break;
      }
      // int64_t h; // sint64, field number = 8
      case 64: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.h = static_cast<int64_t>(decode_zigzag(varint_tmp));
        break;
      }
      // uint32_t i; // fixed32, field number = 9
      case 77: {
        if (pos + 4 > size) {
          return false;
        }
        uint32_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.i = fixed_tmp;
        break;
      }
      // uint64_t j; // fixed64, field number = 10
      case 81: {
        if (pos + 8 > size) {
          return false;
        }
        uint64_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.j = fixed_tmp;
        break;
      }
      // int32_t k; // sfixed32, field number = 11
      case 93: {
        if (pos + 4 > size) {
          return false;
        }
        int32_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.k = fixed_tmp;
        break;
      }
      // int64_t l; // sfixed64, field number = 12
      case 97: {
        if (pos + 8 > size) {
          return false;
        }
        int64_t fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.l = fixed_tmp;
        break;
      }
      // bool m; // bool, field number = 13
      case 104: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.m = static_cast<bool>(varint_tmp);
        break;
      }
      // std::string n; // string, field number = 14
      case 114: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.n.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.n.data(), data+pos, sz);
        pos += sz;
        break;
      }
      // std::string o; // bytes, field number = 15
      case 122: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.o.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.o.data(), data+pos, sz);
        pos += sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::MyTestAll&, const char*, std::size_t)
// end of ::test_struct_pb::MyTestAll
bool deserialize_to(::test_struct_pb::MyTestAll& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::SubMessageForOneof
std::size_t get_needed_size(const ::test_struct_pb::SubMessageForOneof& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.ok != 0) {
    total += 1 + calculate_varint_size(static_cast<uint64_t>(t.ok));
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::SubMessageForOneof& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::SubMessageForOneof& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // bool ok; // bool, field number = 1
    if (t.ok != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.ok));}
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::SubMessageForOneof& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::SubMessageForOneof& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // bool ok; // bool, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.ok = static_cast<bool>(varint_tmp);
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::SubMessageForOneof&, const char*, std::size_t)
// end of ::test_struct_pb::SubMessageForOneof
bool deserialize_to(::test_struct_pb::SubMessageForOneof& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::test_struct_pb::SampleMessageOneof
std::size_t get_needed_size(const ::test_struct_pb::SampleMessageOneof& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.test_oneof.index() == 1) {
    total += 1 + calculate_varint_size(std::get<1>(t.test_oneof));
  }
  if (t.test_oneof.index() == 2) {
    total += 1 + calculate_varint_size(std::get<2>(t.test_oneof));
  }
  if (t.test_oneof.index() == 3) {
    total += 1 + calculate_varint_size(std::get<3>(t.test_oneof).size()) + std::get<3>(t.test_oneof).size();
  }
  if (t.test_oneof.index() == 4) {
    std::size_t sz = 0;
    if (std::get<4>(t.test_oneof)) {
      sz = get_needed_size(*std::get<4>(t.test_oneof));
    }
    total += 1 + calculate_varint_size(sz) + sz;
  }
  return total;
} // std::size_t get_needed_size(const ::test_struct_pb::SampleMessageOneof& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::test_struct_pb::SampleMessageOneof& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    if (t.test_oneof.index() == 3) {
      serialize_varint(data, pos, size, 34);
      serialize_varint(data, pos, size, std::get<3>(t.test_oneof).size());
      std::memcpy(data + pos, std::get<3>(t.test_oneof).data(), std::get<3>(t.test_oneof).size());
      pos += std::get<3>(t.test_oneof).size();
    }
  }
  {
    if (t.test_oneof.index() == 2) {
      serialize_varint(data, pos, size, 64);
      serialize_varint(data, pos, size, static_cast<uint64_t>(std::get<2>(t.test_oneof)));}
  }
  {
    if (t.test_oneof.index() == 4) {
      serialize_varint(data, pos, size, 74);
      if (std::get<4>(t.test_oneof)) {
        std::size_t sz = get_needed_size(*std::get<4>(t.test_oneof));
        serialize_varint(data, pos, size, sz);
        serialize_to(data + pos, sz, *std::get<4>(t.test_oneof));
        pos += sz;
      }
      else {
        serialize_varint(data, pos, size, 0);
      }
    }
  }
  {
    if (t.test_oneof.index() == 1) {
      serialize_varint(data, pos, size, 80);
      serialize_varint(data, pos, size, static_cast<uint64_t>(std::get<1>(t.test_oneof)));}
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::test_struct_pb::SampleMessageOneof& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::test_struct_pb::SampleMessageOneof& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      case 80: {
        if (t.test_oneof.index() != 1) {
          t.test_oneof.emplace<1>();
        }
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        std::get<1>(t.test_oneof) = varint_tmp;
        break;
      }
      case 64: {
        if (t.test_oneof.index() != 2) {
          t.test_oneof.emplace<2>();
        }
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        std::get<2>(t.test_oneof) = varint_tmp;
        break;
      }
      case 34: {
        if (t.test_oneof.index() != 3) {
          t.test_oneof.emplace<3>();
        }
        std::size_t str_sz = 0;
        ok = deserialize_varint(data, pos, size, str_sz);
        if (!ok) {
          return false;
        }
        std::get<3>(t.test_oneof).resize(str_sz);
        if (pos + str_sz > size) {
          return false;
        }
        std::memcpy(std::get<3>(t.test_oneof).data(), data+pos, str_sz);
        pos += str_sz;
        break;
      }
      case 74: {
        if (t.test_oneof.index() != 4) {
          t.test_oneof.emplace<4>(new ::test_struct_pb::SubMessageForOneof());
        }
        std::size_t msg_sz = 0;
        ok = deserialize_varint(data, pos, size, msg_sz);
        if (!ok) {
          return false;
        }
        ok = deserialize_to(*std::get<4>(t.test_oneof), data + pos, msg_sz);
        if (!ok) {
          return false;
        }
        pos += msg_sz;
        break;
      }
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        // TODO: handle unknown field
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::test_struct_pb::SampleMessageOneof&, const char*, std::size_t)
// end of ::test_struct_pb::SampleMessageOneof
bool deserialize_to(::test_struct_pb::SampleMessageOneof& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

} // internal
} // struct_pb
