#include "StructGenerator.h"

#include <iostream>

#include "FileGenerator.h"
#include "Options.hpp"
#include "google/protobuf/descriptor.pb.h"
#include "helpers.hpp"
namespace struct_pb {
namespace compiler {
bool StructGenerator::Generate(
    const google::protobuf::FileDescriptor *file, const std::string &parameter,
    google::protobuf::compiler::GeneratorContext *generator_context,
    std::string *error) const {
  std::vector<std::pair<std::string, std::string>> options;
  google::protobuf::compiler::ParseGeneratorParameter(parameter, &options);
  Options struct_pb_options(file);
  for (const auto &option : options) {
    const auto &key = option.first;
    const auto &value = option.second;
    if (key == "generate_eq_op") {
      struct_pb_options.generate_eq_op = true;
    }
    else if (key == "namespace") {
      struct_pb_options.ns = value;
    }
    else {
      *error = "Unknown generator option: " + key;
      return false;
    }
  }
  if (struct_pb_options.ns.empty()) {
    struct_pb_options.ns = file->package();
  }
  auto basename = strip_proto(file->name());
  FileGenerator file_generator(file, struct_pb_options);
  // generate xxx.struct_pb.h
  {
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        generator_context->Open(basename + ".struct_pb.h"));
    google::protobuf::io::Printer p(output.get(), '$');
    p.Print({{"parameter", parameter}}, R"(// protoc generate parameter
// clang-format off
// $parameter$
// =========================
#pragma once

)");
    file_generator.generate_header(&p);
  }
  // generate xxx.struct_pb.cc
  {
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        generator_context->Open(basename + ".struct_pb.cc"));
    google::protobuf::io::Printer p(output.get(), '$');
    p.Print(
        {{"parameter", parameter}, {"header_file", basename + ".struct_pb.h"}},
        R"(// protoc generate parameter
// clang-format off
// $parameter$
// =========================
#include "$header_file$"
#include "struct_pb/struct_pb/struct_pb_impl.hpp"
)");
    Formatter format(&p);
    file_generator.generate_source(&p);
  }
  return true;
}
}  // namespace compiler
}  // namespace struct_pb