#pragma once
#include <string>

#include "google/protobuf/descriptor.h"
struct Options {
  Options(const google::protobuf::FileDescriptor* f) : f(f) {}
  enum class name_style {
    default_keep_same_as_proto_file,
    camel_case,   // firstName, lastName
    snake_case,   // first_name, last_name
    kebab_case,   // first-name, last-name
    pascal_case,  // FirstName, LastName
  };
  name_style struct_name_style = name_style::default_keep_same_as_proto_file;
  bool generate_eq_op = false;
  std::string ns;
  const google::protobuf::FileDescriptor* f;
};