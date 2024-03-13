#include <cassert>
#include <iostream>
#include <ylt/version.hpp>

int main() {
  std::cout << "FULL_BUILD_VERSION : " << YLT_FULL_BUILD_VERSION
            << ", VERSION : " << YLT_VERSION
            << ", NUM VERSION : " << YLT_NUM_VERSION << std::endl;
  assert(std::string(YLT_VERSION) == "0.3.0");
  assert(YLT_NUM_VERSION % 100 == 0);
  assert(YLT_NUM_VERSION / 100 % 1000 == 3);
  assert(YLT_NUM_VERSION / 1e5 == 0);
}