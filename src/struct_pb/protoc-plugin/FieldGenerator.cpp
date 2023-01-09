#include "FieldGenerator.h"

#include "EnumFieldGenerator.h"
#include "MapFieldGenerator.h"
#include "MessageFieldGenerator.h"
#include "OneofFieldGenerator.h"
#include "PrimitiveFieldGenerator.h"
#include "StringFieldGenerator.h"
#include "google/protobuf/io/printer.h"
#include "helpers.hpp"

using namespace google::protobuf;
namespace struct_pb {
namespace compiler {

bool is_varint(const FieldDescriptor *f) {
  switch (f->type()) {
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
      return true;
    default:
      return false;
  }
}
bool is_sint(const FieldDescriptor *f) {
  switch (f->type()) {
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
      return true;
    default:
      return false;
  }
}
bool is_sint32(const FieldDescriptor *f) {
  switch (f->type()) {
    case FieldDescriptor::TYPE_SINT32:
      return true;
    default:
      return false;
  }
}
bool is_enum(const FieldDescriptor *f) {
  return f->type() == FieldDescriptor::TYPE_ENUM;
}
bool is_bool(const FieldDescriptor *f) {
  return f->type() == FieldDescriptor::TYPE_BOOL;
}
bool is_i64(const FieldDescriptor *f) {
  switch (f->type()) {
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_DOUBLE:
      return true;
    default:
      return false;
  }
}
bool is_i32(const FieldDescriptor *f) {
  switch (f->type()) {
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FLOAT:
      return true;
    default:
      return false;
  }
}
bool is_string(const FieldDescriptor *f) {
  switch (f->type()) {
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return true;
    default:
      return false;
  }
}
bool is_message(const FieldDescriptor *f) {
  return f->type() == FieldDescriptor::TYPE_MESSAGE;
}

bool is_group(const FieldDescriptor *f) {
  return f->type() == FieldDescriptor::TYPE_GROUP;
}

bool is_number(const FieldDescriptor *f) {
  switch (f->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
      return true;
    default:
      return false;
  }
}

uint32_t calculate_tag(const FieldDescriptor *f, bool ignore_repeated) {
  uint32_t number = f->number();
  uint32_t wire_type;
  if (f->is_repeated() && !ignore_repeated) {
    wire_type = 2;
  }
  else if (is_varint(f)) {
    wire_type = 0;
  }
  else if (is_i64(f)) {
    wire_type = 1;
  }
  else if (is_i32(f)) {
    wire_type = 5;
  }
  else {
    wire_type = 2;
  }
  return (number << 3) | wire_type;
}

std::string calculate_tag_str(const FieldDescriptor *f, bool ignore_repeated) {
  return std::to_string(calculate_tag(f, ignore_repeated));
}

std::size_t calculate_varint_size(uint64_t t) {
  std::size_t ret = 0;
  do {
    ret++;
    t >>= 7;
  } while (t != 0);
  return ret;
}

std::string calculate_tag_size(const FieldDescriptor *f, bool ignore_repeated) {
  auto tag = calculate_tag(f, ignore_repeated);
  auto tag_size = calculate_varint_size(tag);
  return std::to_string(tag_size);
}

std::string get_comment(const FieldDescriptor *d_) {
  std::string comment;
  if (d_->has_optional_keyword()) {
    comment += "optional ";
  }
  if (d_->is_repeated()) {
    comment += "repeated ";
  }
  if (is_message(d_)) {
    comment += class_name(d_->message_type());
  }
  else {
    comment += d_->type_name();
  }
  comment += " ";
  comment += d_->name();
  comment += " = ";
  comment += std::to_string(d_->number());
  comment += ", tag = ";
  comment += std::to_string(calculate_tag(d_));
  return comment;
}

void FieldGenerator::generate_definition(
    google::protobuf::io::Printer *p) const {
  p->Print(
      {
          {"type", cpp_type_name()},
          {"name", name()},
          {"pb_type", pb_type_name()},
          {"number", std::to_string(d_->number())},
      },
      R"($type$ $name$; // $pb_type$, field number = $number$
)");
}

std::string FieldGenerator::get_type_name() const {
  switch (d_->type()) {
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
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
      auto m = d_->message_type();
      if (d_->is_map()) {
        auto key = m->field(0);
        auto value = m->field(1);
        std::string type_name = "std::map<";
        type_name += FieldGenerator(key, options_).get_type_name();
        type_name += ", ";
        type_name += FieldGenerator(value, options_).get_type_name();
        type_name += ">";
        return type_name;
      }
      else if (d_->is_repeated()) {
        if (is_message(d_)) {
          return qualified_class_name(d_->message_type(), options_);
        }
        else {
          return m->name();
        }
      }
      else {
        auto p = d_->containing_type();
        if (is_map_entry(p)) {
          return m->name();
        }
        else {
          return "std::unique_ptr<" +
                 qualified_class_name(d_->message_type(), options_) + ">";
        }
      }
    }
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
      return d_->enum_type()->name();
    case google::protobuf::FieldDescriptor::TYPE_GROUP:
      // workaround for group
      return "std::string";
    default: {
      return "not support now";
    }
  }
}

void FieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  //  auto name = value.empty() ? "t." + this->name() : value;
  //  auto tag = calculate_tag(d_);
  //  auto v =
  //      p->WithVars({{"name", name}, {"tag_size",
  //      calculate_varint_size(tag)}});
  //  std::string comment = get_comment(d_);
  //  p->Print({{"comment", comment}}, R"(
  //// $comment$
  //)");
  //  if (d_->has_optional_keyword() && !is_message(d_)) {
  //    p->Print(R"(
  // if ($name$.has_value()) {
  //)");
  //    generate_calculate_one(p, name + ".value()", false);
  //    p->Print("}\n");
  //  } else if (d_->is_repeated()) {
  //    generate_calculate_vector(p);
  //  } else {
  //    generate_calculate_one(p, name, can_ignore_default_value);
  //  }
}

void FieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, const std::string &value,
    bool can_ignore_default_value) const {
  //  auto name = value.empty() ? "t." + this->name() : value;
  //  auto tag = calculate_tag(d_);
  //  auto v =
  //      p->WithVars({{"name", name}, {"tag_size",
  //      calculate_varint_size(tag)}});
  //  auto comment = get_comment(d_);
  //  p->Print({{"comment", comment}}, R"(
  //// $comment$
  //)");
  //  if (is_optional()) {
  //    p->Print(R"(
  // if ($name$.has_value()) {
  //)");
  //    generate_serialize_one(p, name + ".value()", false);
  //    p->Print("}\n");
  //  } else if (d_->is_repeated()) {
  //    generate_serialize_vector(p);
  //  } else {
  //    generate_serialize_one(p, name, can_ignore_default_value);
  //  }
}
void FieldGenerator::generate_deserialization(google::protobuf::io::Printer *p,
                                              const std::string &value) const {}
bool FieldGenerator::is_ptr() const {
  auto p = d_->containing_type();
  if (p && is_map_entry(p)) {
    return false;
  }
  return true;
}
std::string FieldGenerator::name() const { return resolve_keyword(d_->name()); }
bool FieldGenerator::is_optional() const {
  auto p = d_->containing_type();
  if (p && is_map_entry(p)) {
    return false;
  }
  //  return d_->is_optional();
  //  return d_->has_optional_keyword() && !is_message(d_);
  return d_->has_presence();
}
std::string FieldGenerator::qualified_name() const {
  if (is_message(d_)) {
    return qualified_class_name(d_->message_type(), options_);
  }
  else if (is_enum(d_)) {
    return qualified_enum_name(d_->enum_type(), options_);
  }
  else {
    return get_type_name();
  }
}
std::string FieldGenerator::pb_type_name() const { return d_->type_name(); }
void FieldGenerator::generate_calculate_size(
    google::protobuf::io::Printer *p, bool can_ignore_default_value) const {
  auto oneof = d_->real_containing_oneof();
  if (oneof) {
    generate_calculate_size(p, "t." + resolve_keyword(oneof->name()),
                            can_ignore_default_value);
  }
  else {
    generate_calculate_size(p, "t." + name(), can_ignore_default_value);
  }
}
void FieldGenerator::generate_serialization(
    google::protobuf::io::Printer *p, bool can_ignore_default_value) const {
  auto oneof = d_->real_containing_oneof();
  if (oneof) {
    generate_serialization(p, "t." + resolve_keyword(oneof->name()),
                           can_ignore_default_value);
  }
  else {
    Formatter format(p);
    format("// ");
    generate_definition(p);
    generate_serialization(p, "t." + name(), can_ignore_default_value);
  }
}
void FieldGenerator::generate_deserialization(
    google::protobuf::io::Printer *p) const {
  auto oneof = d_->real_containing_oneof();
  if (oneof) {
    generate_deserialization(p, "t." + resolve_keyword(oneof->name()));
  }
  else {
    Formatter format(p);
    format("// ");
    generate_definition(p);
    generate_deserialization(p, "t." + name());
  }
}
std::string FieldGenerator::cpp_type_name() const {
  assert(false && "why?");
  return "???";
}
void OneofGenerator::generate_definition(google::protobuf::io::Printer *p) {
  p->Print(R"(std::variant<std::monostate
)");
  for (int i = 0; i < d_->field_count(); ++i) {
    auto f = d_->field(i);
    FieldGenerator fg(f, options_);
    std::string type_name = fg.get_type_name();
    if (is_message(f)) {
      type_name = qualified_class_name(f->message_type(), options_);
      type_name = "std::unique_ptr<" + type_name + ">";
    }
    else if (is_enum(f)) {
      type_name = qualified_enum_name(f->enum_type(), options_);
    }
    p->Print(
        {
            {"type", type_name},
            {"name", f->name()},
            {"pb_type", f->type_name()},
            {"number", std::to_string(f->number())},
        },
        R"(, $type$ // $pb_type$, field number = $number$
)");
  }
  p->Print({{"name", name()}}, R"(> $name$;
)");
  for (int i = 0; i < d_->field_count(); ++i) {
    auto f = d_->field(i);
    if (is_message(f)) {
      MessageOneofFieldGenerator(f, options_).generate_setter_and_getter(p);
    }
    else if (is_string(f)) {
      StringOneofFieldGenerator(f, options_).generate_setter_and_getter(p);
    }
    else if (is_enum(f)) {
      EnumOneofFieldGenerator(f, options_).generate_setter_and_getter(p);
    }
    else {
      PrimitiveOneofFieldGenerator(f, options_).generate_setter_and_getter(p);
    }
  }
}
void OneofGenerator::generate_calculate_size(google::protobuf::io::Printer *p) {
  Formatter format(p);
  format("{\n");
  format.indent();
  format("switch (t.$1$.index()) {\n", name());
  p->Print(R"(
case 0: {
  // empty, do nothing
  break;
}
)");
  for (int i = 0; i < d_->field_count(); ++i) {
    auto f = d_->field(i);
    format("case $1$: {\n", i + 1);
    format.indent();
    std::string value =
        "std::get<" + std::to_string(i + 1) + ">(t." + name() + ")";
    FieldGenerator(f, options_).generate_calculate_size(p, value, false);
    format("break;\n");
    format.outdent();
    format("}\n");
  }
  format("}\n");
  format.outdent();
  format("}\n");
}
void OneofGenerator::generate_serialization(
    google::protobuf::io::Printer *p,
    const google::protobuf::FieldDescriptor *f) {
  //  auto index = get_index(f);
  //  assert(index != 0);
  //  auto v = p->WithVars({{"name", "t." + name()}, {"index", index}});
  //  Formatter format(p);
  //  format("if ($name$.index() == $index$) {\n");
  //  format.indent();
  //  std::string name = "std::get<";
  //  name += std::to_string(index);
  //  name += ">";
  //  name += "(";
  //  name += "t." + this->name();
  //  name += ")";
  //  FieldGenerator(f, options_).generate_serialization(p, name, false);
  //  format.outdent();
  //  format("}\n");
}
void OneofGenerator::generate_deserialization(
    google::protobuf::io::Printer *p,
    const google::protobuf::FieldDescriptor *f) {
  //  auto index = get_index(f);
  //  assert(index != 0);
  //  auto v = p->WithVars({{"name", "t." + name()}, {"index", index}});
  //  Formatter format(p);
  //  FieldGenerator fg(f, options_);
  //  std::string end =
  //      "t." + name() + ".emplace<" + std::to_string(index) + ">(tmp);";
  //  if (is_message(f)) {
  //    auto type_name = qualified_class_name(f->message_type(), options_);
  //    fg.generate_deserialization(
  //        p, "tmp", type_name + "* tmp = new " + type_name + "();", end);
  //  } else if (is_enum(f)) {
  //    auto type_name = qualified_enum_name(f->enum_type(), options_);
  //    fg.generate_deserialization(p, "tmp", type_name + " tmp{};", end);
  //  } else {
  //    fg.generate_deserialization(p, "tmp", fg.get_type_name() + " tmp{};",
  //    end);
  //  }
}
int OneofGenerator::get_index(
    const google::protobuf::FieldDescriptor *f) const {
  auto index = 0;
  for (int i = 0; i < d_->field_count(); ++i) {
    auto fd = d_->field(i);
    if (fd == f) {
      index = i + 1;
    }
  }
  return index;
}
std::string OneofGenerator::name() const { return resolve_keyword(d_->name()); }
FieldGenerator *FieldGeneratorMap::MakeGenerator(const FieldDescriptor *field,
                                                 const Options &options) {
  if (field->is_repeated()) {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        if (field->is_map()) {
          return new MapFieldGenerator(field, options);
        }
        else {
          return new RepeatedMessageFieldGenerator(field, options);
        }
      case FieldDescriptor::CPPTYPE_STRING:
        return new RepeatedStringFieldGenerator(field, options);
      case FieldDescriptor::CPPTYPE_ENUM:
        return new RepeatedEnumFieldGenerator(field, options);
      default:
        return new RepeatedPrimitiveFieldGenerator(field, options);
    }
  }
  else if (field->real_containing_oneof()) {
    return new OneofFieldGenerator(field, options);
    //    switch (field->cpp_type()) {
    //    case FieldDescriptor::CPPTYPE_MESSAGE:
    //      return new MessageOneofFieldGenerator(field, options, scc_analyzer);
    //    case FieldDescriptor::CPPTYPE_STRING:
    //      return new StringOneofFieldGenerator(field, options);
    //    case FieldDescriptor::CPPTYPE_ENUM:
    //      return new EnumOneofFieldGenerator(field, options);
    //    default:
    //      return new PrimitiveOneofFieldGenerator(field, options);
    //    }
  }
  else {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        return new MessageFieldGenerator(field, options);
      case FieldDescriptor::CPPTYPE_STRING:
        return new StringFieldGenerator(field, options);
      case FieldDescriptor::CPPTYPE_ENUM:
        return new EnumFieldGenerator(field, options);
      default:
        return new PrimitiveFieldGenerator(field, options);
    }
  }
}
FieldGeneratorMap::FieldGeneratorMap(const Descriptor *descriptor,
                                     const Options &options)
    : descriptor_(descriptor), field_generators_(descriptor->field_count()) {
  // Construct all the FieldGenerators.
  for (int i = 0; i < descriptor->field_count(); i++) {
    field_generators_[i].reset(MakeGenerator(descriptor->field(i), options));
  }
}
const FieldGenerator &FieldGeneratorMap::get(
    const FieldDescriptor *field) const {
  return *field_generators_[field->index()];
}
}  // namespace compiler
}  // namespace struct_pb
