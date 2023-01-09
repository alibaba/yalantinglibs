#include "MapFieldGenerator.h"

#include "EnumFieldGenerator.h"
#include "MessageFieldGenerator.h"
#include "PrimitiveFieldGenerator.h"
#include "StringFieldGenerator.h"
namespace struct_pb {
namespace compiler {
MapFieldGenerator::MapFieldGenerator(const FieldDescriptor *field,
                                     const Options &options)
    : FieldGenerator(field, options) {}

static std::string get_type_name_help(FieldDescriptor::Type type) {
  switch (type) {
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
      return "double";
    case google::protobuf::FieldDescriptor::TYPE_FLOAT:
      return "float";
    case google::protobuf::FieldDescriptor::TYPE_INT64:
    case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
    case google::protobuf::FieldDescriptor::TYPE_SINT64:
      return "int64_t";
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
    case google::protobuf::FieldDescriptor::TYPE_FIXED64:
      return "uint64_t";
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
    case google::protobuf::FieldDescriptor::TYPE_FIXED32:
      return "uint32_t";
    case google::protobuf::FieldDescriptor::TYPE_INT32:
    case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
    case google::protobuf::FieldDescriptor::TYPE_SINT32:
      return "int32_t";
    case google::protobuf::FieldDescriptor::TYPE_BOOL:
      return "bool";
    case google::protobuf::FieldDescriptor::TYPE_STRING:
    case google::protobuf::FieldDescriptor::TYPE_BYTES:
      return "std::string";
    case google::protobuf::FieldDescriptor::TYPE_GROUP:
      // workaround for group
      return "std::string";
    default: {
      return "not support now";
    }
  }
}
std::string MapFieldGenerator::cpp_type_name() const {
  return "std::map<" + get_key_type_name() + ", " + get_value_type_name() + ">";
}
std::string MapFieldGenerator::get_value_type_name() const {
  return get_kv_type_name_helper(d_->message_type()->field(1));
}
std::string MapFieldGenerator::get_key_type_name() const {
  return get_kv_type_name_helper(d_->message_type()->field(0));
}
std::string MapFieldGenerator::get_kv_type_name_helper(
    const FieldDescriptor *f) const {
  if (f->type() == FieldDescriptor::TYPE_MESSAGE) {
    return qualified_class_name(f->message_type(), options_);
  }
  else if (f->type() == FieldDescriptor::TYPE_ENUM) {
    return qualified_enum_name(f->enum_type(), options_);
  }
  else {
    return get_type_name_help(f->type());
  }
}
void MapFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  generate_calculate_size_only(p, value);
}
void MapFieldGenerator::generate_calculate_size_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  format("for(const auto& e: $1$) {\n", value);
  format.indent();
  format("std::size_t map_entry_sz = 0;\n");
  auto key_f = d_->message_type()->field(0);
  auto value_f = d_->message_type()->field(1);
  generate_calculate_kv_size(p, key_f, "e.first");
  generate_calculate_kv_size(p, value_f, "e.second");
  format("total += $1$ + calculate_varint_size(map_entry_sz) + map_entry_sz;",
         calculate_tag_size(d_));
  format.outdent();
  format("}\n");
}
void MapFieldGenerator::generate_calculate_kv_size(
    google::protobuf::io::Printer *p, const FieldDescriptor *f,
    const std::string &value) const {
  Formatter format(p);
  switch (f->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      p->Print({{"value", value}, {"tag_sz", calculate_tag_size(f)}}, R"(
std::size_t sz = get_needed_size($value$);
map_entry_sz += $tag_sz$ + calculate_varint_size(sz) + sz;
)");
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      format(
          "map_entry_sz += $1$ + calculate_varint_size($2$.size()) + "
          "$2$.size();\n",
          calculate_tag_size(f), value);
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      p->Print({{"tag_sz", calculate_tag_size(f)}},
               R"(
map_entry_sz += $tag_sz$ + )");
      EnumFieldGenerator(f, options_).generate_calculate_size_only(p, value);
      format(";\n");
      break;
    }
    default: {
      format("map_entry_sz += $1$ + ", calculate_tag_size(f));
      PrimitiveFieldGenerator(f, options_)
          .generate_calculate_size_only(p, value);
      format(";\n");
    }
  }
}
void MapFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  generate_serialization_only(p, value);
}
void MapFieldGenerator::generate_serialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  auto key_f = d_->message_type()->field(0);
  auto value_f = d_->message_type()->field(1);
  std::unique_ptr<FieldGenerator> kg;
  kg.reset(FieldGeneratorMap::MakeGenerator(key_f, options_));
  std::unique_ptr<FieldGenerator> vg;
  kg.reset(FieldGeneratorMap::MakeGenerator(value_f, options_));
  Formatter format(p);
  format("for(const auto& e: $1$) {\n", value);
  format.indent();
  format("serialize_varint(data, pos, size, $1$);\n", calculate_tag_str(d_));
  format("std::size_t map_entry_sz = 0;\n");
  format("{\n");
  format.indent();
  generate_calculate_kv_size(p, key_f, "e.first");
  format.outdent();
  format("}\n");
  format("{\n");
  format.indent();
  generate_calculate_kv_size(p, value_f, "e.second");
  format.outdent();
  format("}\n");

  format("serialize_varint(data, pos, size, map_entry_sz);\n");
  format("{\n");
  format.indent();
  generate_serialize_kv_only(p, key_f, "e.first");
  format.outdent();
  format("}\n");
  format("{\n");
  format.indent();
  generate_serialize_kv_only(p, value_f, "e.second");
  format.outdent();
  format("}\n");

  format.outdent();
  format("}\n");
}
void MapFieldGenerator::generate_serialize_kv_only(
    google::protobuf::io::Printer *p, const FieldDescriptor *f,
    const std::string &value) const {
  switch (f->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      p->Print({{"value", value}, {"tag", calculate_tag_str(f)}}, R"(
serialize_varint(data, pos, size, $tag$);
std::size_t sz = get_needed_size($value$);
serialize_varint(data, pos, size, sz);
serialize_to(data + pos, sz, $value$);
pos += sz;
)");
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      StringFieldGenerator(f, options_).generate_serialization_only(p, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      EnumFieldGenerator(f, options_).generate_serialization_only(p, value);
      break;
    }
    default: {
      p->Print({{"tag", calculate_tag_str(f)}},
               R"(
serialize_varint(data, pos, size, $tag$);
)");
      PrimitiveFieldGenerator(f, options_)
          .generate_serialization_only(p, value);
    }
  }
}
void MapFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  p->Print({{"tag", calculate_tag_str(d_)}},
           R"(
case $tag$: {
  uint64_t sz = 0;
  ok = deserialize_varint(data, pos, size, sz);
  if (!ok) {
    return false;
  }
  std::size_t cur_max_size = pos + sz;
)");
  generate_deserialization_only(p, value);
  p->Print(R"(
break;
}
)");
}
void MapFieldGenerator::generate_deserialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  auto key_f = d_->message_type()->field(0);
  auto value_f = d_->message_type()->field(1);
  Formatter format(p);
  format("$1$ key_tmp_val{};\n", get_key_type_name());
  format("$1$ value_tmp_val{};\n", get_value_type_name());
  p->Print(
      {
          {"key_tag", calculate_tag_str(key_f)},
      },
      R"(
while (pos < cur_max_size) {
  ok = read_tag(data, pos, size, tag);
  if (!ok) {
    return false;
  }
  switch(tag) {
    case $key_tag$: {
)");
  generate_deserialize_kv_only(p, key_f, "key_tmp_val", "cur_max_size");
  p->Print(
      {
          {"value_tag", calculate_tag_str(value_f)},
      },
      R"(
break;
}
case $value_tag$: {
)");
  generate_deserialize_kv_only(p, value_f, "value_tmp_val", "cur_max_size");
  p->Print({{"value", value}}, R"(
break;
    }

    default: {
      return false;
    }
  }
}
$value$[key_tmp_val] = std::move(value_tmp_val);
)");
}
void MapFieldGenerator::generate_deserialize_kv_only(
    google::protobuf::io::Printer *p, const FieldDescriptor *f,
    const std::string &value, const std::string &max_size) const {
  switch (f->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      p->Print({{"value", value},
                {"tag", calculate_tag_str(f)},
                {"max_size", max_size}},
               R"(
uint64_t msg_sz = 0;
ok = deserialize_varint(data, pos, $max_size$, msg_sz);
if (!ok) {
  return false;
}
ok = deserialize_to($value$, data + pos, msg_sz);
if (!ok) {
  return false;
}
pos += msg_sz;
)");
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      StringFieldGenerator(f, options_)
          .generate_deserialization_only(p, value, "str_sz", max_size);
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      EnumFieldGenerator(f, options_)
          .generate_deserialization_only(p, value, max_size);
      break;
    }
    default: {
      PrimitiveFieldGenerator(f, options_)
          .generate_deserialization_only(p, value);
    }
  }
}
}  // namespace compiler
}  // namespace struct_pb