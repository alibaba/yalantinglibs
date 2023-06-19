#include <cassert>
#include <iostream>

#include "struct_yaml/yaml_reader.h"
#include "struct_yaml/yaml_writer.h"

struct address_t {
  std::string_view street;
  std::string_view city;
  std::string_view state;
  std::string_view country;
};
REFLECTION(address_t, street, city, state, country);
struct contact_t {
  std::string_view type;
  std::string_view value;
};
REFLECTION(contact_t, type, value);
struct person_t {
  std::string_view name;
  int age;
  address_t address;
  std::vector<contact_t> contacts;
};
REFLECTION(person_t, name, age, address, contacts);

std::ostream &operator<<(std::ostream &os, person_t p) {
  os << "name: " << p.name << "\tage: " << p.age << std::endl;
  os << p.address.street << "\n";
  os << p.address.city << "\n";
  os << p.address.state << "\n";
  os << p.address.country << "\n";
  os << p.contacts[0].type << " : " << p.contacts[0].value << "\n";
  os << p.contacts[1].type << " : " << p.contacts[1].value << "\n";
  os << p.contacts[2].type << " : " << p.contacts[2].value << "\n";
  return os;
}

void person_example() {
  std::string str = R"(
name: John Doe
age: 30
address:
  street: 123 Main St
  city: Anytown
  state: Example State
  country: Example Country
contacts:
  - type: email
    value: john@example.com
  - type: phone
    value: 123456789
  - type: social
    value: "johndoe"
  )";
  person_t p;
  iguana::from_yaml(p, str);
  std::cout << "========= deserialize person_t ========\n";
  std::cout << p;
  std::string ss;
  iguana::to_yaml(p, ss);
  std::cout << "========== serialize person_t =========\n";
  std::cout << ss;
  person_t p2;
  iguana::from_yaml(p2, ss);
}

int main() {
  person_example();
}