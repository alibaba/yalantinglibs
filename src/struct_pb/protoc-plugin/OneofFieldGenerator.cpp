#include "OneofFieldGenerator.h"

#include "EnumFieldGenerator.h"
#include "PrimitiveFieldGenerator.h"
#include "StringFieldGenerator.h"
#include "helpers.hpp"
namespace struct_pb {
namespace compiler {
void OneofFieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  format("if ($1$.index() == $2$) {\n", value, index());
  generate_calculate_size_only(p, value);
  format("\n}\n");
}
std::string OneofFieldGenerator::index() const {
  auto oneof = d_->containing_oneof();
  assert(oneof);
  int index = 0;
  for (int i = 0; i < oneof->field_count(); ++i) {
    if (oneof->field(i) == d_) {
      index = i + 1;
      break;
    }
  }
  assert(index > 0);
  return std::to_string(index);
}
void OneofFieldGenerator::generate_calculate_size_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  auto oneof_value = "std::get<" + index() + ">(" + value + ")";

  switch (d_->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      p->Print({{"tag_sz", calculate_tag_size(d_)},
                {"index", index()},
                {"oneof_value", oneof_value},
                {"value", value}},
               R"(
uint64_t sz = 0;
if ($oneof_value$) {
  sz = get_needed_size(*$oneof_value$);
}
total += $tag_sz$ + calculate_varint_size(sz) + sz;
)");
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      p->Print(
          {
              {"tag_sz", calculate_tag_size(d_)},
          },
          R"(
total += $tag_sz$ + calculate_varint_size()");
      StringFieldGenerator(d_, options_)
          .generate_calculate_size_only(p, oneof_value);
      p->Print(") + ");

      StringFieldGenerator(d_, options_)
          .generate_calculate_size_only(p, oneof_value);
      p->Print(";\n");
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      format("total += $1$ + ", calculate_tag_size(d_));
      EnumFieldGenerator(d_, options_)
          .generate_calculate_size_only(p, oneof_value);
      format(";\n");
      break;
    }
    default: {
      format("total += $1$ + ", calculate_tag_size(d_));
      PrimitiveFieldGenerator(d_, options_)
          .generate_calculate_size_only(p, oneof_value);
      format(";\n");
    }
  }
}
void OneofFieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  Formatter format(p);
  format("if ($1$.index() == $2$) {\n", value, index());
  generate_serialization_only(p, value);
  format("\n}\n");
}
void OneofFieldGenerator::generate_serialization_only(
    google::protobuf::io::Printer *p, const std::string &value) const {
  auto oneof_value = "std::get<" + index() + ">(" + value + ")";

  switch (d_->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      p->Print({{"value", value},
                {"tag", std::to_string(calculate_tag(d_))},
                {"index", index()},
                {"oneof_value", oneof_value}

               },
               R"(
serialize_varint(data, pos, size, $tag$);
if ($oneof_value$) {
  std::size_t sz = get_needed_size(*$oneof_value$);
  serialize_varint(data, pos, size, sz);
  serialize_to(data + pos, sz, *$oneof_value$);
  pos += sz;
}
else {
  serialize_varint(data, pos, size, 0);
}
)");
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      StringFieldGenerator(d_, options_)
          .generate_serialization_only(p, oneof_value);

      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      EnumFieldGenerator(d_, options_)
          .generate_serialization_only(p, oneof_value);
      break;
    }
    default: {
      p->Print({{"tag", calculate_tag_str(d_)}},
               R"(
serialize_varint(data, pos, size, $tag$);
)");
      PrimitiveFieldGenerator(d_, options_)
          .generate_serialization_only(p, oneof_value);
    }
  }
}
void OneofFieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p, const std::string &value) const {
  Formatter format(p);
  format("case $1$: {\n", calculate_tag_str(d_));
  generate_deserialization_only(p, value);
  format("break;\n");
  format("}\n");
}
void OneofFieldGenerator::generate_deserialization_only(
    google::protobuf::io::Printer *p, const std::string &value,
    const std::string &max_size) const {
  Formatter format(p);
  std::string oneof_value = "std::get<" + index() + ">(" + value + ")";

  switch (d_->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      p->Print(
          {{"value", value},
           {"type_name", qualified_class_name(d_->message_type(), options_)},
           {"tag", std::to_string(calculate_tag(d_))},
           {"max_size", max_size},
           {"oneof_value", oneof_value},
           {"index", index()}},

          R"(
if ($value$.index() != $index$) {
  $value$.emplace<$index$>(new $type_name$());
}
uint64_t msg_sz = 0;
ok = deserialize_varint(data, pos, $max_size$, msg_sz);
if (!ok) {
  return false;
}
ok = deserialize_to(*$oneof_value$, data + pos, msg_sz);
if (!ok) {
  return false;
}
pos += msg_sz;
)");
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      format("if ($1$.index() != $2$) {\n", value, index());
      format.indent();
      format("$1$.emplace<$2$>();\n", value, index());
      format.outdent();
      format("}\n");
      StringFieldGenerator(d_, options_)
          .generate_deserialization_only(p, oneof_value, "str_sz", max_size);
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      format("if ($1$.index() != $2$) {\n", value, index());
      format.indent();
      format("$1$.emplace<$2$>();\n", value, index());
      format.outdent();
      format("}\n");
      EnumFieldGenerator(d_, options_)
          .generate_deserialization_only(p, oneof_value, max_size);
      break;
    }
    default: {
      format("if ($1$.index() != $2$) {\n", value, index());
      format.indent();
      format("$1$.emplace<$2$>();\n", value, index());
      format.outdent();
      format("}\n");
      PrimitiveFieldGenerator(d_, options_)
          .generate_deserialization_only(p, oneof_value);
    }
  }
}
std::string OneofFieldGenerator::cpp_type_name() const {
  return "std::variant";
}
std::string OneofFieldGenerator::pb_type_name() const {
  return d_->type_name();
}
OneofFieldGenerator::OneofFieldGenerator(const FieldDescriptor *descriptor,
                                         const Options &options)
    : FieldGenerator(descriptor, options) {}

MessageOneofFieldGenerator::MessageOneofFieldGenerator(
    const FieldDescriptor *descriptor, const Options &options)
    : OneofFieldGenerator(descriptor, options) {}

void MessageOneofFieldGenerator::generate_setter_and_getter(
    google::protobuf::io::Printer *p) {
  auto oneof = d_->containing_oneof();
  assert(oneof);
  std::string type_name = get_type_name();
  type_name = qualified_class_name(d_->message_type(), options_);
  p->Print({{"name", resolve_keyword(d_->name())},
            {"type_name", type_name},
            {"field_name", resolve_keyword(oneof->name())},
            {"index", index()}},
           R"(
bool has_$name$() const {
  return $field_name$.index() == $index$;
}
void set_allocated_$name$($type_name$* p) {
  assert(p);
  $field_name$.emplace<$index$>(p);
}
const std::unique_ptr<$type_name$>& $name$() const {
  assert($field_name$.index() == $index$);
  return std::get<$index$>($field_name$);
}
)");
}
PrimitiveOneofFieldGenerator::PrimitiveOneofFieldGenerator(
    const FieldDescriptor *descriptor, const Options &options)
    : OneofFieldGenerator(descriptor, options) {}
void PrimitiveOneofFieldGenerator::generate_setter_and_getter(
    google::protobuf::io::Printer *p) {
  auto oneof = d_->containing_oneof();
  assert(oneof);
  std::string type_name = get_type_name();
  p->Print({{"name", resolve_keyword(d_->name())},
            {"type_name", type_name},
            {"field_name", resolve_keyword(oneof->name())},
            {"index", index()}},
           R"(
bool has_$name$() const {
  return $field_name$.index() == $index$;
}
void set_$name$($type_name$ $name$) {
  $field_name$.emplace<$index$>($name$);
}
$type_name$ $name$() const {
  assert($field_name$.index() == $index$);
  return std::get<$index$>($field_name$);
}
)");
}
StringOneofFieldGenerator::StringOneofFieldGenerator(
    const FieldDescriptor *descriptor, const Options &options)
    : OneofFieldGenerator(descriptor, options) {}
void StringOneofFieldGenerator::generate_setter_and_getter(
    google::protobuf::io::Printer *p) {
  auto oneof = d_->containing_oneof();
  assert(oneof);
  std::string type_name = get_type_name();
  p->Print({{"name", resolve_keyword(d_->name())},
            {"type_name", type_name},
            {"field_name", resolve_keyword(oneof->name())},
            {"index", index()}},
           R"(
bool has_$name$() const {
  return $field_name$.index() == $index$;
}
void set_$name$($type_name$ $name$) {
  $field_name$.emplace<$index$>(std::move($name$));
}
const $type_name$& $name$() const {
  assert($field_name$.index() == $index$);
  return std::get<$index$>($field_name$);
}
)");
}
EnumOneofFieldGenerator::EnumOneofFieldGenerator(
    const FieldDescriptor *descriptor, const Options &options)
    : OneofFieldGenerator(descriptor, options) {}
void EnumOneofFieldGenerator::generate_setter_and_getter(
    google::protobuf::io::Printer *p) {
  auto oneof = d_->containing_oneof();
  assert(oneof);
  std::string type_name = get_type_name();
  type_name = qualified_enum_name(d_->enum_type(), options_);
  p->Print({{"name", resolve_keyword(d_->name())},
            {"type_name", type_name},
            {"field_name", resolve_keyword(oneof->name())},
            {"index", index()}},
           R"(
bool has_$name$() const {
  return $field_name$.index() == $index$;
}
void set_allocated_$name$($type_name$ p) {
  $field_name$.emplace<$index$>(p);
}
$type_name$ $name$() const {
  assert($field_name$.index() == $index$);
  return std::get<$index$>($field_name$);
}
)");
}
}  // namespace compiler
}  // namespace struct_pb