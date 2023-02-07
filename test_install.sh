#
# Copyright (c) 2023, Alibaba Group Holding Limited;
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
set -e
echo "test yaLanTingLibs install"
rm -rf tmp
mkdir tmp
cd tmp
echo "copy test code"
prj_list="struct_pack struct_pb coro_rpc easylog"
for lib in $prj_list
do
  mkdir "$lib"
  echo "    copy $lib"
  cp -r ../src/"$lib"/tests "$lib"/
done
cp -r ../thirdparty/doctest .
cp -r ../src/struct_pb/conformance struct_pb/
echo "add CMakeLists.txt"
prj_file=CMakeLists.txt
(
cat << EOF
cmake_minimum_required(VERSION 3.15)
project(yaLanTingLibs_install_tests
        VERSION 1.0.0
        LANGUAGES CXX
        )
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
find_package(Threads REQUIRED)
find_package(OpenSSL)
find_package(yalantinglibs REQUIRED)
add_compile_definitions(ASYNC_SIMPLE_HAS_NOT_AIO)
#############################
# doctest
#############################
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE doctest)
EOF
) > $prj_file
for lib in $prj_list
do
  echo "add_subdirectory($lib/tests)" >> $prj_file
done
echo "add_subdirectory(struct_pb/conformance)" >> $prj_file
# workaround
cp -r ../src/struct_pack/benchmark struct_pack/
echo "target_include_directories(test_struct_pb PUBLIC struct_pack/benchmark)" >> $prj_file

echo "build tests"
cmake -B build -S . -DCMAKE_PREFIX_PATH="$(pwd)/../installed"
cmake --build build -j
echo "run tests"
cd build && ctest
