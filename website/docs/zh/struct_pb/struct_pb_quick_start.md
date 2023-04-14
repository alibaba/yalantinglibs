# struct_pb Quick Start

Note: if you are not familiar with the official c++ implement, 
[Protocol Buffer Basics: C++](https://developers.google.com/protocol-buffers/docs/cpptutorial)
is a good material to learn.

This tutorial provides a basic C++ programmer's introduction to work with struct_pb, 
which is the protobuf compatible solution of the [yalantinglibs](https://github.com/alibaba/yalantinglibs).

By walking through creating a simple example application, it shows you how to

- Define message formats in a `.proto` file. (same as official protobuf document)
- Use the protocol buffer compiler (`protoc`) with `struct_pb` plugin.
- Use the `struct_pb` low-level API to write and read messages.

## Defining Your Protocol Format

we use proto3 here.

```
syntax = "proto3";

package tutorial;

message Person {
  string name = 1;
  int32 id = 2;
  string email = 3;

  enum PhoneType {
    MOBILE = 0;
    HOME = 1;
    WORK = 2;
  }

  message PhoneNumber {
    string number = 1;
    PhoneType type = 2;
  }

  repeated PhoneNumber phones = 4;
}

message AddressBook {
  repeated Person people = 1;
}

```

## Compiling Your Protocol Buffers

Now that you have a `.proto`, the next thing you need to do is generate the struct you'll need to
read and write `AddressBook` (and hence `Person` and `PhoneNumber`) messages.
To do this, you need to run the protocol buffer compiler `protoc` with `struct_pb` plugin `protoc-gen-struct_pb` on your `.proto`.

```shell
protoc -I=$SRC_DIR --plugin=$PATH_TO_protoc-gen-struct_pb --struct_pb_out=$DST_DIR $SRC_DIR/addressbook.proto
```
see [Generating source code](https://alibaba.github.io/yalantinglibs/guide/struct-pb-generating-your-struct.html) for details.

This generates the following files in your specified destination directory:

- `addressbook.struct_pb.h`, the header which declares your generated struct.
- `addressbook.struct_pb.cc`, which contains the `struct_pb` low-level API implementation of your struct.


## The struct_pb low-level API

Let's look at some generated code and see what struct and functions the compiler has created for you.
If you look in `addressbook.struct_pb.h`, you can see that you have a struct for each message you specified in `addressbook.proto`.


```cpp

namespace tutorial {
struct Person;
struct AddressBook;
struct Person {
  enum class PhoneType: int {
    MOBILE = 0,
    HOME = 1,
    WORK = 2,
  };
  struct PhoneNumber {
    std::string number; // string, field number = 1
    ::tutorial::Person::PhoneType type; // enum, field number = 2
  };
  std::string name; // string, field number = 1
  int32_t id; // int32, field number = 2
  std::string email; // string, field number = 3
  std::vector<::tutorial::Person::PhoneNumber> phones; // message, field number = 4
};
struct AddressBook {
  std::vector<::tutorial::Person> people; // message, field number = 1
};

} // namespace tutorial
namespace struct_pb {
namespace internal {
// ::tutorial::Person::PhoneNumber
template<>
std::size_t get_needed_size<::tutorial::Person::PhoneNumber>(const ::tutorial::Person::PhoneNumber&, const ::struct_pb::UnknownFields& unknown_fields);
template<>
void serialize_to<::tutorial::Person::PhoneNumber>(char*, std::size_t, const ::tutorial::Person::PhoneNumber&, const ::struct_pb::UnknownFields& unknown_fields);
template<>
bool deserialize_to<::tutorial::Person::PhoneNumber>(::tutorial::Person::PhoneNumber&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
template<>
bool deserialize_to<::tutorial::Person::PhoneNumber>(::tutorial::Person::PhoneNumber&, const char*, std::size_t);

// ::tutorial::Person
template<>
std::size_t get_needed_size<::tutorial::Person>(const ::tutorial::Person&, const ::struct_pb::UnknownFields& unknown_fields);
template<>
void serialize_to<::tutorial::Person>(char*, std::size_t, const ::tutorial::Person&, const ::struct_pb::UnknownFields& unknown_fields);
template<>
bool deserialize_to<::tutorial::Person>(::tutorial::Person&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
template<>
bool deserialize_to<::tutorial::Person>(::tutorial::Person&, const char*, std::size_t);

// ::tutorial::AddressBook
template<>
std::size_t get_needed_size<::tutorial::AddressBook>(const ::tutorial::AddressBook&, const ::struct_pb::UnknownFields& unknown_fields);
template<>
void serialize_to<::tutorial::AddressBook>(char*, std::size_t, const ::tutorial::AddressBook&, const ::struct_pb::UnknownFields& unknown_fields);
template<>
bool deserialize_to<::tutorial::AddressBook>(::tutorial::AddressBook&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
template<>
bool deserialize_to<::tutorial::AddressBook>(::tutorial::AddressBook&, const char*, std::size_t);

} // internal
} // struct_pb
```

## The struct_pb high-level API

We encapsulate the low-level API to make `struct_pb` easier to use.

```
/*
 * High-Level API for struct_pb user
 * If you need more fine-grained operations, encapsulate the internal API yourself.
 */
template<typename Buffer = std::vector<char>, typename T>
Buffer serialize(const T& t, const UnknownFields& unknown_fields = {}) {
  Buffer buffer;
  auto size = struct_pb::internal::get_needed_size(t, unknown_fields);
  buffer.resize(size);
  struct_pb::internal::serialize_to(buffer.data(), buffer.size(), t, unknown_fields);
  return buffer;
}
template<typename T, typename Buffer>
bool deserialize_to(T& t, UnknownFields& unknown_fields, const Buffer& buffer) {
  return struct_pb::internal::deserialize_to(t, buffer.data(), buffer.size(), unknown_fields);
}
template<typename T, typename Buffer>
bool deserialize_to(T& t, const Buffer& buffer) {
  return struct_pb::internal::deserialize_to(t, buffer.data(), buffer.size());
}

```

## Writing A Message

```cpp
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
      break ;
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
```

## Reading A Message

```cpp
void list_people(const tutorial::AddressBook& address_book) {
  std::cout << "==================================" << std::endl;
  std::cout << "          List People             " << std::endl;
  std::cout << "==================================" << std::endl;
  for(const auto& person: address_book.people) {
    std::cout << "     Person ID: " << person.id << std::endl;
    std::cout << "          Name: " << person.name << std::endl;
    if (!person.email.empty()) {
      std::cout << "E-mail address: " << person.email << std::endl;
    }
    for(const auto& phone: person.phones) {
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
```

## the demo code

- [code in yalantinglibs repo](https://github.com/alibaba/yalantinglibs/blob/main/src/struct_pb/examples/tutorial.cpp)
- [code in standalone repo](https://github.com/PikachuHyA/struct_pb_tutorial)
