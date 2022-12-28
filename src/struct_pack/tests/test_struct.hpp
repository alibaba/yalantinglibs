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
  auto operator==(const person1 &rhs) const {
    return age == rhs.age && name == rhs.name && id == rhs.id && maybe == rhs.maybe;
  }
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

struct my_list {
  std::unique_ptr<my_list> next;
  std::string name;
  int value;
  friend bool operator==(const my_list &a, const my_list &b) {
    return a.value == b.value && a.name == b.name &&
           ((a.next == nullptr && b.next == nullptr) ||
            ((a.next != nullptr && b.next != nullptr) && *a.next == *b.next));
  }
};
struct my_list2 {
  struct my_list {
    std::unique_ptr<my_list2> next;
    std::string name;
    int value;
  };
  my_list list;
  friend bool operator==(const my_list2 &a, const my_list2 &b) {
    return a.list.value == b.list.value && a.list.name == b.list.name &&
           ((a.list.next == nullptr && b.list.next == nullptr) ||
            ((a.list.next != nullptr && b.list.next != nullptr) &&
             *a.list.next == *b.list.next));
  }
};

struct my_tree {
  std::unique_ptr<my_tree> left_child;
  std::unique_ptr<my_tree> right_child;
  std::string name;
  int value;
  friend bool operator==(const my_tree &a, const my_tree &b) {
    return a.value == b.value && a.name == b.name &&
           ((a.left_child == nullptr && b.left_child == nullptr) ||
            ((a.left_child != nullptr && b.left_child != nullptr) &&
             *a.left_child == *b.left_child)) &&
           ((a.right_child == nullptr && b.right_child == nullptr) ||
            ((a.right_child != nullptr && b.right_child != nullptr) &&
             *a.right_child == *b.right_child));
  }
};