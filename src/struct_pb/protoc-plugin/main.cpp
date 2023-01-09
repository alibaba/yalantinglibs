#include <google/protobuf/compiler/plugin.h>

#include "StructGenerator.h"
using namespace struct_pb::compiler;
using namespace google::protobuf::compiler;
int main(int argc, char* argv[]) {
  StructGenerator generator;
  return PluginMain(argc, argv, &generator);
}