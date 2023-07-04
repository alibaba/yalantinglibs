#include <cassert>
#include <iostream>

#include "ylt/struct_xml/xml_reader.h"
#include "ylt/struct_xml/xml_writer.h"

struct Owner_t {
  std::string_view ID;
  std::string_view DisplayName;
};
REFLECTION(Owner_t, ID, DisplayName);

struct Contents_t {
  std::string_view Key;
  std::string_view LastModified;
  std::string_view ETag;
  std::string_view Type;
  uint32_t Size;
  std::string_view StorageClass;
  Owner_t Owner;
};
REFLECTION(Contents_t, Key, LastModified, ETag, Type, Size, StorageClass,
           Owner);

struct ListBucketResult {
  std::string_view Name;
  std::string_view Prefix;
  std::string_view Marker;
  int MaxKeys;
  std::string_view Delimiter;
  bool IsTruncated;
  std::vector<Contents_t> Contents;
  //  std::unordered_map<std::string, std::string> __attr;
};
REFLECTION(ListBucketResult, Name, Prefix, Marker, MaxKeys, Delimiter,
           IsTruncated, Contents);

std::string xml_str = R"(
<?xml version="1.0" encoding="UTF-8"?>
<ListBucketResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
<Name>examplebucket</Name>
<Prefix></Prefix>
<Marker></Marker>
<MaxKeys>100</MaxKeys>
<Delimiter></Delimiter>
<IsTruncated>false</IsTruncated>
<Contents>
      <Key>fun/movie/001.avi</Key>
      <LastModified>2012-02-24T08:43:07.000Z</LastModified>
      <ETag>"5B3C1A2E053D763E1B002CC607C5A0FE1****"</ETag>
      <Type>Normal</Type>
      <Size>344606</Size>
      <StorageClass>Standard</StorageClass>
      <Owner>
          <ID>0022012****</ID>
          <DisplayName>user-example</DisplayName>
      </Owner>
</Contents>
<Contents>
      <Key>fun/movie/007.avi</Key>
      <LastModified>2012-02-24T08:43:27.000Z</LastModified>
      <ETag>"5B3C1A2E053D763E1B002CC607C5A0FE1****"</ETag>
      <Type>Normal</Type>
      <Size>344606</Size>
      <StorageClass>Standard</StorageClass>
      <Owner>
          <ID>0022012****</ID>
          <DisplayName>user-example</DisplayName>
      </Owner>
</Contents>
<Contents>
      <Key>fun/test.jpg</Key>
      <LastModified>2012-02-24T08:42:32.000Z</LastModified>
      <ETag>"5B3C1A2E053D763E1B002CC607C5A0FE1****"</ETag>
      <Type>Normal</Type>
      <Size>344606</Size>
      <StorageClass>Standard</StorageClass>
      <Owner>
          <ID>0022012****</ID>
          <DisplayName>user-example</DisplayName>
      </Owner>
</Contents>
<Contents>
      <Key>oss.jpg</Key>
      <LastModified>2012-02-24T06:07:48.000Z</LastModified>
      <ETag>"5B3C1A2E053D763E1B002CC607C5A0FE1****"</ETag>
      <Type>Normal</Type>
      <Size>344606</Size>
      <StorageClass>Standard</StorageClass>
      <Owner>
          <ID>0022012****</ID>
          <DisplayName>user-example</DisplayName>
      </Owner>
</Contents>
</ListBucketResult>
)";

void nested_xml() {
  ListBucketResult list_bucket;
  bool r = struct_xml::from_xml(list_bucket, xml_str.data());
  assert(r);

  std::string out;
  r = struct_xml::to_xml_pretty(list_bucket, out);
  assert(r);
  std::cout << out << "\n";
}

struct book_attr_t {
  std::map<std::string, float> __attr;
  std::string title;
};
REFLECTION(book_attr_t, __attr, title);
void attribute() {
  std::string str = R"(
    <book_attr_t id="5" pages="392" price="79.9">
      <title>C++ templates</title>
    </book_with_attr_t>
  )";
  book_attr_t b;
  iguana::from_xml(b, str.data());
  assert(b.__attr["id"] == 5);
  assert(b.__attr["pages"] == 392.0f);
  assert(b.__attr["price"] == 79.9f);
}

struct person {
  std::string name;
  int age;
};
REFLECTION(person, name, age);

void basic_usage() {
  std::string xml = R"(
<person>
    <name>tom</name>
    <age>20</age>
</person>
)";

  person p;
  bool r = struct_xml::from_xml(p, xml.data());
  assert(r);
  assert(p.name == "tom" && p.age == 20);

  std::string str;
  r = struct_xml::to_xml_pretty(p, str);
  assert(r);
  std::cout << str;
}

void get_error() {
  std::string xml = "invalid xml";
  person p;
  bool r = struct_xml::from_xml(p, xml.data());
  if (!r) {
    std::cout << struct_xml::get_last_read_err() << "\n";
  }
}

struct person2 {
  std::string name;
  int age;
};
REFLECTION(person2, name, age);
REQUIRED(person2, age);

void required_field() {
  std::string xml = R"(
<person>
    <name>tom</name>
</person>
)";

  person2 p;
  bool r = struct_xml::from_xml(p, xml.data());
  assert(!r);
  std::cout << struct_xml::get_last_read_err() << "\n";
}

int main() {
  basic_usage();
  nested_xml();
  attribute();
  get_error();
  required_field();
}