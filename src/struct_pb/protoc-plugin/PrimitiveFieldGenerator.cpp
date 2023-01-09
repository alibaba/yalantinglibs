#include "PrimitiveFieldGenerator.h"

namespace struct_pb {
namespace compiler {
std::string get_type_name_help(FieldDescriptor::Type type) {
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
    default: {
      // can't reach, dead path
      return "not support";
    }
  }
}
PrimitiveFieldGenerator::PrimitiveFieldGenerator(
    const FieldDescriptor *descriptor, const Options &options)
    : FieldGenerator(descriptor, options) {}
void PrimitiveFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  if (is_optional()) {
    format("if ($1$.has_value()) {\n", value);
    format.indent();
    format("total += $1$ + ", calculate_tag_size(d_));
    generate_calculate_size_only(p, value + ".value()");
    format(";\n");
    format.outdent();
    format("}\n");
  }
  else {
    if (can_ignore_default_value) {
      format("if ($1$ != 0) {\n", value);
      format.indent();
      format("total += $1$ + ", calculate_tag_size(d_));
      generate_calculate_size_only(p, value);
      format(";\n");
      format.outdent();
      format("}\n");
    }
    else {
      format("total += $1$ + ", calculate_tag_size(d_));
      generate_calculate_size_only(p, value);
      format(";\n");
    }
  }
}
void PrimitiveFieldGenerator::generate_calculate_size_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  if (is_varint(d_)) {
    if (is_sint(d_)) {
      format("calculate_varint_size(encode_zigzag($1$))", value);
    }
    else if (is_bool(d_)) {
      format("calculate_varint_size(static_cast<uint64_t>($1$))", value);
    }
    else {
      format("calculate_varint_size($1$)", value);
    }
  }
  else if (is_i64(d_)) {
    format("8");
  }
  else if (is_i32(d_)) {
    format("4");
  }
}

std::string PrimitiveFieldGenerator::cpp_type_name() const {
  std::string type_name = get_type_name_help(d_->type());
  if (is_optional()) {
    type_name = "std::optional<" + type_name + ">";
  }
  return type_name;
}
void PrimitiveFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  if (is_optional()) {
    format("if ($1$.has_value()) {\n", value);
    format.indent();
    format("serialize_varint(data, pos, size, $1$);\n", calculate_tag_str(d_));
    generate_serialization_only(p, value + ".value()");
    format("\n");
    format.outdent();
    format("}\n");
  }
  else {
    if (can_ignore_default_value) {
      format("if ($1$ != 0) {\n", value);
      format.indent();
      format("serialize_varint(data, pos, size, $1$);\n",
             calculate_tag_str(d_));
      generate_serialization_only(p, value);
      format("\n");
      format.outdent();
      format("}\n");
    }
    else {
      format("serialize_varint(data, pos, size, $1$);\n",
             calculate_tag_str(d_));
      generate_serialization_only(p, value);
    }
  }
}
void PrimitiveFieldGenerator::generate_serialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  if (is_varint(d_)) {
    if (is_sint(d_)) {
      format("serialize_varint(data, pos, size, encode_zigzag($1$));", value);
    }
    else if (is_bool(d_)) {
      format("serialize_varint(data, pos, size, static_cast<uint64_t>($1$));",
             value);
    }
    else {
      if (d_->cpp_type() != FieldDescriptor::CPPTYPE_UINT64) {
        format("serialize_varint(data, pos, size, static_cast<uint64_t>($1$));",
               value);
      }
      else {
        format("serialize_varint(data, pos, size, $1$);", value);
      }
    }
  }
  else if (is_i64(d_) || is_i32(d_)) {
    std::size_t sz = is_i64(d_) ? 8 : 4;
    p->Print({{"value", value}, {"sz", std::to_string(sz)}}, R"(
std::memcpy(data + pos, &$value$, $sz$);
pos += $sz$;
)");
  }
}
void PrimitiveFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  format("case $1$: {\n", calculate_tag_str(d_));
  format.indent();
  generate_deserialization_only(p, value);
  format("break;\n");
  format.outdent();
  format("}\n");
}
void PrimitiveFieldGenerator::generate_deserialization_only(
    google::protobuf::io::Printer *p, const std::string &output,
    const std::string &max_size) const {
  if (is_varint(d_)) {
    p->Print({{"output", output},
              {"type_name", get_type_name_help(d_->type())},
              {"max_size", max_size}},
             R"(
uint64_t varint_tmp = 0;
ok = deserialize_varint(data, pos, $max_size$, varint_tmp);
if (!ok) {
  return false;
}
)");
    if (is_sint(d_)) {
      if (is_sint32(d_)) {
        p->Print(
            {

                {"output", output},
                {"type_name", get_type_name_help(d_->type())},
                {"max_size", max_size}},
            R"(
$output$ = static_cast<$type_name$>(decode_zigzag(uint32_t(varint_tmp)));
)");
      }
      else {
        p->Print(
            {

                {"output", output},
                {"type_name", get_type_name_help(d_->type())},
                {"max_size", max_size}},
            R"(
$output$ = static_cast<$type_name$>(decode_zigzag(varint_tmp));
)");
      }
    }
    else if (is_bool(d_)) {
      p->Print(
          {

              {"output", output},
              {"type_name", get_type_name_help(d_->type())},
              {"max_size", max_size}},
          R"(
$output$ = static_cast<$type_name$>(varint_tmp);
)");
    }
    else {
      p->Print(
          {

              {"output", output},
              {"type_name", get_type_name_help(d_->type())},
              {"max_size", max_size}},
          R"(
$output$ = varint_tmp;
)");
    }
  }
  else if (is_i64(d_) || is_i32(d_)) {
    auto sz = is_i64(d_) ? 8 : 4;
    p->Print(
        {

            {"output", output},
            {"type_name", get_type_name_help(d_->type())},
            {"max_size", max_size},
            {"sz", std::to_string(sz)}},
        R"(
if (pos + $sz$ > $max_size$) {
  return false;
}
$type_name$ fixed_tmp = 0;
std::memcpy(&fixed_tmp, data + pos, $sz$);
pos += $sz$;
$output$ = fixed_tmp;
)");
  }
}
RepeatedPrimitiveFieldGenerator::RepeatedPrimitiveFieldGenerator(
    const FieldDescriptor *descriptor, const Options &options)
    : FieldGenerator(descriptor, options) {}
void RepeatedPrimitiveFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  PrimitiveFieldGenerator g(d_, options_);
  if (is_packed()) {
    format("if (!$1$.empty()) {\n", value);
    format.indent();
    generate_calculate_packed_size_only(p, value);
    format(
        "total += $1$ + calculate_varint_size(container_total) + "
        "container_total;\n",
        calculate_tag_size(d_));
    format.outdent();
    format("}\n");
  }
  else {
    format("if (!$1$.empty()) {\n", value);
    format.indent();
    generate_calculate_unpacked_size_only(p, value);
    format("total += container_total;\n");
    format.outdent();
    format("}\n");
  }
}
bool RepeatedPrimitiveFieldGenerator::is_packed() const {
  return d_->is_packable() && d_->is_packed();
}
std::string RepeatedPrimitiveFieldGenerator::cpp_type_name() const {
  return "std::vector<" + get_type_name_help(d_->type()) + ">";
}
void RepeatedPrimitiveFieldGenerator::generate_calculate_packed_size_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  PrimitiveFieldGenerator g(d_, options_);
  if (is_varint(d_)) {
    p->Print({{"value", value}},
             R"(
std::size_t container_total = 0;
for(const auto& e: $value$) {
  container_total += )");
    g.generate_calculate_size_only(p, "e");
    format(";\n");
    format("}\n");
  }
  else if (is_i64(d_) || is_i32(d_)) {
    auto sz = is_i64(d_) ? 8 : 4;
    p->Print({{"sz", std::to_string(sz)}, {"value", value}}, R"(
std::size_t container_total = $sz$ * $value$.size();
)");
  }
}
void RepeatedPrimitiveFieldGenerator::generate_calculate_unpacked_size_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  PrimitiveFieldGenerator g(d_, options_);
  Formatter format(p);
  if (is_varint(d_)) {
    p->Print({{"value", value}, {"tag_sz", calculate_tag_size(d_, true)}},
             R"(
std::size_t container_total = 0;
for(const auto& e: $value$) {
  container_total += $tag_sz$;
  container_total += )");
    g.generate_calculate_size_only(p, "e");
    format(";\n");
    format("}\n");
  }
  else if (is_i64(d_) || is_i32(d_)) {
    auto sz = is_i64(d_) ? 8 : 4;
    p->Print({{"sz", std::to_string(sz)},
              {"value", value},
              {"tag_sz", calculate_tag_size(d_, true)}},
             R"(
std::size_t container_total = ($tag_sz$ + $sz$) * $value$.size();
)");
  }
}
void RepeatedPrimitiveFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  PrimitiveFieldGenerator g(d_, options_);
  Formatter format(p);
  if (is_packed()) {
    if (is_varint(d_)) {
      format("if (!$1$.empty()) {\n", value);
      format.indent();
      format("serialize_varint(data, pos, size, $1$);\n",
             calculate_tag_str(d_));
      generate_calculate_packed_size_only(p, value);
      format("serialize_varint(data, pos, size, container_total);\n");
      format("for(const auto& v: $1$) {\n", value);
      format.indent();
      g.generate_serialization_only(p, "v");
      format.outdent();
      format("}\n");
      format.outdent();
      format("}\n");
    }
    else if (is_i64(d_) || is_i32(d_)) {
      format("if (!$1$.empty()) {\n", value);
      format.indent();
      format("serialize_varint(data, pos, size, $1$);\n",
             calculate_tag_str(d_));
      generate_calculate_packed_size_only(p, value);
      format("serialize_varint(data, pos, size, container_total);\n");
      format("std::memcpy(data + pos, $1$.data(), container_total);\n", value);
      format.outdent();
      format("}\n");
    }
  }
  else {
    format("for(const auto& v: $1$) {\n", value);
    format.indent();
    format("serialize_varint(data, pos, size, $1$);\n",
           calculate_tag_str(d_, true));
    g.generate_serialization_only(p, "v");
    format.outdent();
    format("}\n");
  }
}
void RepeatedPrimitiveFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  format("case $1$: {\n", unpacked_tag());
  format.indent();
  format("$1$ e{};\n", get_type_name_help(d_->type()));
  generate_deserialization_unpacked_only(p, "e");
  format("$1$.push_back(e);\n", value);
  format("break;\n");
  format.outdent();
  format("}\n");

  format("case $1$: {\n", packed_tag());
  format.indent();
  format("uint64_t sz = 0;\n");
  format("ok = deserialize_varint(data, pos, size, sz);\n");
  format("if (!ok) {\n");
  format.indent();
  format("return false;\n");
  format.outdent();
  format("}\n");
  format("std::size_t cur_max_size = pos + sz;\n");
  generate_deserialization_packed_only(p, value, "cur_max_size");
  format("break;\n");
  format.outdent();
  format("}\n");
}
void RepeatedPrimitiveFieldGenerator::generate_deserialization_packed_only(
    google::protobuf::io::Printer *p, const std::string &value,
    const std::string &max_size) const {
  if (is_varint(d_)) {
    Formatter format(p);
    format("while (pos < $1$) {\n", max_size);
    format.indent();
    format("$1$ e{};\n", get_type_name_help(d_->type()));
    PrimitiveFieldGenerator g(d_, options_);
    g.generate_deserialization_only(p, "e");
    format("$1$.push_back(e);\n", value);
    format.outdent();
    format("}\n");
  }
  else if (is_i64(d_) || is_i32(d_)) {
    auto sz = is_i64(d_) ? 8 : 4;
    p->Print({{"value", value}, {"sz", std::to_string(sz)}}, R"(
int count = sz / $sz$;
if ($sz$ * count != sz) {
  return false;
}
if (pos + sz > size) {
  return false;
}
$value$.resize(count);
std::memcpy($value$.data(), data + pos, sz);
pos += sz;
)");
  }
}
std::string RepeatedPrimitiveFieldGenerator::packed_tag() const {
  uint32_t number = d_->number();
  uint32_t wire_type = 2;
  auto tag = number << 3 | wire_type;
  return std::to_string(tag);
}
std::string RepeatedPrimitiveFieldGenerator::unpacked_tag() const {
  uint32_t number = d_->number();
  uint32_t wire_type = 0;
  if (is_varint(d_)) {
    wire_type = 0;
  }
  else if (is_i64(d_)) {
    wire_type = 1;
  }
  else if (is_i32(d_)) {
    wire_type = 5;
  }
  auto tag = number << 3 | wire_type;
  return std::to_string(tag);
}
void RepeatedPrimitiveFieldGenerator::generate_deserialization_unpacked_only(
    google::protobuf::io::Printer *p, const std::string &output) const {
  PrimitiveFieldGenerator g(d_, options_);
  g.generate_deserialization_only(p, output);
}
}  // namespace compiler
}  // namespace struct_pb