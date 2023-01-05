# struct_pb Guide (proto3)

## Defining A Message Type

Here is a simple message type from [official document - Defining A Message Type](https://developers.google.com/protocol-buffers/docs/proto3#simple).

```
syntax = "proto3";

message SearchRequest {
  string query = 1;
  int32 page_number = 2;
  int32 result_per_page = 3;
}
```
Our protoc plugin convert the message type to C++ struct and corresponding low-level serialization API.

```cpp
struct SearchRequest {
  std::string query; // string, field number = 1
  int32_t page_number; // int32, field number = 2
  int32_t result_per_page; // int32, field number = 3
};
namespace struct_pb {
namespace internal {
// ::SearchRequest
std::size_t get_needed_size(const ::SearchRequest&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::SearchRequest&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::SearchRequest&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::SearchRequest&, const char*, std::size_t);

} // internal
} // struct_pb
```

You can use the struct `SearchRequest` as a normal C++ struct.

## Enumerations

```
syntax = "proto3";
enum Corpus {
  CORPUS_UNSPECIFIED = 0;
  CORPUS_UNIVERSAL = 1;
  CORPUS_WEB = 2;
  CORPUS_IMAGES = 3;
  CORPUS_LOCAL = 4;
  CORPUS_NEWS = 5;
  CORPUS_PRODUCTS = 6;
  CORPUS_VIDEO = 7;
}
```

```cpp
enum class Corpus: int {
  CORPUS_UNSPECIFIED = 0,
  CORPUS_UNIVERSAL = 1,
  CORPUS_WEB = 2,
  CORPUS_IMAGES = 3,
  CORPUS_LOCAL = 4,
  CORPUS_NEWS = 5,
  CORPUS_PRODUCTS = 6,
  CORPUS_VIDEO = 7,
};
```

## Nested messages

Here is the sample from [proto3 Nested Types](https://developers.google.com/protocol-buffers/docs/proto3#nested)

```
syntax = "proto3";
message Outer {                  // Level 0
  message MiddleAA {  // Level 1
    message Inner {   // Level 2
      int64 ival = 1;
      bool  booly = 2;
    }
  }
  message MiddleBB {  // Level 1
    message Inner {   // Level 2
      int32 ival = 1;
      bool  booly = 2;
    }
  }
}
```

```cpp
struct Outer {
  struct MiddleAA {
    struct Inner {
      int64_t ival; // int64, field number = 1
      bool booly; // bool, field number = 2
    };
  };
  struct MiddleBB {
    struct Inner {
      int32_t ival; // int32, field number = 1
      bool booly; // bool, field number = 2
    };
  };
};
```

## Oneof

Here is the sample from [proto3 Oneof](https://developers.google.com/protocol-buffers/docs/proto3#oneof)

```
syntax = "proto3";
message SampleMessage {
  oneof test_oneof {
    string name = 4;
    SubMessage sub_message = 9;
  }
}
message SubMessage {
  int32 val = 1;
}
```
The oneof type is mapped to `std::varint` with lots of helper functions.

```cpp
struct SampleMessage {
  enum class TestOneofCase {
    none = 0,
    name = 4,
    sub_message = 9,
  };
  TestOneofCase test_oneof_case() const {
    switch (test_oneof.index()) {
      case 1:
        return TestOneofCase::name;
      case 2:
        return TestOneofCase::sub_message;
      default:
        return TestOneofCase::none;
    }
  }

  std::variant<std::monostate
  , std::string // string, field number = 4
  , std::unique_ptr<::SubMessage> // message, field number = 9
  > test_oneof;

  bool has_name() const {
    return test_oneof.index() == 1;
  }
  void set_name(std::string name) {
    test_oneof.emplace<1>(std::move(name));
  }
  const std::string& name() const {
    assert(test_oneof.index() == 1);
    return std::get<1>(test_oneof);
  }

  bool has_sub_message() const {
    return test_oneof.index() == 2;
  }
  void set_allocated_sub_message(::SubMessage* p) {
    assert(p);
    test_oneof.emplace<2>(p);
  }
  const std::unique_ptr<::SubMessage>& sub_message() const {
    assert(test_oneof.index() == 2);
    return std::get<2>(test_oneof);
  }
};
struct SubMessage {
  int32_t val; // int32, field number = 1
};
```

## Maps

```
syntax = "proto3";
message SampleMap {
  map<string, Project> projects = 3;
}
message Project {
  string name = 1;
}
```

```cpp
struct SampleMap {
  std::map<std::string, ::Project> projects; // message, field number = 3
};
struct Project {
  std::string name; // string, field number = 1
};
```