# struct_pack benchmark

## How to?

- download dependencies

assume in `REPO_ROOT`

msgpack, see https://github.com/msgpack/msgpack-c/tree/cpp_master

protobuf, see https://github.com/protocolbuffers/protobuf

```shell
cd thirdparty
git clone https://github.com/PragmaTwice/protopuf.git
git clone https://github.com/mapbox/protozero.git
```

- build

assume in `REPO_ROOT`

```shell
mkdir build
cd build
cmake ..
make -j struct_pack_benchmark
```
- run

assume in `REPO_ROOT`

```shell
cd build
./src/struct_pack/benchmark/struct_pack_benchmark
```

