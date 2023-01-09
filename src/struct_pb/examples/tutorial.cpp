#include <fstream>
#include <iostream>
#include <string>

#include "addressbook.struct_pb.h"
void prompt_for_address(tutorial::Person& person) {
  std::cout << "==================================" << std::endl;
  std::cout << "            Add People            " << std::endl;
  std::cout << "==================================" << std::endl;
  std::cout << "Enter person ID number: ";
  std::cin >> person.id;
  std::cin.ignore(256, '\n');
  std::cout << "Enter name: ";
  std::getline(std::cin, person.name);
  std::cout << "Enter email address (blank for none): ";
  std::getline(std::cin, person.email);
  while (true) {
    std::cout << "Enter a phone number (or leave blank to finish): ";
    tutorial::Person::PhoneNumber phone_number;
    std::getline(std::cin, phone_number.number);
    if (phone_number.number.empty()) {
      break;
    }
    std::cout << "Is this a mobile, home, or work phone? ";
    std::string type;
    std::getline(std::cin, type);
    if (type == "mobile") {
      phone_number.type = tutorial::Person::PhoneType::MOBILE;
    }
    else if (type == "home") {
      phone_number.type = tutorial::Person::PhoneType::HOME;
    }
    else if (type == "work") {
      phone_number.type = tutorial::Person::PhoneType::WORK;
    }
    else {
      std::cout << "Unknown phone type: Using default." << std::endl;
    }
    person.phones.push_back(phone_number);
  }
}
void list_people(const tutorial::AddressBook& address_book) {
  std::cout << "==================================" << std::endl;
  std::cout << "          List People             " << std::endl;
  std::cout << "==================================" << std::endl;
  for (const auto& person : address_book.people) {
    std::cout << "     Person ID: " << person.id << std::endl;
    std::cout << "          Name: " << person.name << std::endl;
    if (!person.email.empty()) {
      std::cout << "E-mail address: " << person.email << std::endl;
    }
    for (const auto& phone : person.phones) {
      switch (phone.type) {
        case tutorial::Person::PhoneType::MOBILE:
          std::cout << "Mobile phone #: ";
          break;
        case tutorial::Person::PhoneType::HOME:
          std::cout << "  Home phone #: ";
          break;
        case tutorial::Person::PhoneType::WORK:
          std::cout << "  Work phone #: ";
          break;
      }
      std::cout << phone.number << std::endl;
    }
  }
}
int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " ADDRESS_BOOK_FILE" << std::endl;
    return -1;
  }
  tutorial::AddressBook address_book;
  std::fstream input(argv[1], std::ios::in | std::ios::binary);
  if (!input) {
    std::cout << argv[1] << ": File not found. Creating a new file."
              << std::endl;
  }
  else {
    input.seekg(0, input.end);
    int length = input.tellg();
    input.seekg(0, input.beg);
    std::string buffer;
    buffer.resize(length);
    input.read(buffer.data(), buffer.size());
    input.close();
    bool ok = struct_pb::internal::deserialize_to(address_book, buffer.data(),
                                                  buffer.size());
    if (!ok) {
      std::cerr << "Failed to parse address book." << std::endl;
      return -1;
    }
  }
  list_people(address_book);
  address_book.people.emplace_back();
  prompt_for_address(address_book.people.back());
  std::fstream output(argv[1],
                      std::ios::out | std::ios::trunc | std::ios::binary);
  std::string buffer;
  auto length = struct_pb::internal::get_needed_size(address_book);
  buffer.resize(length);
  struct_pb::internal::serialize_to(buffer.data(), buffer.size(), address_book);
  output.write(buffer.data(), buffer.size());
  output.close();
  list_people(address_book);
  std::cout << "Done!!!" << std::endl;
  return 0;
}