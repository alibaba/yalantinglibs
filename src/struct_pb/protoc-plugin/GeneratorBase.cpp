#include "GeneratorBase.h"

#include <iostream>
namespace struct_pb {
namespace compiler {

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
}  // namespace compiler
}  // namespace struct_pb