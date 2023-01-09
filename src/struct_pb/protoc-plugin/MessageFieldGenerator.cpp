#include "MessageFieldGenerator.h"

namespace struct_pb {
namespace compiler {
MessageFieldGenerator::MessageFieldGenerator(const FieldDescriptor *field,
                                             const Options &options)
    : FieldGenerator(field, options) {}
void MessageFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  if (can_ignore_default_value) {
    p->Print({{"tag_sz", calculate_tag_size(d_)}, {"value", value}}, R"(
if ($value$) {
  auto sz = get_needed_size(*$value$);
  total += $tag_sz$ + calculate_varint_size(sz) + sz;
}
)");
  }
  else {
    p->Print({{"tag_sz", calculate_tag_size(d_)}, {"value", value}}, R"(
auto sz = get_needed_size(*$value$);
total += $tag_sz$ + calculate_varint_size(sz) + sz;
)");
  }
}
void MessageFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  p->Print({{"value", value}, {"tag", calculate_tag_str(d_)}}, R"(
if ($value$) {
  serialize_varint(data, pos, size, $tag$);
  auto sz = get_needed_size(*$value$);
  serialize_varint(data, pos, size, sz);
  serialize_to(data + pos, sz, *$value$);
  pos += sz;
}
)");
}
void MessageFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  p->Print({{"value", value},
            {"tag", calculate_tag_str(d_)},
            {"classname", qualified_class_name(d_->message_type(), options_)}},
           R"(case $tag$: {
  if (!$value$) {
    $value$ = std::make_unique<$classname$>();
  }
  uint64_t sz = 0;
  ok = deserialize_varint(data, pos, size, sz);
  if (!ok) {
    return false;
  }
  ok = deserialize_to(*$value$, data + pos, sz);
  if (!ok) {
    return false;
  }
  pos += sz;
  break;
}
)");
}
std::string MessageFieldGenerator::cpp_type_name() const {
  return "std::unique_ptr<" +
         qualified_class_name(d_->message_type(), options_) + ">";
}

RepeatedMessageFieldGenerator::RepeatedMessageFieldGenerator(
    const FieldDescriptor *field, const Options &options)
    : FieldGenerator(field, options) {}
void RepeatedMessageFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  if (can_ignore_default_value) {
    format("if (!$1$.empty()) {\n", value);
    format.indent();
    generate_calculate_size_only(p, value);
    format.outdent();
    format("}\n");
  }
  else {
    generate_calculate_size_only(p, value);
  }
}
void RepeatedMessageFieldGenerator::generate_serialization(
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
void RepeatedMessageFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  p->Print({{"tag", std::to_string(calculate_tag(d_))},
            {"value", value},
            {"classname", qualified_class_name(d_->message_type(), options_)}},
           R"(case $tag$: {
  uint64_t sz = 0;
  ok = deserialize_varint(data, pos, size, sz);
  if (!ok) {
    return false;
  }
  $value$.emplace_back();
  ok = deserialize_to($value$.back(), data + pos, sz);
  if (!ok) {
    $value$.pop_back();
    return false;
  }
  pos += sz;
  break;
}
)");
}
std::string RepeatedMessageFieldGenerator::cpp_type_name() const {
  return "std::vector<" + qualified_class_name(d_->message_type(), options_) +
         ">";
}
void RepeatedMessageFieldGenerator::generate_calculate_size_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  p->Print({{"tag_sz", calculate_tag_size(d_)}, {"value", value}}, R"(
for(const auto& e: $value$) {
  std::size_t sz = get_needed_size(e);
  total += $tag_sz$ + calculate_varint_size(sz) + sz;
}
)");
}
void RepeatedMessageFieldGenerator::generate_serialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  p->Print({{"tag", std::to_string(calculate_tag(d_))}, {"value", value}}, R"(
for(const auto& e: $value$) {
  serialize_varint(data, pos, size, $tag$);
  std::size_t sz = get_needed_size(e);
  serialize_varint(data, pos, size, sz);
  serialize_to(data+pos, sz, e);
  pos += sz;
}
)");
}

}  // namespace compiler
}  // namespace struct_pb