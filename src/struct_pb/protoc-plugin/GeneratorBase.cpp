#include "GeneratorBase.h"

#include <iostream>
namespace struct_pb {
namespace compiler {
std::string GeneratorBase::get_type(google::protobuf::FieldDescriptor::Type t,
                                    const std::string &name) {
  switch (t) {
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
      return "double";
    case google::protobuf::FieldDescriptor::TYPE_FLOAT:
      return "float";
    case google::protobuf::FieldDescriptor::TYPE_INT64:
      return "struct_pack::pb::varint64_t";
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
      return "struct_pack::pb::varuint64_t";
    case google::protobuf::FieldDescriptor::TYPE_INT32:
      return "struct_pack::pb::varint32_t";
    case google::protobuf::FieldDescriptor::TYPE_FIXED64:
      return "uint64_t";
    case google::protobuf::FieldDescriptor::TYPE_FIXED32:
      return "uint32_t";
    case google::protobuf::FieldDescriptor::TYPE_BOOL:
      return "bool";
    case google::protobuf::FieldDescriptor::TYPE_STRING:
      return "std::string";
    case google::protobuf::FieldDescriptor::TYPE_GROUP:
      break;
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
      return get_prefer_struct_name(name);
    case google::protobuf::FieldDescriptor::TYPE_BYTES:
      return "std::string";
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
      return "struct_pack::pb::varuint32_t";
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
      return get_prefer_struct_name(name);
    case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
      return "int32_t";
    case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
      return "int64_t";
    case google::protobuf::FieldDescriptor::TYPE_SINT32:
      return "struct_pack::pb::sint32_t";
    case google::protobuf::FieldDescriptor::TYPE_SINT64:
      return "struct_pack::pb::sint64_t";
  }
  return "not support";
}

// Convert lowerCamelCase and UpperCamelCase strings to lower_with_underscore.
std::string convert(const std::string &camelCase) {
  std::string str(1, tolower(camelCase[0]));

  // First place underscores between contiguous lower and upper case letters.
  // For example, `_LowerCamelCase` becomes `_Lower_Camel_Case`.
  for (auto it = camelCase.begin() + 1; it != camelCase.end(); ++it) {
    if (isupper(*it) && *(it - 1) != '_' && islower(*(it - 1))) {
      str += "_";
    }
    str += *it;
  }

  // Then convert it to lower case.
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);

  return str;
}
std::string GeneratorBase::get_prefer_struct_name(
    const std::string &name) const {
  if (options_.struct_name_style == Options::name_style::snake_case) {
    return convert(name);
  }
  return name;
}
std::string GeneratorBase::get_type_str(
    const google::protobuf::FieldDescriptor *d) {
  if (d->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
    return get_prefer_struct_name(d->message_type()->name());
  }
  else if (d->type() == google::protobuf::FieldDescriptor::TYPE_ENUM) {
    return get_prefer_struct_name(d->enum_type()->name());
  }
  else {
    return get_type(d->type());
  }
}
std::string GeneratorBase::get_field_type_str(
    const google::protobuf::FieldDescriptor *d) {
  auto name = get_type_str(d);
  if (d->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
    name = "std::unique_ptr<" + name + ">";
  }
  return name;
}
std::string GeneratorBase::get_prefer_field_name(
    const std::string &name) const {
  std::set<std::string> keyword_set{"inline", "void", "concept", "requires",
                                    "return", "int",  "double",  "float"};
  auto it = keyword_set.find(name);
  if (it == keyword_set.end()) {
    return name;
  }
  std::cout << "field name in keywords find: " << name;
  std::cout << ", change to _" << name << "_";
  std::cout << std::endl;
  return "_" + name + "_";
}
std::string GeneratorBase::add_modifier(
    const google::protobuf::FieldDescriptor *f, std::string type_name) const {
  if (f->is_repeated()) {
    type_name = "std::vector<" + type_name + ">";
  }
  if (f->has_optional_keyword()) {
    type_name = "std::optional<" + type_name + ">";
  }
  return type_name;
}
}  // namespace compiler
}  // namespace struct_pb