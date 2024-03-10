#include <cassert>
#include <iostream>
#include <ylt/ylt.hpp>

int main() {
  std::cout << "FULL_BUILD_VERSION : " << YLT_FULL_BUILD_VERSION
            << ", VERSION : " << YLT_VERSION << std::endl;
  assert(std::string(YLT_VERSION) == "0.3.0");
}