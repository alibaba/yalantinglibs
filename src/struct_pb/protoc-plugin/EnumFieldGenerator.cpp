#include "EnumFieldGenerator.h"

#include "helpers.hpp"
namespace struct_pb {
namespace compiler {
EnumFieldGenerator::EnumFieldGenerator(const FieldDescriptor *descriptor,
                                       const Options &options)
    : FieldGenerator(descriptor, options) {}
std::string EnumFieldGenerator::cpp_type_name() const {
  auto name = qualified_enum_name(d_->enum_type(), options_);
  if (is_optional()) {
    return "std::optional<" + name + ">";
  }
  return name;
}
void EnumFieldGenerator::generate_calculate_size(
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
      p->Print(
          {
              {"tag_sz", calculate_tag_size(d_)},
              {"value", value},
              {"default_value", default_value()},
          },
          R"(
if ($value$ != $default_value$) {
  total += $tag_sz$ + )");
      generate_calculate_size_only(p, value);
      p->Print(R"(;
}
)");
    }
    else {
      p->Print(
          {
              {"tag_sz", calculate_tag_size(d_)},
          },
          R"(
total += $tag_sz$ + )");
      generate_calculate_size_only(p, value);
      p->Print(R"(;
}
)");
    }
  }
}
void EnumFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  // auto v = p->WithVars({{"tag", calculate_tag(d_)}, {"value", value}});
  if (is_optional()) {
    p->Print({{"tag", calculate_tag_str(d_)}, {"value", value}}, R"(
if ($value$.has_value()) {
)");
    generate_serialization_only(p, value + ".value()");
    p->Print(R"(
}
)");
  }
  else {
    if (can_ignore_default_value) {
      p->Print({{"tag", calculate_tag_str(d_)},
                {"value", value},
                {"default_value", default_value()}},
               R"(
if ($value$ != $default_value$) {
)");
      generate_serialization_only(p, value);
      p->Print(R"(
}
)");
    }
    else {
      generate_serialization_only(p, value);
    }
  }
}
void EnumFieldGenerator::generate_serialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  p->Print({{"tag", calculate_tag_str(d_)}, {"value", value}}, R"(
serialize_varint(data, pos, size, $tag$);
serialize_varint(data, pos, size, static_cast<uint64_t>($value$));
)");
}
void EnumFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  p->Print(
      {
          {"tag", calculate_tag_str(d_)},
          {"value", value},
      },
      R"(
case $tag$: {
)");
  generate_deserialization_only(p, value);
  p->Print(R"(
  break;
}
)");
}
void EnumFieldGenerator::generate_deserialization_only(
    google::protobuf::io::Printer *p, const std::string &output,
    const std::string &max_size) const {
  auto enum_name = qualified_enum_name(d_->enum_type(), options_);
  p->Print(
      {{"enum_name", enum_name}, {"output", output}, {"max_size", max_size}},
      R"(
uint64_t enum_val_tmp{};
ok = deserialize_varint(data, pos, $max_size$, enum_val_tmp);
if (!ok) {
  return false;
}
$output$ = static_cast<$enum_name$>(enum_val_tmp);
)");
}
RepeatedEnumFieldGenerator::RepeatedEnumFieldGenerator(
    const FieldDescriptor *descriptor, const Options &options)
    : FieldGenerator(descriptor, options) {}
std::string RepeatedEnumFieldGenerator::cpp_type_name() const {
  return "std::vector<" + qualified_enum_name(d_->enum_type(), options_) + ">";
}

void RepeatedEnumFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  if (can_ignore_default_value) {
    if (is_packed()) {
      p->Print({{"value", value}}, R"(
if (!$value$.empty()) { // packed
)");
      generate_calculate_size_only(p, value, "sz");
      p->Print(
          {
              {"tag_sz", calculate_tag_size(d_)},
          },
          R"(
total += $tag_sz$ + calculate_varint_size(sz) + sz;
}
)");
    }
    else {
      p->Print({{"value", value}}, R"(
if (!$value$.empty()) { // unpacked
)");
      generate_calculate_size_only(p, value, "sz");
      p->Print(
          {
              {"tag_sz", calculate_tag_size(d_)},
          },
          R"(
  total += sz;
}
)");
    }
  }
  else {
    if (is_packed()) {
      generate_calculate_size_only(p, value, "sz");
      p->Print(
          {
              {"tag_sz", calculate_tag_size(d_)},
          },
          R"(
// packed
total += $tag_sz$ + calculate_varint_size(sz) + sz;
)");
    }
    else {
      generate_calculate_size_only(p, value, "sz");
      p->Print(
          R"(
// unpacked
total += sz;
)");
    }
  }
}
void RepeatedEnumFieldGenerator::generate_calculate_size_only(
    google::protobuf::io::Printer *p, const std::string &value,
    const std::string &output) const {
  if (is_packed()) {
    p->Print({{"value", value},
              {"output", output},
              {"tag_sz", calculate_tag_size(d_)}},
             R"(
std::size_t $output$ = 0;
for(const auto& v: $value$) {
  $output$ += calculate_varint_size(static_cast<uint64_t>(v));
}
)");
  }
  else {
    p->Print({{"value", value},
              {"output", output},
              {"tag_sz", calculate_tag_size(d_, true)}},
             R"(
std::size_t $output$ = 0;
for(const auto& v: $value$) {
  $output$ += $tag_sz$;
  $output$ += calculate_varint_size(static_cast<uint64_t>(v));
}
)");
  }
}

void RepeatedEnumFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  if (can_ignore_default_value) {
    p->Print({{"value", value}}, R"(
if (!$value$.empty()) {
)");
    generate_serialization_only(p, value);
    p->Print("}\n");
  }
  else {
    generate_serialization_only(p, value);
  }
}
void RepeatedEnumFieldGenerator::generate_serialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  if (is_packed()) {
    p->Print({{"tag", std::to_string(calculate_tag(d_))}}, R"(
serialize_varint(data, pos, size, $tag$);
)");
    generate_calculate_size_only(p, value, "sz");
    p->Print({{"value", value}},
             R"(
serialize_varint(data, pos, size, sz);
for(const auto& v: $value$) {
  serialize_varint(data, pos, size, static_cast<uint64_t>(v));
}
)");
  }
  else {
    p->Print({{"tag", std::to_string(calculate_tag(d_, true))}}, R"(
for(const auto& v: $value$) {
  serialize_varint(data, pos, size, $tag$);
  serialize_varint(data, pos, size, static_cast<uint64_t>(v));
}
)");
  }
}
void RepeatedEnumFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  // generate packed
  p->Print({{"tag", packed_tag()},
            {"value", value},
            {"enum_name", qualified_enum_name(d_->enum_type(), options_)}

           },
           R"(
case $tag$: {
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
    $value$.push_back(static_cast<$enum_name$>(enum_val_tmp));
  }
  break;
}
)");
  p->Print({{"tag", unpacked_tag()},
            {"value", value},
            {"enum_name", qualified_enum_name(d_->enum_type(), options_)}},
           R"(
case $tag$: {
  uint64_t enum_val_tmp{};
  ok = deserialize_varint(data, pos, size, enum_val_tmp);
  if (!ok) {
    return false;
  }
  $value$.push_back(static_cast<$enum_name$>(enum_val_tmp));
  break;
}
)");
}
bool RepeatedEnumFieldGenerator::is_packed() const {
  return d_->is_packable() && d_->is_packed();
}
std::string RepeatedEnumFieldGenerator::packed_tag() const {
  uint32_t number = d_->number();
  uint32_t wire_type = 2;
  auto tag = number << 3 | wire_type;
  return std::to_string(tag);
}
std::string RepeatedEnumFieldGenerator::unpacked_tag() const {
  uint32_t number = d_->number();
  uint32_t wire_type = 0;
  auto tag = number << 3 | wire_type;
  return std::to_string(tag);
}
std::string EnumFieldGenerator::default_value() const {
  return qualified_enum_name(d_->enum_type(), options_) +
         "::" + d_->default_value_enum()->name();
}
void EnumFieldGenerator::generate_calculate_size_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  p->Print({{"value", value}},
           "calculate_varint_size(static_cast<uint64_t>($value$))");
}
}  // namespace compiler
}  // namespace struct_pb