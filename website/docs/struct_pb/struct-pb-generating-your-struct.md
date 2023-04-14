# Generating source code

First, you need install protobuf.

on Mac
```shell
brew install protobuf
```

on Ubuntu
```shell
apt-get install protobuf
```

on CentOS
```shell
yum install protobuf
```

Second, you need install or compile struct_pb protoc plugin `protoc-gen-struct_pb`.

Finally, you can use the following command to generate the code.

```shell
protoc --plugin=protoc-gen-struct_pb --struct_pb_out . xxx.proto
```

or use the helper function on your `CMakeLists.txt`

```cmake
find_package(Protobuf)
protobuf_generate_struct_pb(PROTO_SRCS PROTO_HDRS xxxx.proto)
```

