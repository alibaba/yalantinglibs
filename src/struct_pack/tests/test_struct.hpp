#include <array>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <util/expected.hpp>
#include <vector>

#include "struct_pack/struct_pack.hpp"
struct person {
  int age;
  std::string name;
  auto operator==(const person &rhs) const {
    return age == rhs.age && name == rhs.name;
  }
  bool operator<(person const &p) const {
    return age < p.age || (age == p.age && name < p.name);
  }
};

struct person1 {
  int age;
  std::string name;
  struct_pack::compatible<int32_t> id;
  struct_pack::compatible<bool> maybe;
};

struct empty {};

enum class Color { red, black, white };

struct complicated_object {
  Color color;
  int a;
  std::string b;
  std::vector<person> c;
  std::list<std::string> d;
  std::deque<int> e;
  std::map<int, person> f;
  std::multimap<int, person> g;
  std::set<std::string> h;
  std::multiset<int> i;
  std::unordered_map<int, person> j;
  std::unordered_multimap<int, int> k;
  std::array<person, 2> m;
  person n[2];
  std::pair<std::string, person> o;
  bool operator==(const complicated_object &o) const {
    return color == o.color && a == o.a && b == o.b && c == o.c && d == o.d &&
           e == o.e && f == o.f && g == o.g && h == o.h && i == o.i &&
           j == o.j && k == o.k && m == o.m && n[0] == o.n[0] &&
           n[1] == o.n[1] && this->o == o.o;
  }
};

struct nested_object {
  int id;
  std::string name;
  person p;
  complicated_object o;
  bool operator==(const nested_object &o) const = default;
};