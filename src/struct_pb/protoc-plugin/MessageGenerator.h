#pragma once
#include "FieldGenerator.h"
#include "GeneratorBase.h"
namespace struct_pb {
namespace compiler {
class FileGenerator;
class MessageGenerator : public GeneratorBase {
 public:
  MessageGenerator(const google::protobuf::Descriptor *d, Options options)
      : GeneratorBase(options), d_(d), fg_map_(d, options) {}

  void generate(google::protobuf::io::Printer *p);
  void generate_struct_definition(google::protobuf::io::Printer *p);
  void generate_source(google::protobuf::io::Printer *p);

 private:
  void generate_get_needed_size(google::protobuf::io::Printer *p);
  void generate_serialize_to(google::protobuf::io::Printer *p);
  void generate_deserialize_to(google::protobuf::io::Printer *p);

 private:
  const google::protobuf::Descriptor *d_;
  std::vector<std::string> field_name_list;
  std::vector<std::size_t> unpack_index_list;
  std::vector<std::size_t> field_number_list;
  std::map<std::size_t, std::size_t> n2i_map;
  std::map<std::size_t, std::size_t> i2n_map;
  FieldGeneratorMap fg_map_;
  friend class FileGenerator;
};

}  // namespace compiler
}  // namespace struct_pb
