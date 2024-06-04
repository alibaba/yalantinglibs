#include <iostream>
#include <string>
using namespace std;
string i1 = "else if constexpr (Count == ";
string j = " ){ return visitor(";
string l = ");}";
int main() {
  std::string list = "_SPG0(o)";
  for (int i = 2; i <= 256; ++i) {
    list += ",_SPG" + to_string(i - 1) + "(o)";
    cout << i1 + to_string(i) + j + list + l << endl;
  }
  return 0;
}
