#include "EnumGenerator.h"

#include "helpers.hpp"
namespace struct_pb {
namespace compiler {
void EnumGenerator::generate(google::protobuf::io::Printer *p) {
  std::string enum_name = d_->name();
  //  enum_name = get_prefer_struct_name(enum_name);
  p->Print({{"enum_name", enum_name}}, R"(
enum class $enum_name$ {
)");
  for (int j = 0; j < d_->value_count(); ++j) {
    auto v = d_->value(j);
    p->Print({{"name", resolve_keyword(v->name())},
              {"value", std::to_string(v->number())}},
             R"(
$name$ = $value$,
)");
  }
  p->Print(
      R"(
};
)");
}
void EnumGenerator::generate_definition(google::protobuf::io::Printer *p) {
  Formatter format(p);
  format("enum class $1$: int {\n", d_->name());
  format.indent();
  for (int i = 0; i < d_->value_count(); ++i) {
    auto value = resolve_keyword(d_->value(i)->name());
    auto number = d_->value(i)->number();
    format("$1$ = $2$,\n", value, number);
  }
  format.outdent();
  format("};\n");
}
}  // namespace compiler
}  // namespace struct_pb
