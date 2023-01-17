#include "StringFieldGenerator.h"

namespace struct_pb {
namespace compiler {
StringFieldGenerator::StringFieldGenerator(const FieldDescriptor *field,
                                           const Options &options)
    : FieldGenerator(field, options) {}
void StringFieldGenerator::generate_calculate_size_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  format("$1$.size()", value);
}
void StringFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  if (can_ignore_default_value) {
    format("if (!$1$.empty()) {\n", value);
    format.indent();
    format("total += $1$ + calculate_varint_size($2$.size()) + $2$.size();\n",
           calculate_tag_size(d_), value);
    format.outdent();
    format("}\n");
  }
  else {
    format("total += $1$ + calculate_varint_size($2$.size()) + $2$.size();\n",
           calculate_tag_size(d_), value);
  }
}
void StringFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  if (can_ignore_default_value) {
    format("if (!$1$.empty()) {\n", value);
    format.indent();
    generate_serialization_only(p, value);
    format.outdent();
    format("}\n");
  }
  else {
    generate_serialization_only(p, value);
  }
}
void StringFieldGenerator::generate_serialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  format("serialize_varint(data, pos, size, $1$);\n", calculate_tag_str(d_));
  format("serialize_varint(data, pos, size, $1$.size());\n", value);
  format("std::memcpy(data + pos, $1$.data(), $1$.size());\n", value);
  format("pos += $1$.size();\n", value);
}
std::string StringFieldGenerator::cpp_type_name() const {
  return "std::string";
}
void StringFieldGenerator::generate_deserialization_only(
    google::protobuf::io::Printer *p, const std::string &output,
    const std::string &sz, const std::string &max_size) const {
  p->Print({{"output", output}, {"sz", sz}, {"max_size", max_size}},
           R"(uint64_t $sz$ = 0;
ok = deserialize_varint(data, pos, $max_size$, $sz$);
if (!ok) {
  return false;
}
$output$.resize($sz$);
if (pos + $sz$ > $max_size$) {
  return false;
}
std::memcpy($output$.data(), data+pos, $sz$);
pos += $sz$;
)");
}
void StringFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  format("case $1$: {\n", calculate_tag_str(d_));
  format.indent();
  generate_deserialization_only(p, value);
  format("break;\n");
  format.outdent();
  format("}\n");
}
RepeatedStringFieldGenerator::RepeatedStringFieldGenerator(
    const FieldDescriptor *field, const Options &options)
    : FieldGenerator(field, options) {}
std::string RepeatedStringFieldGenerator::cpp_type_name() const {
  return "std::vector<std::string>";
}
void RepeatedStringFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  format("for (const auto& s: $1$) {\n", value);
  format.indent();
  format("for (const auto& s: $1$) {\n", value);
  format.indent();
  format("total += $1$ + calculate_varint_size(s.size()) + s.size();\n",
         calculate_tag_size(d_));
  format.outdent();
  format("}\n");
  format.outdent();
  format("}\n");
}
void RepeatedStringFieldGenerator::generate_calculate_only(
    google::protobuf::io::Printer *p, const std::string &value,
    const std::string &output) const {
  p->Print({{"value", value},
            {"tag_sz", calculate_tag_size(d_)},
            {"output", output}},
           R"(
std::size_t $output$ = 0;
for (const auto& s: $value$) {
  $output$ += $tag_sz$ + calculate_varint_size(s.size()) + s.size();
}
)");
}
void RepeatedStringFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  if (can_ignore_default_value) {
    format("if (!$1$.empty()) {\n", value);
    format.indent();
    generate_serialization_only(p, value);
    format.outdent();
    format("}\n");
  }
  else {
    generate_serialization_only(p, value);
  }
}
void RepeatedStringFieldGenerator::generate_serialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  format("for(const auto& s: $1$) {\n", value);
  format.indent();
  StringFieldGenerator g(d_, options_);
  g.generate_serialization_only(p, "s");
  format.outdent();
  format("}\n");
}
void RepeatedStringFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  StringFieldGenerator g(d_, options_);
  Formatter format(p);
  format("case $1$: {\n", calculate_tag_str(d_));
  format.indent();
  format("std::string tmp_str;\n");
  g.generate_deserialization_only(p, "tmp_str");
  format("$1$.push_back(std::move(tmp_str));\n", value);
  format.outdent();
  format("}\n");
}
}  // namespace compiler
}  // namespace struct_pb