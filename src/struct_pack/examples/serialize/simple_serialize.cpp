/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <memory>

#include "struct_pack/struct_pack.hpp"
#include "struct_pack/struct_pack/struct_pack_impl.hpp"

static uint32_t deserialize_to_u32(const uint8_t bytes[4])
{
  return ((bytes[3] << 24) + (bytes[2] << 16) + (bytes[1] << 8) + bytes[0]);

}


static uint64_t deserialize_to_u64(const uint8_t m[8]) 
{
    return ((uint64_t)m[7] << 56) | ((uint64_t)m[6] << 48) | ((uint64_t)m[5] << 40) | ((uint64_t)m[4] << 32) 
		| ((uint64_t)m[3] << 24) | ((uint64_t)m[2] << 16) | ((uint64_t)m[1] << 8) | ((uint64_t)m[0] << 0);
}

struct person {
  int age;
  std::string name;
};

int main()
{ 
  person p{.age = 21, .name = "Betty"};
  std::string buffer = struct_pack::serialize<std::string>(p);

  std::cout << typeid(buffer).name() << std::endl;
  for (int i = 0; i < buffer.size(); i++)
    std::cout << buffer[i] << std::endl;

  std::fstream file;
  file.open("./example.txt", std::ios::out | std::ios::binary);

  file << buffer;

  file.close();

  // c deserialize
  // read serialize string and print deserialize's value 
  uint8_t *text = (uint8_t*)malloc(128);
  FILE *fp = NULL;
  fp = fopen("./example.txt", "r");
  fgets((char*)text, 128, fp);
  fclose(fp);
  // test hash code
  uint32_t hash_code = deserialize_to_u32(text);
  std::cout << "hash code is: " << hash_code << std::endl;

  int offset = 4; // first 4 bytes are hash code, no use for other language
  int age = deserialize_to_u32(text + offset);
  std::cout << "deserialize get age field: " << age << std::endl;
  offset += 4;
  // only one byte, maxsize string size is 255
  uint8_t string_length = text[offset];
  offset += 1;
  char *name = (char*)malloc(string_length);
  memcpy(name, (char*)(text + offset), string_length);
  std::cout << "deserialize get name field: " << name << std::endl;

  free(text);
  free(name);

  return 0;
}
