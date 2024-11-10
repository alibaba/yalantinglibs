# Instructions for using the .proto file to struc_pack header tool

## compile

libprotobuf and libprotoc version is 3.21.0.

```shell
mkdir build
cd build
cmake .. && make
```

## usage

Usage:

```shell
protoc --plugin=protoc-gen-example=./proto_to_struct data.proto --example_out=protos
```

data.proto is the original file that is intended to be the structure pack file.

`--example_out=` is followed by the path to the generated file.

data.proto:

```proto
syntax = "proto3";

package mygame;

option optimize_for = SPEED;
option cc_enable_arenas = true;

message Vec3 {
    float x = 1;
    float y = 2;
    float z = 3;
}

message Weapon {
    string name = 1;
    int32 damage = 2;
}

message Monster {
  Vec3 pos = 1;
  int32 mana = 2;
  int32 hp = 3;
  string name = 4;
  bytes inventory = 5;
  enum Color {
        Red = 0;
        Green = 1;
        Blue = 2;
  }
  Color color = 6;
  repeated Weapon weapons = 7;
  Weapon equipped = 8;
  repeated Vec3 path = 9;
}

message Monsters {
    repeated Monster monsters = 1;
}

message person {
    int32 id = 1;
    string name = 2;
    int32 age = 3;
    double salary = 4;
}

message persons {
    repeated person person_list = 1;
}

message bench_int32 {
    int32 a = 1;
    int32 b = 2;
    int32 c = 3;
    int32 d = 4;
}
```

generate struct pack file:

```cpp
#pragma once
#include <ylt/struct_pb.hpp>

#define PUBLIC(T) : public iguana::base_impl<T>

enum class Color {
	Red = 0,
	Green = 1,
	Blue = 2,
};

struct Vec3 PUBLIC(Vec3) {
	Vec3() = default;
	Vec3(float a, float b, float c) : x(a), y(b), z(c) {}
	float x;
	float y;
	float z;
};
YLT_REFL(Vec3, x, y, z);

struct Weapon PUBLIC(Weapon) {
	Weapon() = default;
	Weapon(std::string a, int32 b) : name(std::move(a)), damage(b) {}
	std::string name;
	int32 damage;
};
YLT_REFL(Weapon, name, damage);

struct Monster PUBLIC(Monster) {
	Monster() = default;
	Monster(Vec3 a, int32 b, int32 c, std::string d, std::string e, enum Color f, std::vector<Weapon> g, Weapon h, std::vector<Vec3> i) : pos(a), mana(b), hp(c), name(std::move(d)), inventory(std::move(e)), weapons(std::move(g)), equipped(h), path(std::move(i)) {}
	Vec3 pos;
	int32 mana;
	int32 hp;
	std::string name;
	std::string inventory;
	enum Color color;
	std::vector<Weapon> weapons;
	Weapon equipped;
	std::vector<Vec3> path;
};
YLT_REFL(Monster, pos, mana, hp, name, inventory, color, weapons, equipped, path);

struct Monsters PUBLIC(Monsters) {
	Monsters() = default;
	Monsters(std::vector<Monster> a) : monsters(std::move(a)) {}
	std::vector<Monster> monsters;
};
YLT_REFL(Monsters, monsters);

struct person PUBLIC(person) {
	person() = default;
	person(int32 a, std::string b, int32 c, double d) : id(a), name(std::move(b)), age(c), salary(d) {}
	int32 id;
	std::string name;
	int32 age;
	double salary;
};
YLT_REFL(person, id, name, age, salary);

struct persons PUBLIC(persons) {
	persons() = default;
	persons(std::vector<person> a) : person_list(std::move(a)) {}
	std::vector<person> person_list;
};
YLT_REFL(persons, person_list);

struct bench_int32 PUBLIC(bench_int32) {
	bench_int32() = default;
	bench_int32(int32 a, int32 b, int32 c, int32 d) : a(a), b(b), c(c), d(d) {}
	int32 a;
	int32 b;
	int32 c;
	int32 d;
};
YLT_REFL(bench_int32, a, b, c, d);


```