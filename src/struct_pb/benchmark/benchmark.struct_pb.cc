// protoc generate parameter
// namespace=struct_pb_sample
// =========================
#include "benchmark.struct_pb.h"
#include "struct_pb/struct_pb/struct_pb_impl.hpp"
namespace struct_pb {
namespace internal {
// ::struct_pb_sample::Vec3
std::size_t get_needed_size(const ::struct_pb_sample::Vec3& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.x != 0) {
    total += 1 + 4;
  }
  if (t.y != 0) {
    total += 1 + 4;
  }
  if (t.z != 0) {
    total += 1 + 4;
  }
  return total;
} // std::size_t get_needed_size(const ::struct_pb_sample::Vec3& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::Vec3& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // float x; // float, field number = 1
    if (t.x != 0) {
      serialize_varint(data, pos, size, 13);
      std::memcpy(data + pos, &t.x, 4);
      pos += 4;
    }
  }
  {
    // float y; // float, field number = 2
    if (t.y != 0) {
      serialize_varint(data, pos, size, 21);
      std::memcpy(data + pos, &t.y, 4);
      pos += 4;
    }
  }
  {
    // float z; // float, field number = 3
    if (t.z != 0) {
      serialize_varint(data, pos, size, 29);
      std::memcpy(data + pos, &t.z, 4);
      pos += 4;
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::Vec3& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::struct_pb_sample::Vec3& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // float x; // float, field number = 1
      case 13: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.x = fixed_tmp;
        break;
      }
      // float y; // float, field number = 2
      case 21: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.y = fixed_tmp;
        break;
      }
      // float z; // float, field number = 3
      case 29: {
        if (pos + 4 > size) {
          return false;
        }
        float fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 4);
        pos += 4;
        t.z = fixed_tmp;
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
} // bool deserialize_to(::struct_pb_sample::Vec3&, const char*, std::size_t)
// end of ::struct_pb_sample::Vec3
bool deserialize_to(::struct_pb_sample::Vec3& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::struct_pb_sample::Weapon
std::size_t get_needed_size(const ::struct_pb_sample::Weapon& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.name.empty()) {
    total += 1 + calculate_varint_size(t.name.size()) + t.name.size();
  }
  if (t.damage != 0) {
    total += 1 + calculate_varint_size(t.damage);
  }
  return total;
} // std::size_t get_needed_size(const ::struct_pb_sample::Weapon& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::Weapon& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::string name; // string, field number = 1
    if (!t.name.empty()) {
      serialize_varint(data, pos, size, 10);
      serialize_varint(data, pos, size, t.name.size());
      std::memcpy(data + pos, t.name.data(), t.name.size());
      pos += t.name.size();
    }
  }
  {
    // int32_t damage; // int32, field number = 2
    if (t.damage != 0) {
      serialize_varint(data, pos, size, 16);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.damage));}
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::Weapon& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::struct_pb_sample::Weapon& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::string name; // string, field number = 1
      case 10: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.name.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.name.data(), data+pos, sz);
        pos += sz;
        break;
      }
      // int32_t damage; // int32, field number = 2
      case 16: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.damage = varint_tmp;
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
} // bool deserialize_to(::struct_pb_sample::Weapon&, const char*, std::size_t)
// end of ::struct_pb_sample::Weapon
bool deserialize_to(::struct_pb_sample::Weapon& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::struct_pb_sample::Monster
std::size_t get_needed_size(const ::struct_pb_sample::Monster& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.pos) {
    auto sz = get_needed_size(*t.pos);
    total += 1 + calculate_varint_size(sz) + sz;
  }
  if (t.mana != 0) {
    total += 1 + calculate_varint_size(t.mana);
  }
  if (t.hp != 0) {
    total += 1 + calculate_varint_size(t.hp);
  }
  if (!t.name.empty()) {
    total += 1 + calculate_varint_size(t.name.size()) + t.name.size();
  }
  if (!t.inventory.empty()) {
    total += 1 + calculate_varint_size(t.inventory.size()) + t.inventory.size();
  }

  if (t.color != ::struct_pb_sample::Monster::Color::Red) {
   total += 1 + calculate_varint_size(static_cast<uint64_t>(t.color));
  }

  if (!t.weapons.empty()) {
    for(const auto& e: t.weapons) {
      std::size_t sz = get_needed_size(e);
      total += 1 + calculate_varint_size(sz) + sz;
    }
  }
  if (t.equipped) {
    auto sz = get_needed_size(*t.equipped);
    total += 1 + calculate_varint_size(sz) + sz;
  }
  if (!t.path.empty()) {
    for(const auto& e: t.path) {
      std::size_t sz = get_needed_size(e);
      total += 1 + calculate_varint_size(sz) + sz;
    }
  }
  return total;
} // std::size_t get_needed_size(const ::struct_pb_sample::Monster& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::Monster& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::unique_ptr<::struct_pb_sample::Vec3> pos; // message, field number = 1
    if (t.pos) {
      serialize_varint(data, pos, size, 10);
      auto sz = get_needed_size(*t.pos);
      serialize_varint(data, pos, size, sz);
      serialize_to(data + pos, sz, *t.pos);
      pos += sz;
    }
  }
  {
    // int32_t mana; // int32, field number = 2
    if (t.mana != 0) {
      serialize_varint(data, pos, size, 16);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.mana));}
  }
  {
    // int32_t hp; // int32, field number = 3
    if (t.hp != 0) {
      serialize_varint(data, pos, size, 24);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.hp));}
  }
  {
    // std::string name; // string, field number = 4
    if (!t.name.empty()) {
      serialize_varint(data, pos, size, 34);
      serialize_varint(data, pos, size, t.name.size());
      std::memcpy(data + pos, t.name.data(), t.name.size());
      pos += t.name.size();
    }
  }
  {
    // std::string inventory; // bytes, field number = 5
    if (!t.inventory.empty()) {
      serialize_varint(data, pos, size, 42);
      serialize_varint(data, pos, size, t.inventory.size());
      std::memcpy(data + pos, t.inventory.data(), t.inventory.size());
      pos += t.inventory.size();
    }
  }
  {
    // ::struct_pb_sample::Monster::Color color; // enum, field number = 6
    if (t.color != ::struct_pb_sample::Monster::Color::Red) {
      serialize_varint(data, pos, size, 48);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.color));
    }
  }
  {
    // std::vector<::struct_pb_sample::Weapon> weapons; // message, field number = 7
    if (!t.weapons.empty()) {
      for(const auto& e: t.weapons) {
        serialize_varint(data, pos, size, 58);
        std::size_t sz = get_needed_size(e);
        serialize_varint(data, pos, size, sz);
        serialize_to(data+pos, sz, e);
        pos += sz;
      }
    }
  }
  {
    // std::unique_ptr<::struct_pb_sample::Weapon> equipped; // message, field number = 8
    if (t.equipped) {
      serialize_varint(data, pos, size, 66);
      auto sz = get_needed_size(*t.equipped);
      serialize_varint(data, pos, size, sz);
      serialize_to(data + pos, sz, *t.equipped);
      pos += sz;
    }
  }
  {
    // std::vector<::struct_pb_sample::Vec3> path; // message, field number = 9
    if (!t.path.empty()) {
      for(const auto& e: t.path) {
        serialize_varint(data, pos, size, 74);
        std::size_t sz = get_needed_size(e);
        serialize_varint(data, pos, size, sz);
        serialize_to(data+pos, sz, e);
        pos += sz;
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::Monster& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::struct_pb_sample::Monster& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::unique_ptr<::struct_pb_sample::Vec3> pos; // message, field number = 1
      case 10: {
        if (!t.pos) {
          t.pos = std::make_unique<::struct_pb_sample::Vec3>();
        }
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        ok = deserialize_to(*t.pos, data + pos, sz);
        if (!ok) {
          return false;
        }
        pos += sz;
        break;
      }
      // int32_t mana; // int32, field number = 2
      case 16: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.mana = varint_tmp;
        break;
      }
      // int32_t hp; // int32, field number = 3
      case 24: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.hp = varint_tmp;
        break;
      }
      // std::string name; // string, field number = 4
      case 34: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.name.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.name.data(), data+pos, sz);
        pos += sz;
        break;
      }
      // std::string inventory; // bytes, field number = 5
      case 42: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.inventory.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.inventory.data(), data+pos, sz);
        pos += sz;
        break;
      }
      // ::struct_pb_sample::Monster::Color color; // enum, field number = 6
      case 48: {
        uint64_t enum_val_tmp{};
        ok = deserialize_varint(data, pos, size, enum_val_tmp);
        if (!ok) {
          return false;
        }
        t.color = static_cast<::struct_pb_sample::Monster::Color>(enum_val_tmp);
        break;
      }
      // std::vector<::struct_pb_sample::Weapon> weapons; // message, field number = 7
      case 58: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.weapons.emplace_back();
        ok = deserialize_to(t.weapons.back(), data + pos, sz);
        if (!ok) {
          t.weapons.pop_back();
          return false;
        }
        pos += sz;
        break;
      }
      // std::unique_ptr<::struct_pb_sample::Weapon> equipped; // message, field number = 8
      case 66: {
        if (!t.equipped) {
          t.equipped = std::make_unique<::struct_pb_sample::Weapon>();
        }
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        ok = deserialize_to(*t.equipped, data + pos, sz);
        if (!ok) {
          return false;
        }
        pos += sz;
        break;
      }
      // std::vector<::struct_pb_sample::Vec3> path; // message, field number = 9
      case 74: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.path.emplace_back();
        ok = deserialize_to(t.path.back(), data + pos, sz);
        if (!ok) {
          t.path.pop_back();
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
} // bool deserialize_to(::struct_pb_sample::Monster&, const char*, std::size_t)
// end of ::struct_pb_sample::Monster
bool deserialize_to(::struct_pb_sample::Monster& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::struct_pb_sample::Monsters
std::size_t get_needed_size(const ::struct_pb_sample::Monsters& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.monsters.empty()) {
    for(const auto& e: t.monsters) {
      std::size_t sz = get_needed_size(e);
      total += 1 + calculate_varint_size(sz) + sz;
    }
  }
  return total;
} // std::size_t get_needed_size(const ::struct_pb_sample::Monsters& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::Monsters& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<::struct_pb_sample::Monster> monsters; // message, field number = 1
    if (!t.monsters.empty()) {
      for(const auto& e: t.monsters) {
        serialize_varint(data, pos, size, 10);
        std::size_t sz = get_needed_size(e);
        serialize_varint(data, pos, size, sz);
        serialize_to(data+pos, sz, e);
        pos += sz;
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::Monsters& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::struct_pb_sample::Monsters& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<::struct_pb_sample::Monster> monsters; // message, field number = 1
      case 10: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.monsters.emplace_back();
        ok = deserialize_to(t.monsters.back(), data + pos, sz);
        if (!ok) {
          t.monsters.pop_back();
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
} // bool deserialize_to(::struct_pb_sample::Monsters&, const char*, std::size_t)
// end of ::struct_pb_sample::Monsters
bool deserialize_to(::struct_pb_sample::Monsters& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::struct_pb_sample::rect32
std::size_t get_needed_size(const ::struct_pb_sample::rect32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.x != 0) {
    total += 1 + calculate_varint_size(t.x);
  }
  if (t.y != 0) {
    total += 1 + calculate_varint_size(t.y);
  }
  if (t.width != 0) {
    total += 1 + calculate_varint_size(t.width);
  }
  if (t.height != 0) {
    total += 1 + calculate_varint_size(t.height);
  }
  return total;
} // std::size_t get_needed_size(const ::struct_pb_sample::rect32& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::rect32& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int32_t x; // int32, field number = 1
    if (t.x != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.x));}
  }
  {
    // int32_t y; // int32, field number = 2
    if (t.y != 0) {
      serialize_varint(data, pos, size, 16);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.y));}
  }
  {
    // int32_t width; // int32, field number = 3
    if (t.width != 0) {
      serialize_varint(data, pos, size, 24);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.width));}
  }
  {
    // int32_t height; // int32, field number = 4
    if (t.height != 0) {
      serialize_varint(data, pos, size, 32);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.height));}
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::rect32& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::struct_pb_sample::rect32& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int32_t x; // int32, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.x = varint_tmp;
        break;
      }
      // int32_t y; // int32, field number = 2
      case 16: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.y = varint_tmp;
        break;
      }
      // int32_t width; // int32, field number = 3
      case 24: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.width = varint_tmp;
        break;
      }
      // int32_t height; // int32, field number = 4
      case 32: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.height = varint_tmp;
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
} // bool deserialize_to(::struct_pb_sample::rect32&, const char*, std::size_t)
// end of ::struct_pb_sample::rect32
bool deserialize_to(::struct_pb_sample::rect32& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::struct_pb_sample::rect32s
std::size_t get_needed_size(const ::struct_pb_sample::rect32s& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.rect32_list.empty()) {
    for(const auto& e: t.rect32_list) {
      std::size_t sz = get_needed_size(e);
      total += 1 + calculate_varint_size(sz) + sz;
    }
  }
  return total;
} // std::size_t get_needed_size(const ::struct_pb_sample::rect32s& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::rect32s& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<::struct_pb_sample::rect32> rect32_list; // message, field number = 1
    if (!t.rect32_list.empty()) {
      for(const auto& e: t.rect32_list) {
        serialize_varint(data, pos, size, 10);
        std::size_t sz = get_needed_size(e);
        serialize_varint(data, pos, size, sz);
        serialize_to(data+pos, sz, e);
        pos += sz;
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::rect32s& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::struct_pb_sample::rect32s& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<::struct_pb_sample::rect32> rect32_list; // message, field number = 1
      case 10: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.rect32_list.emplace_back();
        ok = deserialize_to(t.rect32_list.back(), data + pos, sz);
        if (!ok) {
          t.rect32_list.pop_back();
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
} // bool deserialize_to(::struct_pb_sample::rect32s&, const char*, std::size_t)
// end of ::struct_pb_sample::rect32s
bool deserialize_to(::struct_pb_sample::rect32s& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::struct_pb_sample::person
std::size_t get_needed_size(const ::struct_pb_sample::person& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (t.id != 0) {
    total += 1 + calculate_varint_size(t.id);
  }
  if (!t.name.empty()) {
    total += 1 + calculate_varint_size(t.name.size()) + t.name.size();
  }
  if (t.age != 0) {
    total += 1 + calculate_varint_size(t.age);
  }
  if (t.salary != 0) {
    total += 1 + 8;
  }
  return total;
} // std::size_t get_needed_size(const ::struct_pb_sample::person& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::person& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // int32_t id; // int32, field number = 1
    if (t.id != 0) {
      serialize_varint(data, pos, size, 8);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.id));}
  }
  {
    // std::string name; // string, field number = 2
    if (!t.name.empty()) {
      serialize_varint(data, pos, size, 18);
      serialize_varint(data, pos, size, t.name.size());
      std::memcpy(data + pos, t.name.data(), t.name.size());
      pos += t.name.size();
    }
  }
  {
    // int32_t age; // int32, field number = 3
    if (t.age != 0) {
      serialize_varint(data, pos, size, 24);
      serialize_varint(data, pos, size, static_cast<uint64_t>(t.age));}
  }
  {
    // double salary; // double, field number = 4
    if (t.salary != 0) {
      serialize_varint(data, pos, size, 33);
      std::memcpy(data + pos, &t.salary, 8);
      pos += 8;
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::person& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::struct_pb_sample::person& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // int32_t id; // int32, field number = 1
      case 8: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.id = varint_tmp;
        break;
      }
      // std::string name; // string, field number = 2
      case 18: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.name.resize(sz);
        if (pos + sz > size) {
          return false;
        }
        std::memcpy(t.name.data(), data+pos, sz);
        pos += sz;
        break;
      }
      // int32_t age; // int32, field number = 3
      case 24: {
        uint64_t varint_tmp = 0;
        ok = deserialize_varint(data, pos, size, varint_tmp);
        if (!ok) {
          return false;
        }
        t.age = varint_tmp;
        break;
      }
      // double salary; // double, field number = 4
      case 33: {
        if (pos + 8 > size) {
          return false;
        }
        double fixed_tmp = 0;
        std::memcpy(&fixed_tmp, data + pos, 8);
        pos += 8;
        t.salary = fixed_tmp;
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
} // bool deserialize_to(::struct_pb_sample::person&, const char*, std::size_t)
// end of ::struct_pb_sample::person
bool deserialize_to(::struct_pb_sample::person& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

// ::struct_pb_sample::persons
std::size_t get_needed_size(const ::struct_pb_sample::persons& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  if (!t.person_list.empty()) {
    for(const auto& e: t.person_list) {
      std::size_t sz = get_needed_size(e);
      total += 1 + calculate_varint_size(sz) + sz;
    }
  }
  return total;
} // std::size_t get_needed_size(const ::struct_pb_sample::persons& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::persons& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  {
    // std::vector<::struct_pb_sample::person> person_list; // message, field number = 1
    if (!t.person_list.empty()) {
      for(const auto& e: t.person_list) {
        serialize_varint(data, pos, size, 10);
        std::size_t sz = get_needed_size(e);
        serialize_varint(data, pos, size, sz);
        serialize_to(data+pos, sz, e);
        pos += sz;
      }
    }
  }
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::struct_pb_sample::persons& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::struct_pb_sample::persons& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      // std::vector<::struct_pb_sample::person> person_list; // message, field number = 1
      case 10: {
        std::size_t sz = 0;
        ok = deserialize_varint(data, pos, size, sz);
        if (!ok) {
          return false;
        }
        t.person_list.emplace_back();
        ok = deserialize_to(t.person_list.back(), data + pos, sz);
        if (!ok) {
          t.person_list.pop_back();
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
} // bool deserialize_to(::struct_pb_sample::persons&, const char*, std::size_t)
// end of ::struct_pb_sample::persons
bool deserialize_to(::struct_pb_sample::persons& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

} // internal
} // struct_pb
