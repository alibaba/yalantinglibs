# struct_pb API

Current, struct_pb provide low-level APIs only, which are in namespace `struct_pb::internal`.

For example, the `SearchRequest` message from [Language Guide (proto3)](https://developers.google.com/protocol-buffers/docs/proto3#simple)

```
syntax = "proto3";

message SearchRequest {
  string query = 1;
  int32 page_number = 2;
  int32 result_per_page = 3;
}
```

we generate the source code
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

## Serialization

- get_needed_size

To get the buffer size, use the `get_needed_size` function.

To support [Unknown Fields](https://developers.google.com/protocol-buffers/docs/proto3#unknowns),
an extra argument `unknown_fields` with default value is added.

- serialize_to

To serialize a message to a buffer, use the `serialize_to` function.

Before use this function, make sure the buffer is sufficiently large. 
e.g. use `get_needed_size` and then allocate buffer.

To support [Unknown Fields](https://developers.google.com/protocol-buffers/docs/proto3#unknowns),
an extra argument `unknown_fields` with default value is added.

## Deserialization

- deserialize_to

To deserialize a message from buffer, use the `deserialize_to` function.

Returns true on success. 

To support [Unknown Fields](https://developers.google.com/protocol-buffers/docs/proto3#unknowns),
the overload function with an extra argument `unknown_fields` is added.
