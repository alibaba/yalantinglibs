#include "FileGenerator.h"

#include <google/protobuf/compiler/code_generator.h>

#include "EnumGenerator.h"
#include "MessageGenerator.h"
#include "helpers.hpp"
using namespace google::protobuf;
using namespace google::protobuf::compiler;
namespace struct_pb {
namespace compiler {

FileGenerator::FileGenerator(const google::protobuf::FileDescriptor *file,
                             Options options)
    : GeneratorBase(options), file_(file) {
  //  std::vector<const Descriptor*> msgs = flatten_messages_in_file(file);
  for (int i = 0; i < file_->message_type_count(); ++i) {
    message_generators_.push_back(
        std::make_unique<MessageGenerator>(file_->message_type(i), options));
  }
  for (int i = 0; i < file->enum_type_count(); ++i) {
    enum_generators_.push_back(
        std::make_unique<EnumGenerator>(file->enum_type(i), options));
  }
}

void FileGenerator::generate_enum_definitions(
    google::protobuf::io::Printer *p) {
  for (int i = 0; i < enum_generators_.size(); ++i) {
    enum_generators_[i]->generate_definition(p);
  }
}
void FileGenerator::generate_message_definitions(
    google::protobuf::io::Printer *p) {
  for (int i = 0; i < message_generators_.size(); ++i) {
    message_generators_[i]->generate_struct_definition(p);
  }
}
void FileGenerator::generate_shared_header_code(
    google::protobuf::io::Printer *p) {
  generate_fwd_decls(p);
  generate_enum_definitions(p);
  generate_message_definitions(p);
}
void FileGenerator::generate_header(google::protobuf::io::Printer *p) {
  p->Print(
      {{"filename", file_->name()}},
      R"(// Generated by the protocol buffer compiler (struct_pb).  DO NOT EDIT!
// source: $filename$

#pragma once

#include <limits>
#include <string>
#include <type_traits>
#include <memory>
#include <map>
#include <variant>
#include <vector>
#include <optional>
)");

  generate_dependency_includes(p);
  generate_ns_open(p);
  generate_shared_header_code(p);
  generate_ns_close(p);
  generate_message_struct_pb_func_definitions(p);
  p->Print("// clang-format on\n");
}

void FileGenerator::generate_fwd_decls(google::protobuf::io::Printer *p) {
  Formatter format(p);
  for (int i = 0; i < file_->message_type_count(); ++i) {
    auto m = file_->message_type(i);
    format("struct $1$;\n", resolve_keyword(m->name()));
  }
}
void FileGenerator::generate_dependency_includes(
    google::protobuf::io::Printer *p) {
  Formatter format(p);
  for (int i = 0; i < file_->dependency_count(); ++i) {
    auto dep = file_->dependency(i);
    std::string basename = StripProto(dep->name());
    std::string header_name = basename + ".struct_pb.h";
    format("#include \"$1$\"\n", header_name);
  }
}
void FileGenerator::generate_source(google::protobuf::io::Printer *p) {
  generate_message_struct_pb_func_source(p);
}
void FileGenerator::generate_message_struct_pb_func_definitions(
    google::protobuf::io::Printer *p) {
  std::vector<const Descriptor *> msgs = flatten_messages_in_file(file_);
  Formatter format(p);
  format("namespace struct_pb {\n");
  format("namespace internal {\n");
  for (auto msg : msgs) {
    auto name = qualified_class_name(msg, options_);
    format("// $1$\n", name);
    format(
        "std::size_t get_needed_size(const $1$&, const "
        "::struct_pb::UnknownFields& unknown_fields = {});\n",
        name);
    format(
        "void serialize_to(char*, std::size_t, const $1$&, const "
        "::struct_pb::UnknownFields& unknown_fields = {});\n",
        name);
    format(
        "bool deserialize_to($1$&, const char*, std::size_t, "
        "::struct_pb::UnknownFields& unknown_fields);\n",
        name);
    format("bool deserialize_to($1$&, const char*, std::size_t);\n", name);
    format("\n");
  }
  format("} // internal\n");
  format("} // struct_pb\n");
}
void FileGenerator::generate_message_struct_pb_func_source(
    google::protobuf::io::Printer *p) {
  std::vector<const Descriptor *> msgs = flatten_messages_in_file(file_);
  Formatter format(p);
  format("namespace struct_pb {\n");
  format("namespace internal {\n");
  for (auto msg : msgs) {
    auto name = qualified_class_name(msg, options_);
    MessageGenerator g(msg, options_);
    format("// $1$\n", name);
    format(
        "std::size_t get_needed_size(const $1$& t, const "
        "::struct_pb::UnknownFields& unknown_fields) {\n",
        name);
    format.indent();
    g.generate_get_needed_size(p);
    format.outdent();
    format(
        "} // std::size_t get_needed_size(const $1$& t, const "
        "::struct_pb::UnknownFields& unknown_fields)\n",
        name);

    format(
        "void serialize_to(char* data, std::size_t size, const $1$& t, "
        "const ::struct_pb::UnknownFields& unknown_fields) {\n",
        name);
    format.indent();
    g.generate_serialize_to(p);
    format.outdent();
    format(
        "} // void serialize_to(char* data, std::size_t size, const $1$& t, "
        "const ::struct_pb::UnknownFields& unknown_fields)\n",
        name);

    format(
        "bool deserialize_to($1$& t, const char* data, std::size_t size, "
        "::struct_pb::UnknownFields& unknown_fields) {\n",
        name);
    format.indent();
    g.generate_deserialize_to(p);
    format.outdent();
    format("return true;\n");
    format("} // bool deserialize_to($1$&, const char*, std::size_t)\n", name);
    format("// end of $1$\n", name);
    format(
        "bool deserialize_to($1$& t, const char* data, std::size_t size) {\n",
        name);
    format.indent();
    format("::struct_pb::UnknownFields unknown_fields{};\n");
    format("return deserialize_to(t,data,size,unknown_fields);\n");
    format.outdent();
    format("}\n");
    format("\n");
  }
  format("} // internal\n");
  format("} // struct_pb\n");
}
void FileGenerator::generate_ns_open(google::protobuf::io::Printer *p) {
  NamespaceOpener(p, options_.ns).open();
}
void FileGenerator::generate_ns_close(google::protobuf::io::Printer *p) {
  NamespaceOpener(p, options_.ns).close();
}

}  // namespace compiler

}  // namespace struct_pb