// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "binary_json_conformance_suite.h"

#include "google/protobuf/text_format.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/type_resolver_util.h"
// #include "absl/status/status.h"
#include "absl/strings/str_cat.h"
// #include "json/json.h"
#include "conformance/conformance.pb.h"
#include "conformance_test.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/wire_format_lite.h"

namespace proto2_messages = protobuf_test_messages::proto2;

using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using conformance::WireFormat;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::TextFormat;
using google::protobuf::internal::WireFormatLite;
using google::protobuf::util::NewTypeResolverForDescriptorPool;
using proto2_messages::TestAllTypesProto2;
using protobuf_test_messages::proto3::TestAllTypesProto3;
using std::string;

namespace {

static const char kTypeUrlPrefix[] = "type.googleapis.com";

// The number of repetitions to use for performance tests.
// Corresponds approx to 500KB wireformat bytes.
static const size_t kPerformanceRepeatCount = 50000;

static string GetTypeUrl(const Descriptor* message) {
  return string(kTypeUrlPrefix) + "/" + message->full_name();
}

/* Routines for building arbitrary protos *************************************/

// We would use CodedOutputStream except that we want more freedom to build
// arbitrary protos (even invalid ones).

const string empty;

string cat(const string& a, const string& b, const string& c = empty,
           const string& d = empty, const string& e = empty,
           const string& f = empty, const string& g = empty,
           const string& h = empty, const string& i = empty,
           const string& j = empty, const string& k = empty,
           const string& l = empty) {
  string ret;
  ret.reserve(a.size() + b.size() + c.size() + d.size() + e.size() + f.size() +
              g.size() + h.size() + i.size() + j.size() + k.size() + l.size());
  ret.append(a);
  ret.append(b);
  ret.append(c);
  ret.append(d);
  ret.append(e);
  ret.append(f);
  ret.append(g);
  ret.append(h);
  ret.append(i);
  ret.append(j);
  ret.append(k);
  ret.append(l);
  return ret;
}

// The maximum number of bytes that it takes to encode a 64-bit varint.
#define VARINT_MAX_LEN 10

size_t vencode64(uint64_t val, int over_encoded_bytes, char* buf) {
  if (val == 0) {
    buf[0] = 0;
    return 1;
  }
  size_t i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val || over_encoded_bytes)
      byte |= 0x80U;
    buf[i++] = byte;
  }
  while (over_encoded_bytes--) {
    assert(i < 10);
    uint8_t byte = over_encoded_bytes ? 0x80 : 0;
    buf[i++] = byte;
  }
  return i;
}

string varint(uint64_t x) {
  char buf[VARINT_MAX_LEN];
  size_t len = vencode64(x, 0, buf);
  return string(buf, len);
}

// Encodes a varint that is |extra| bytes longer than it needs to be, but still
// valid.
string longvarint(uint64_t x, int extra) {
  char buf[VARINT_MAX_LEN];
  size_t len = vencode64(x, extra, buf);
  return string(buf, len);
}

// TODO: proper byte-swapping for big-endian machines.
string fixed32(void* data) { return string(static_cast<char*>(data), 4); }
string fixed64(void* data) { return string(static_cast<char*>(data), 8); }

string delim(const string& buf) { return cat(varint(buf.size()), buf); }
string u32(uint32_t u32) { return fixed32(&u32); }
string u64(uint64_t u64) { return fixed64(&u64); }
string flt(float f) { return fixed32(&f); }
string dbl(double d) { return fixed64(&d); }
string zz32(int32_t x) { return varint(WireFormatLite::ZigZagEncode32(x)); }
string zz64(int64_t x) { return varint(WireFormatLite::ZigZagEncode64(x)); }

string tag(uint32_t fieldnum, char wire_type) {
  return varint((fieldnum << 3) | wire_type);
}

string GetDefaultValue(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_BOOL:
      return varint(0);
    case FieldDescriptor::TYPE_SINT32:
      return zz32(0);
    case FieldDescriptor::TYPE_SINT64:
      return zz64(0);
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return u32(0);
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return u64(0);
    case FieldDescriptor::TYPE_FLOAT:
      return flt(0);
    case FieldDescriptor::TYPE_DOUBLE:
      return dbl(0);
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_MESSAGE:
      return delim("");
    default:
      return "";
  }
  return "";
}

string GetNonDefaultValue(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_BOOL:
      return varint(1);
    case FieldDescriptor::TYPE_SINT32:
      return zz32(1);
    case FieldDescriptor::TYPE_SINT64:
      return zz64(1);
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return u32(1);
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return u64(1);
    case FieldDescriptor::TYPE_FLOAT:
      return flt(1);
    case FieldDescriptor::TYPE_DOUBLE:
      return dbl(1);
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return delim("a");
    case FieldDescriptor::TYPE_MESSAGE:
      return delim(cat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1234)));
    default:
      return "";
  }
  return "";
}

#define UNKNOWN_FIELD 666

enum class Packed {
  kUnspecified = 0,
  kTrue = 1,
  kFalse = 2,
};

const FieldDescriptor* GetFieldForType(FieldDescriptor::Type type,
                                       bool repeated, bool is_proto3,
                                       Packed packed = Packed::kUnspecified) {
  const Descriptor* d = is_proto3 ? TestAllTypesProto3().GetDescriptor()
                                  : TestAllTypesProto2().GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (f->type() == type && f->is_repeated() == repeated) {
      if ((packed == Packed::kTrue && !f->is_packed()) ||
          (packed == Packed::kFalse && f->is_packed())) {
        continue;
      }
      return f;
    }
  }

  string packed_string = "";
  const string repeated_string = repeated ? "Repeated " : "Singular ";
  const string proto_string = is_proto3 ? "Proto3" : "Proto2";
  if (packed == Packed::kTrue) {
    packed_string = "Packed ";
  }
  if (packed == Packed::kFalse) {
    packed_string = "Unpacked ";
  }
  GOOGLE_LOG(FATAL) << "Couldn't find field with type: "
                    << repeated_string.c_str() << packed_string.c_str()
                    << FieldDescriptor::TypeName(type) << " for "
                    << proto_string.c_str();
  return nullptr;
}

const FieldDescriptor* GetFieldForMapType(FieldDescriptor::Type key_type,
                                          FieldDescriptor::Type value_type,
                                          bool is_proto3) {
  const Descriptor* d = is_proto3 ? TestAllTypesProto3().GetDescriptor()
                                  : TestAllTypesProto2().GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (f->is_map()) {
      const Descriptor* map_entry = f->message_type();
      const FieldDescriptor* key = map_entry->field(0);
      const FieldDescriptor* value = map_entry->field(1);
      if (key->type() == key_type && value->type() == value_type) {
        return f;
      }
    }
  }

  const string proto_string = is_proto3 ? "Proto3" : "Proto2";
  GOOGLE_LOG(FATAL) << "Couldn't find map field with type: "
                    << FieldDescriptor::TypeName(key_type) << " and "
                    << FieldDescriptor::TypeName(key_type) << " for "
                    << proto_string.c_str();
  return nullptr;
}

const FieldDescriptor* GetFieldForOneofType(FieldDescriptor::Type type,
                                            bool is_proto3,
                                            bool exclusive = false) {
  const Descriptor* d = is_proto3 ? TestAllTypesProto3().GetDescriptor()
                                  : TestAllTypesProto2().GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (f->containing_oneof() && ((f->type() == type) ^ exclusive)) {
      return f;
    }
  }

  const string proto_string = is_proto3 ? "Proto3" : "Proto2";
  GOOGLE_LOG(FATAL) << "Couldn't find oneof field with type: "
                    << FieldDescriptor::TypeName(type) << " for "
                    << proto_string.c_str();
  return nullptr;
}

string UpperCase(string str) {
  for (size_t i = 0; i < str.size(); i++) {
    str[i] = toupper(str[i]);
  }
  return str;
}

std::unique_ptr<Message> NewTestMessage(bool is_proto3) {
  std::unique_ptr<Message> prototype;
  if (is_proto3) {
    prototype.reset(new TestAllTypesProto3());
  }
  else {
    prototype.reset(new TestAllTypesProto2());
  }
  return prototype;
}

bool IsProto3Default(FieldDescriptor::Type type, const string& binary_data) {
  switch (type) {
    case FieldDescriptor::TYPE_DOUBLE:
      return binary_data == dbl(0);
    case FieldDescriptor::TYPE_FLOAT:
      return binary_data == flt(0);
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_ENUM:
      return binary_data == varint(0);
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return binary_data == u64(0);
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return binary_data == u32(0);
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return binary_data == delim("");
    default:
      return false;
  }
}

}  // anonymous namespace

namespace google {
namespace protobuf {

bool BinaryAndJsonConformanceSuite::ParseResponse(
    const ConformanceResponse& response,
    const ConformanceRequestSetting& setting, Message* test_message) {
  const ConformanceRequest& request = setting.GetRequest();
  WireFormat requested_output = request.requested_output_format();
  const string& test_name = setting.GetTestName();
  ConformanceLevel level = setting.GetLevel();

  switch (response.result_case()) {
    case ConformanceResponse::kProtobufPayload: {
      if (requested_output != conformance::PROTOBUF) {
        ReportFailure(test_name, level, request, response,
                      absl::StrCat("Test was asked for ",
                                   WireFormatToString(requested_output),
                                   " output but provided PROTOBUF instead.")
                          .c_str());
        return false;
      }

      if (!test_message->ParseFromString(response.protobuf_payload())) {
        ReportFailure(test_name, level, request, response,
                      "Protobuf output we received from test was unparseable.");
        return false;
      }

      break;
    }

    case ConformanceResponse::kJsonPayload: {
      GOOGLE_LOG(FATAL) << "json not support: " << response.DebugString();
      break;
    }

    default:
      GOOGLE_LOG(FATAL) << test_name
                        << ": unknown payload type: " << response.result_case();
  }

  return true;
}

void BinaryAndJsonConformanceSuite::ExpectParseFailureForProtoWithProtoVersion(
    const string& proto, const string& test_name, ConformanceLevel level,
    bool is_proto3) {
  std::unique_ptr<Message> prototype = NewTestMessage(is_proto3);
  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  ConformanceRequestSetting setting(
      level, conformance::PROTOBUF, conformance::PROTOBUF,
      conformance::BINARY_TEST, *prototype, test_name, proto);

  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  string effective_test_name = absl::StrCat(
      setting.ConformanceLevelToString(level),
      (is_proto3 ? ".Proto3" : ".Proto2"), ".ProtobufInput.", test_name);

  RunTest(effective_test_name, request, &response);
  if (response.result_case() == ConformanceResponse::kParseError) {
    ReportSuccess(effective_test_name);
  }
  else if (response.result_case() == ConformanceResponse::kSkipped) {
    ReportSkip(effective_test_name, request, response);
  }
  else {
    ReportFailure(effective_test_name, level, request, response,
                  "Should have failed to parse, but didn't.");
  }
}

// Expect that this precise protobuf will cause a parse error.
void BinaryAndJsonConformanceSuite::ExpectParseFailureForProto(
    const string& proto, const string& test_name, ConformanceLevel level) {
  ExpectParseFailureForProtoWithProtoVersion(proto, test_name, level, true);
  ExpectParseFailureForProtoWithProtoVersion(proto, test_name, level, false);
}

// Expect that this protobuf will cause a parse error, even if it is followed
// by valid protobuf data.  We can try running this twice: once with this
// data verbatim and once with this data followed by some valid data.
//
// TODO(haberman): implement the second of these.
void BinaryAndJsonConformanceSuite::ExpectHardParseFailureForProto(
    const string& proto, const string& test_name, ConformanceLevel level) {
  return ExpectParseFailureForProto(proto, test_name, level);
}

void BinaryAndJsonConformanceSuite::RunValidJsonTest(
    const string& test_name, ConformanceLevel level, const string& input_json,
    const string& equivalent_text_format, bool is_proto3) {
  //  if (is_proto3) {
  //    RunValidJsonTest(test_name, level, input_json, equivalent_text_format);
  //  } else {
  //    TestAllTypesProto2 prototype;
  //    RunValidJsonTestWithMessage(test_name, level, input_json,
  //                                equivalent_text_format, prototype);
  //  }
}

void BinaryAndJsonConformanceSuite::RunValidProtobufTest(
    const string& test_name, ConformanceLevel level,
    const string& input_protobuf, const string& equivalent_text_format,
    bool is_proto3) {
  std::unique_ptr<Message> prototype = NewTestMessage(is_proto3);

  ConformanceRequestSetting setting1(
      level, conformance::PROTOBUF, conformance::PROTOBUF,
      conformance::BINARY_TEST, *prototype, test_name, input_protobuf);
  RunValidInputTest(setting1, equivalent_text_format);

  if (is_proto3) {
    ConformanceRequestSetting setting2(
        level, conformance::PROTOBUF, conformance::JSON,
        conformance::BINARY_TEST, *prototype, test_name, input_protobuf);
    //    RunValidInputTest(setting2, equivalent_text_format);
  }
}

void BinaryAndJsonConformanceSuite::RunValidBinaryProtobufTest(
    const string& test_name, ConformanceLevel level,
    const string& input_protobuf, bool is_proto3) {
  RunValidBinaryProtobufTest(test_name, level, input_protobuf, input_protobuf,
                             is_proto3);
}

void BinaryAndJsonConformanceSuite::RunValidBinaryProtobufTest(
    const string& test_name, ConformanceLevel level,
    const string& input_protobuf, const string& expected_protobuf,
    bool is_proto3) {
  std::unique_ptr<Message> prototype = NewTestMessage(is_proto3);
  ConformanceRequestSetting setting(
      level, conformance::PROTOBUF, conformance::PROTOBUF,
      conformance::BINARY_TEST, *prototype, test_name, input_protobuf);
  RunValidBinaryInputTest(setting, expected_protobuf, true);
}

void BinaryAndJsonConformanceSuite::RunBinaryPerformanceMergeMessageWithField(
    const string& test_name, const string& field_proto, bool is_proto3) {
  string message_tag = tag(27, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
  string message_proto = cat(message_tag, delim(field_proto));

  string proto;
  for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
    proto.append(message_proto);
  }

  string multiple_repeated_field_proto;
  for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
    multiple_repeated_field_proto.append(field_proto);
  }
  string expected_proto =
      cat(message_tag, delim(multiple_repeated_field_proto));

  RunValidBinaryProtobufTest(test_name, RECOMMENDED, proto, expected_proto,
                             is_proto3);
}

void BinaryAndJsonConformanceSuite::RunValidProtobufTestWithMessage(
    const string& test_name, ConformanceLevel level, const Message* input,
    const string& equivalent_text_format, bool is_proto3) {
  RunValidProtobufTest(test_name, level, input->SerializeAsString(),
                       equivalent_text_format, is_proto3);
}

void BinaryAndJsonConformanceSuite::ExpectParseFailureForJson(
    const string& test_name, ConformanceLevel level, const string& input_json) {
  TestAllTypesProto3 prototype;
  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  ConformanceRequestSetting setting(level, conformance::JSON, conformance::JSON,
                                    conformance::JSON_TEST, prototype,
                                    test_name, input_json);
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  string effective_test_name = absl::StrCat(
      setting.ConformanceLevelToString(level), ".Proto3.JsonInput.", test_name);

  RunTest(effective_test_name, request, &response);
  if (response.result_case() == ConformanceResponse::kParseError) {
    ReportSuccess(effective_test_name);
  }
  else if (response.result_case() == ConformanceResponse::kSkipped) {
    ReportSkip(effective_test_name, request, response);
  }
  else {
    ReportFailure(effective_test_name, level, request, response,
                  "Should have failed to parse, but didn't.");
  }
}

void BinaryAndJsonConformanceSuite::ExpectSerializeFailureForJson(
    const string& test_name, ConformanceLevel level,
    const string& text_format) {
  TestAllTypesProto3 payload_message;
  GOOGLE_CHECK(TextFormat::ParseFromString(text_format, &payload_message))
      << "Failed to parse: " << text_format;

  TestAllTypesProto3 prototype;
  ConformanceRequestSetting setting(
      level, conformance::PROTOBUF, conformance::JSON, conformance::JSON_TEST,
      prototype, test_name, payload_message.SerializeAsString());
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  string effective_test_name = absl::StrCat(
      setting.ConformanceLevelToString(level), ".", test_name, ".JsonOutput");

  RunTest(effective_test_name, request, &response);
  if (response.result_case() == ConformanceResponse::kSerializeError) {
    ReportSuccess(effective_test_name);
  }
  else if (response.result_case() == ConformanceResponse::kSkipped) {
    ReportSkip(effective_test_name, request, response);
  }
  else {
    ReportFailure(effective_test_name, level, request, response,
                  "Should have failed to serialize, but didn't.");
  }
}

void BinaryAndJsonConformanceSuite::TestPrematureEOFForType(
    FieldDescriptor::Type type) {
  // Incomplete values for each wire type.
  static const string incompletes[6] = {
      string("\x80"),     // VARINT
      string("abcdefg"),  // 64BIT
      string("\x80"),     // DELIMITED (partial length)
      string(),           // START_GROUP (no value required)
      string(),           // END_GROUP (no value required)
      string("abc")       // 32BIT
  };

  const FieldDescriptor* field = GetFieldForType(type, false, true);
  const FieldDescriptor* rep_field = GetFieldForType(type, true, true);
  WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(type));
  const string& incomplete = incompletes[wire_type];
  const string type_name =
      UpperCase(string(".") + FieldDescriptor::TypeName(type));

  ExpectParseFailureForProto(
      tag(field->number(), wire_type),
      "PrematureEofBeforeKnownNonRepeatedValue" + type_name, REQUIRED);

  ExpectParseFailureForProto(tag(rep_field->number(), wire_type),
                             "PrematureEofBeforeKnownRepeatedValue" + type_name,
                             REQUIRED);

  ExpectParseFailureForProto(tag(UNKNOWN_FIELD, wire_type),
                             "PrematureEofBeforeUnknownValue" + type_name,
                             REQUIRED);

  ExpectParseFailureForProto(
      cat(tag(field->number(), wire_type), incomplete),
      "PrematureEofInsideKnownNonRepeatedValue" + type_name, REQUIRED);

  ExpectParseFailureForProto(
      cat(tag(rep_field->number(), wire_type), incomplete),
      "PrematureEofInsideKnownRepeatedValue" + type_name, REQUIRED);

  ExpectParseFailureForProto(cat(tag(UNKNOWN_FIELD, wire_type), incomplete),
                             "PrematureEofInsideUnknownValue" + type_name,
                             REQUIRED);

  if (wire_type == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    ExpectParseFailureForProto(
        cat(tag(field->number(), wire_type), varint(1)),
        "PrematureEofInDelimitedDataForKnownNonRepeatedValue" + type_name,
        REQUIRED);

    ExpectParseFailureForProto(
        cat(tag(rep_field->number(), wire_type), varint(1)),
        "PrematureEofInDelimitedDataForKnownRepeatedValue" + type_name,
        REQUIRED);

    // EOF in the middle of delimited data for unknown value.
    ExpectParseFailureForProto(
        cat(tag(UNKNOWN_FIELD, wire_type), varint(1)),
        "PrematureEofInDelimitedDataForUnknownValue" + type_name, REQUIRED);

    if (type == FieldDescriptor::TYPE_MESSAGE) {
      // Submessage ends in the middle of a value.
      string incomplete_submsg =
          cat(tag(WireFormatLite::TYPE_INT32, WireFormatLite::WIRETYPE_VARINT),
              incompletes[WireFormatLite::WIRETYPE_VARINT]);
      ExpectHardParseFailureForProto(
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              varint(incomplete_submsg.size()), incomplete_submsg),
          "PrematureEofInSubmessageValue" + type_name, REQUIRED);
    }
  }
  else if (type != FieldDescriptor::TYPE_GROUP) {
    // Non-delimited, non-group: eligible for packing.

    // Packed region ends in the middle of a value.
    ExpectHardParseFailureForProto(
        cat(tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            varint(incomplete.size()), incomplete),
        "PrematureEofInPackedFieldValue" + type_name, REQUIRED);

    // EOF in the middle of packed region.
    ExpectParseFailureForProto(
        cat(tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            varint(1)),
        "PrematureEofInPackedField" + type_name, REQUIRED);
  }
}

void BinaryAndJsonConformanceSuite::TestValidDataForType(
    FieldDescriptor::Type type,
    std::vector<std::pair<std::string, std::string>> values) {
  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    const string type_name =
        UpperCase(string(".") + FieldDescriptor::TypeName(type));
    WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
        static_cast<WireFormatLite::FieldType>(type));
    const FieldDescriptor* field = GetFieldForType(type, false, is_proto3);
    const FieldDescriptor* rep_field = GetFieldForType(type, true, is_proto3);

    // Test singular data for singular fields.
    for (size_t i = 0; i < values.size(); i++) {
      string proto = cat(tag(field->number(), wire_type), values[i].first);
      // In proto3, default primitive fields should not be encoded.
      string expected_proto =
          is_proto3 && IsProto3Default(field->type(), values[i].second)
              ? ""
              : cat(tag(field->number(), wire_type), values[i].second);
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(expected_proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);

      RunValidProtobufTest(
          absl::StrCat("ValidDataScalar", type_name, "[", i, "]"), REQUIRED,
          proto, text, is_proto3);
      RunValidBinaryProtobufTest(
          absl::StrCat("ValidDataScalarBinary", type_name, "[", i, "]"),
          RECOMMENDED, proto, expected_proto, is_proto3);
    }

    // Test repeated data for singular fields.
    // For scalar message fields, repeated values are merged, which is tested
    // separately.
    if (type != FieldDescriptor::TYPE_MESSAGE) {
      string proto;
      for (size_t i = 0; i < values.size(); i++) {
        proto += cat(tag(field->number(), wire_type), values[i].first);
      }
      string expected_proto =
          cat(tag(field->number(), wire_type), values.back().second);
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(expected_proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);

      RunValidProtobufTest("RepeatedScalarSelectsLast" + type_name, REQUIRED,
                           proto, text, is_proto3);
    }

    // Test repeated fields.
    if (FieldDescriptor::IsTypePackable(type)) {
      const FieldDescriptor* packed_field =
          GetFieldForType(type, true, is_proto3, Packed::kTrue);
      const FieldDescriptor* unpacked_field =
          GetFieldForType(type, true, is_proto3, Packed::kFalse);

      string default_proto_packed;
      string default_proto_unpacked;
      string default_proto_packed_expected;
      string default_proto_unpacked_expected;
      string packed_proto_packed;
      string packed_proto_unpacked;
      string packed_proto_expected;
      string unpacked_proto_packed;
      string unpacked_proto_unpacked;
      string unpacked_proto_expected;

      for (size_t i = 0; i < values.size(); i++) {
        default_proto_unpacked +=
            cat(tag(rep_field->number(), wire_type), values[i].first);
        default_proto_unpacked_expected +=
            cat(tag(rep_field->number(), wire_type), values[i].second);
        default_proto_packed += values[i].first;
        default_proto_packed_expected += values[i].second;
        packed_proto_unpacked +=
            cat(tag(packed_field->number(), wire_type), values[i].first);
        packed_proto_packed += values[i].first;
        packed_proto_expected += values[i].second;
        unpacked_proto_unpacked +=
            cat(tag(unpacked_field->number(), wire_type), values[i].first);
        unpacked_proto_packed += values[i].first;
        unpacked_proto_expected +=
            cat(tag(unpacked_field->number(), wire_type), values[i].second);
      }
      default_proto_packed = cat(
          tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(default_proto_packed));
      default_proto_packed_expected = cat(
          tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(default_proto_packed_expected));
      packed_proto_packed = cat(tag(packed_field->number(),
                                    WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                                delim(packed_proto_packed));
      packed_proto_expected =
          cat(tag(packed_field->number(),
                  WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(packed_proto_expected));
      unpacked_proto_packed =
          cat(tag(unpacked_field->number(),
                  WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(unpacked_proto_packed));

      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(default_proto_packed_expected);
      string text;
      TextFormat::PrintToString(*test_message, &text);

      // Ensures both packed and unpacked data can be parsed.
      RunValidProtobufTest(
          absl::StrCat("ValidDataRepeated", type_name, ".UnpackedInput"),
          REQUIRED, default_proto_unpacked, text, is_proto3);
      RunValidProtobufTest(
          absl::StrCat("ValidDataRepeated", type_name, ".PackedInput"),
          REQUIRED, default_proto_packed, text, is_proto3);

      // proto2 should encode as unpacked by default and proto3 should encode as
      // packed by default.
      string expected_proto = rep_field->is_packed()
                                  ? default_proto_packed_expected
                                  : default_proto_unpacked_expected;
      RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                              ".UnpackedInput.DefaultOutput"),
                                 RECOMMENDED, default_proto_unpacked,
                                 expected_proto, is_proto3);
      RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                              ".PackedInput.DefaultOutput"),
                                 RECOMMENDED, default_proto_packed,
                                 expected_proto, is_proto3);
      RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                              ".UnpackedInput.PackedOutput"),
                                 RECOMMENDED, packed_proto_unpacked,
                                 packed_proto_expected, is_proto3);
      RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                              ".PackedInput.PackedOutput"),
                                 RECOMMENDED, packed_proto_packed,
                                 packed_proto_expected, is_proto3);
      RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                              ".UnpackedInput.UnpackedOutput"),
                                 RECOMMENDED, unpacked_proto_unpacked,
                                 unpacked_proto_expected, is_proto3);
      RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                              ".PackedInput.UnpackedOutput"),
                                 RECOMMENDED, unpacked_proto_packed,
                                 unpacked_proto_expected, is_proto3);
    }
    else {
      string proto;
      string expected_proto;
      for (size_t i = 0; i < values.size(); i++) {
        proto += cat(tag(rep_field->number(), wire_type), values[i].first);
        expected_proto +=
            cat(tag(rep_field->number(), wire_type), values[i].second);
      }
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(expected_proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);

      RunValidProtobufTest(absl::StrCat("ValidDataRepeated", type_name),
                           REQUIRED, proto, text, is_proto3);
    }
  }
}

void BinaryAndJsonConformanceSuite::TestValidDataForRepeatedScalarMessage() {
  std::vector<std::string> values = {
      delim(cat(
          tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(cat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1234),
                    tag(2, WireFormatLite::WIRETYPE_VARINT), varint(1234),
                    tag(31, WireFormatLite::WIRETYPE_VARINT), varint(1234))))),
      delim(cat(
          tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(cat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(4321),
                    tag(3, WireFormatLite::WIRETYPE_VARINT), varint(4321),
                    tag(31, WireFormatLite::WIRETYPE_VARINT), varint(4321))))),
  };

  const std::string expected =
      R"({
        corecursive: {
          optional_int32: 4321,
          optional_int64: 1234,
          optional_uint32: 4321,
          repeated_int32: [1234, 4321],
        }
      })";

  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    string proto;
    const FieldDescriptor* field =
        GetFieldForType(FieldDescriptor::TYPE_MESSAGE, false, is_proto3);
    for (size_t i = 0; i < values.size(); i++) {
      proto +=
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              values[i]);
    }

    RunValidProtobufTest("RepeatedScalarMessageMerge", REQUIRED, proto,
                         field->name() + ": " + expected, is_proto3);
  }
}

void BinaryAndJsonConformanceSuite::TestValidDataForMapType(
    FieldDescriptor::Type key_type, FieldDescriptor::Type value_type) {
  const string key_type_name =
      UpperCase(string(".") + FieldDescriptor::TypeName(key_type));
  const string value_type_name =
      UpperCase(string(".") + FieldDescriptor::TypeName(value_type));
  WireFormatLite::WireType key_wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(key_type));
  WireFormatLite::WireType value_wire_type =
      WireFormatLite::WireTypeForFieldType(
          static_cast<WireFormatLite::FieldType>(value_type));

  string key1_data = cat(tag(1, key_wire_type), GetDefaultValue(key_type));
  string value1_data =
      cat(tag(2, value_wire_type), GetDefaultValue(value_type));
  string key2_data = cat(tag(1, key_wire_type), GetNonDefaultValue(key_type));
  string value2_data =
      cat(tag(2, value_wire_type), GetNonDefaultValue(value_type));

  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    const FieldDescriptor* field =
        GetFieldForMapType(key_type, value_type, is_proto3);

    {
      // Tests map with default key and value.
      string proto =
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(cat(key1_data, value1_data)));
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);
      RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
                                        value_type_name, ".Default"),
                           REQUIRED, proto, text, is_proto3);
    }

    {
      // Tests map with missing default key and value.
      string proto =
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(""));
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);
      RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
                                        value_type_name, ".MissingDefault"),
                           REQUIRED, proto, text, is_proto3);
    }

    {
      // Tests map with non-default key and value.
      string proto =
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(cat(key2_data, value2_data)));
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);
      RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
                                        value_type_name, ".NonDefault"),
                           REQUIRED, proto, text, is_proto3);
    }

    {
      // Tests map with unordered key and value.
      string proto =
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(cat(value2_data, key2_data)));
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);
      RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
                                        value_type_name, ".Unordered"),
                           REQUIRED, proto, text, is_proto3);
    }

    {
      // Tests map with duplicate key.
      string proto1 =
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(cat(key2_data, value1_data)));
      string proto2 =
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(cat(key2_data, value2_data)));
      string proto = cat(proto1, proto2);
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto2);
      string text;
      TextFormat::PrintToString(*test_message, &text);
      RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
                                        value_type_name, ".DuplicateKey"),
                           REQUIRED, proto, text, is_proto3);
    }

    {
      // Tests map with duplicate key in map entry.
      string proto =
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(cat(key1_data, key2_data, value2_data)));
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);
      RunValidProtobufTest(
          absl::StrCat("ValidDataMap", key_type_name, value_type_name,
                       ".DuplicateKeyInMapEntry"),
          REQUIRED, proto, text, is_proto3);
    }

    {
      // Tests map with duplicate value in map entry.
      string proto =
          cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              delim(cat(key2_data, value1_data, value2_data)));
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);
      RunValidProtobufTest(
          absl::StrCat("ValidDataMap", key_type_name, value_type_name,
                       ".DuplicateValueInMapEntry"),
          REQUIRED, proto, text, is_proto3);
    }
  }
}

void BinaryAndJsonConformanceSuite::TestOverwriteMessageValueMap() {
  string key_data =
      cat(tag(1, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), delim(""));
  string field1_data = cat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1));
  string field2_data = cat(tag(2, WireFormatLite::WIRETYPE_VARINT), varint(1));
  string field31_data =
      cat(tag(31, WireFormatLite::WIRETYPE_VARINT), varint(1));
  string submsg1_data = delim(cat(field1_data, field31_data));
  string submsg2_data = delim(cat(field2_data, field31_data));
  string value1_data =
      cat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(cat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                    submsg1_data)));
  string value2_data =
      cat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(cat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                    submsg2_data)));

  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    const FieldDescriptor* field = GetFieldForMapType(
        FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_MESSAGE, is_proto3);

    string proto1 =
        cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            delim(cat(key_data, value1_data)));
    string proto2 =
        cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            delim(cat(key_data, value2_data)));
    string proto = cat(proto1, proto2);
    std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
    test_message->MergeFromString(proto2);
    string text;
    TextFormat::PrintToString(*test_message, &text);
    RunValidProtobufTest("ValidDataMap.STRING.MESSAGE.MergeValue", REQUIRED,
                         proto, text, is_proto3);
  }
}

void BinaryAndJsonConformanceSuite::TestValidDataForOneofType(
    FieldDescriptor::Type type) {
  const string type_name =
      UpperCase(string(".") + FieldDescriptor::TypeName(type));
  WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(type));

  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    const FieldDescriptor* field = GetFieldForOneofType(type, is_proto3);
    const string default_value =
        cat(tag(field->number(), wire_type), GetDefaultValue(type));
    const string non_default_value =
        cat(tag(field->number(), wire_type), GetNonDefaultValue(type));

    {
      // Tests oneof with default value.
      const string proto = default_value;
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);

      RunValidProtobufTest(
          absl::StrCat("ValidDataOneof", type_name, ".DefaultValue"), REQUIRED,
          proto, text, is_proto3);
      RunValidBinaryProtobufTest(
          absl::StrCat("ValidDataOneofBinary", type_name, ".DefaultValue"),
          RECOMMENDED, proto, proto, is_proto3);
    }

    {
      // Tests oneof with non-default value.
      const string proto = non_default_value;
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);

      RunValidProtobufTest(
          absl::StrCat("ValidDataOneof", type_name, ".NonDefaultValue"),
          REQUIRED, proto, text, is_proto3);
      RunValidBinaryProtobufTest(
          absl::StrCat("ValidDataOneofBinary", type_name, ".NonDefaultValue"),
          RECOMMENDED, proto, proto, is_proto3);
    }

    {
      // Tests oneof with multiple values of the same field.
      const string proto = absl::StrCat(default_value, non_default_value);
      const string expected_proto = non_default_value;
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(expected_proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);

      RunValidProtobufTest(absl::StrCat("ValidDataOneof", type_name,
                                        ".MultipleValuesForSameField"),
                           REQUIRED, proto, text, is_proto3);
      RunValidBinaryProtobufTest(absl::StrCat("ValidDataOneofBinary", type_name,
                                              ".MultipleValuesForSameField"),
                                 RECOMMENDED, proto, expected_proto, is_proto3);
    }

    {
      // Tests oneof with multiple values of the different fields.
      const FieldDescriptor* other_field =
          GetFieldForOneofType(type, is_proto3, true);
      FieldDescriptor::Type other_type = other_field->type();
      WireFormatLite::WireType other_wire_type =
          WireFormatLite::WireTypeForFieldType(
              static_cast<WireFormatLite::FieldType>(other_type));
      const string other_value =
          cat(tag(other_field->number(), other_wire_type),
              GetDefaultValue(other_type));

      const string proto = absl::StrCat(other_value, non_default_value);
      const string expected_proto = non_default_value;
      std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
      test_message->MergeFromString(expected_proto);
      string text;
      TextFormat::PrintToString(*test_message, &text);

      RunValidProtobufTest(absl::StrCat("ValidDataOneof", type_name,
                                        ".MultipleValuesForDifferentField"),
                           REQUIRED, proto, text, is_proto3);
      RunValidBinaryProtobufTest(
          absl::StrCat("ValidDataOneofBinary", type_name,
                       ".MultipleValuesForDifferentField"),
          RECOMMENDED, proto, expected_proto, is_proto3);
    }
  }
}

void BinaryAndJsonConformanceSuite::TestMergeOneofMessage() {
  string field1_data = cat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1));
  string field2a_data = cat(tag(2, WireFormatLite::WIRETYPE_VARINT), varint(1));
  string field2b_data = cat(tag(2, WireFormatLite::WIRETYPE_VARINT), varint(1));
  string field89_data =
      cat(tag(89, WireFormatLite::WIRETYPE_VARINT), varint(1));
  string submsg1_data =
      cat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(cat(field1_data, field2a_data, field89_data)));
  string submsg2_data = cat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                            delim(cat(field2b_data, field89_data)));
  string merged_data =
      cat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(cat(field1_data, field2b_data, field89_data, field89_data)));

  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    const FieldDescriptor* field =
        GetFieldForOneofType(FieldDescriptor::TYPE_MESSAGE, is_proto3);

    string proto1 =
        cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            delim(submsg1_data));
    string proto2 =
        cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            delim(submsg2_data));
    string proto = cat(proto1, proto2);
    string expected_proto =
        cat(tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            delim(merged_data));

    std::unique_ptr<Message> test_message = NewTestMessage(is_proto3);
    test_message->MergeFromString(expected_proto);
    string text;
    TextFormat::PrintToString(*test_message, &text);
    RunValidProtobufTest("ValidDataOneof.MESSAGE.Merge", REQUIRED, proto, text,
                         is_proto3);
    RunValidBinaryProtobufTest("ValidDataOneofBinary.MESSAGE.Merge",
                               RECOMMENDED, proto, expected_proto, is_proto3);
  }
}

void BinaryAndJsonConformanceSuite::TestIllegalTags() {
  // field num 0 is illegal
  string nullfield[] = {"\1DEADBEEF", "\2\1\1", "\3\4", "\5DEAD"};
  for (int i = 0; i < 4; i++) {
    string name = "IllegalZeroFieldNum_Case_0";
    name.back() += i;
    ExpectParseFailureForProto(nullfield[i], name, REQUIRED);
  }
}
template <class MessageType>
void BinaryAndJsonConformanceSuite::TestOneofMessage(MessageType& message,
                                                     bool is_proto3) {
  message.set_oneof_uint32(0);
  RunValidProtobufTestWithMessage("OneofZeroUint32", RECOMMENDED, &message,
                                  "oneof_uint32: 0", is_proto3);
  message.mutable_oneof_nested_message()->set_a(0);
  RunValidProtobufTestWithMessage(
      "OneofZeroMessage", RECOMMENDED, &message,
      is_proto3 ? "oneof_nested_message: {}" : "oneof_nested_message: {a: 0}",
      is_proto3);
  message.mutable_oneof_nested_message()->set_a(1);
  RunValidProtobufTestWithMessage("OneofZeroMessageSetTwice", RECOMMENDED,
                                  &message, "oneof_nested_message: {a: 1}",
                                  is_proto3);
  message.set_oneof_string("");
  RunValidProtobufTestWithMessage("OneofZeroString", RECOMMENDED, &message,
                                  "oneof_string: \"\"", is_proto3);
  message.set_oneof_bytes("");
  RunValidProtobufTestWithMessage("OneofZeroBytes", RECOMMENDED, &message,
                                  "oneof_bytes: \"\"", is_proto3);
  message.set_oneof_bool(false);
  RunValidProtobufTestWithMessage("OneofZeroBool", RECOMMENDED, &message,
                                  "oneof_bool: false", is_proto3);
  message.set_oneof_uint64(0);
  RunValidProtobufTestWithMessage("OneofZeroUint64", RECOMMENDED, &message,
                                  "oneof_uint64: 0", is_proto3);
  message.set_oneof_float(0.0f);
  RunValidProtobufTestWithMessage("OneofZeroFloat", RECOMMENDED, &message,
                                  "oneof_float: 0", is_proto3);
  message.set_oneof_double(0.0);
  RunValidProtobufTestWithMessage("OneofZeroDouble", RECOMMENDED, &message,
                                  "oneof_double: 0", is_proto3);
  message.set_oneof_enum(MessageType::FOO);
  RunValidProtobufTestWithMessage("OneofZeroEnum", RECOMMENDED, &message,
                                  "oneof_enum: FOO", is_proto3);
}

template <class MessageType>
void BinaryAndJsonConformanceSuite::TestUnknownMessage(MessageType& message,
                                                       bool is_proto3) {
  message.ParseFromString("\xA8\x1F\x01");
  RunValidBinaryProtobufTest("UnknownVarint", REQUIRED,
                             message.SerializeAsString(), is_proto3);
}

void BinaryAndJsonConformanceSuite::
    TestBinaryPerformanceForAlternatingUnknownFields() {
  string unknown_field_1 =
      cat(tag(UNKNOWN_FIELD, WireFormatLite::WIRETYPE_VARINT), varint(1234));
  string unknown_field_2 = cat(
      tag(UNKNOWN_FIELD + 1, WireFormatLite::WIRETYPE_VARINT), varint(5678));
  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    string proto;
    for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
      proto.append(unknown_field_1);
      proto.append(unknown_field_2);
    }

    RunValidBinaryProtobufTest(
        "TestBinaryPerformanceForAlternatingUnknownFields", RECOMMENDED, proto,
        is_proto3);
  }
}

void BinaryAndJsonConformanceSuite::
    TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
        FieldDescriptor::Type type) {
  const string type_name =
      UpperCase(string(".") + FieldDescriptor::TypeName(type));
  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    int field_number =
        GetFieldForType(type, true, is_proto3, Packed::kFalse)->number();
    string rep_field_proto = cat(
        tag(field_number, WireFormatLite::WireTypeForFieldType(
                              static_cast<WireFormatLite::FieldType>(type))),
        GetNonDefaultValue(type));

    RunBinaryPerformanceMergeMessageWithField(
        "TestBinaryPerformanceMergeMessageWithRepeatedFieldForType" + type_name,
        rep_field_proto, is_proto3);
  }
}

void BinaryAndJsonConformanceSuite::
    TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
        FieldDescriptor::Type type) {
  const string type_name =
      UpperCase(string(".") + FieldDescriptor::TypeName(type));
  string unknown_field_proto =
      cat(tag(UNKNOWN_FIELD, WireFormatLite::WireTypeForFieldType(
                                 static_cast<WireFormatLite::FieldType>(type))),
          GetNonDefaultValue(type));
  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    RunBinaryPerformanceMergeMessageWithField(
        "TestBinaryPerformanceMergeMessageWithUnknownFieldForType" + type_name,
        unknown_field_proto, is_proto3);
  }
}

void BinaryAndJsonConformanceSuite::RunSuiteImpl() {
  // Hack to get the list of test failures based on whether
  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER is enabled or not.
  conformance::FailureSet failure_set;
  ConformanceRequest req;
  ConformanceResponse res;
  req.set_message_type(failure_set.GetTypeName());
  req.set_protobuf_payload("");
  req.set_requested_output_format(conformance::WireFormat::PROTOBUF);
  RunTest("FindFailures", req, &res);
  GOOGLE_CHECK(failure_set.MergeFromString(res.protobuf_payload()));
  for (const string& failure : failure_set.failure()) {
    AddExpectedFailedTest(failure);
  }

  type_resolver_.reset(NewTypeResolverForDescriptorPool(
      kTypeUrlPrefix, DescriptorPool::generated_pool()));
  type_url_ = GetTypeUrl(TestAllTypesProto3::descriptor());

  if (!performance_) {
    for (int i = 1; i <= FieldDescriptor::MAX_TYPE; i++) {
      if (i == FieldDescriptor::TYPE_GROUP)
        continue;
      TestPrematureEOFForType(static_cast<FieldDescriptor::Type>(i));
    }

    TestIllegalTags();

    int64 kInt64Min = -9223372036854775808ULL;
    int64 kInt64Max = 9223372036854775807ULL;
    uint64 kUint64Max = 18446744073709551615ULL;
    int32 kInt32Max = 2147483647;
    int32 kInt32Min = -2147483648;
    uint32 kUint32Max = 4294967295UL;

    TestValidDataForType(
        FieldDescriptor::TYPE_DOUBLE,
        {
            {dbl(0), dbl(0)},
            {dbl(0.1), dbl(0.1)},
            {dbl(1.7976931348623157e+308), dbl(1.7976931348623157e+308)},
            {dbl(2.22507385850720138309e-308),
             dbl(2.22507385850720138309e-308)},
        });
    TestValidDataForType(
        FieldDescriptor::TYPE_FLOAT,
        {
            {flt(0), flt(0)},
            {flt(0.1), flt(0.1)},
            {flt(1.00000075e-36), flt(1.00000075e-36)},
            {flt(3.402823e+38), flt(3.402823e+38)},  // 3.40282347e+38
            {flt(1.17549435e-38f), flt(1.17549435e-38)},
        });
    TestValidDataForType(FieldDescriptor::TYPE_INT64,
                         {
                             {varint(0), varint(0)},
                             {varint(12345), varint(12345)},
                             {varint(kInt64Max), varint(kInt64Max)},
                             {varint(kInt64Min), varint(kInt64Min)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_UINT64,
                         {
                             {varint(0), varint(0)},
                             {varint(12345), varint(12345)},
                             {varint(kUint64Max), varint(kUint64Max)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_INT32,
                         {
                             {varint(0), varint(0)},
                             {varint(12345), varint(12345)},
                             {longvarint(12345, 2), varint(12345)},
                             {longvarint(12345, 7), varint(12345)},
                             {varint(kInt32Max), varint(kInt32Max)},
                             {varint(kInt32Min), varint(kInt32Min)},
                             {varint(1LL << 33), varint(0)},
                             {varint((1LL << 33) - 1), varint(-1)},
                             {varint(kInt64Max), varint(-1)},
                             {varint(kInt64Min + 1), varint(1)},
                         });
    TestValidDataForType(
        FieldDescriptor::TYPE_UINT32,
        {
            {varint(0), varint(0)},
            {varint(12345), varint(12345)},
            {longvarint(12345, 2), varint(12345)},
            {longvarint(12345, 7), varint(12345)},
            {varint(kUint32Max), varint(kUint32Max)},  // UINT32_MAX
            {varint(1LL << 33), varint(0)},
            {varint((1LL << 33) + 1), varint(1)},
            {varint((1LL << 33) - 1), varint((1LL << 32) - 1)},
            {varint(kInt64Max), varint((1LL << 32) - 1)},
            {varint(kInt64Min + 1), varint(1)},
        });
    TestValidDataForType(FieldDescriptor::TYPE_FIXED64,
                         {
                             {u64(0), u64(0)},
                             {u64(12345), u64(12345)},
                             {u64(kUint64Max), u64(kUint64Max)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_FIXED32,
                         {
                             {u32(0), u32(0)},
                             {u32(12345), u32(12345)},
                             {u32(kUint32Max), u32(kUint32Max)},  // UINT32_MAX
                         });
    TestValidDataForType(FieldDescriptor::TYPE_SFIXED64,
                         {
                             {u64(0), u64(0)},
                             {u64(12345), u64(12345)},
                             {u64(kInt64Max), u64(kInt64Max)},
                             {u64(kInt64Min), u64(kInt64Min)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_SFIXED32,
                         {
                             {u32(0), u32(0)},
                             {u32(12345), u32(12345)},
                             {u32(kInt32Max), u32(kInt32Max)},
                             {u32(kInt32Min), u32(kInt32Min)},
                         });
    // Bools should be serialized as 0 for false and 1 for true. Parsers should
    // also interpret any nonzero value as true.
    TestValidDataForType(FieldDescriptor::TYPE_BOOL,
                         {
                             {varint(0), varint(0)},
                             {varint(1), varint(1)},
                             {varint(-1), varint(1)},
                             {varint(12345678), varint(1)},
                             {varint(1LL << 33), varint(1)},
                             {varint(kInt64Max), varint(1)},
                             {varint(kInt64Min), varint(1)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_SINT32,
                         {
                             {zz32(0), zz32(0)},
                             {zz32(12345), zz32(12345)},
                             {zz32(kInt32Max), zz32(kInt32Max)},
                             {zz32(kInt32Min), zz32(kInt32Min)},
                             {zz64(kInt32Max + 2LL), zz32(1)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_SINT64,
                         {
                             {zz64(0), zz64(0)},
                             {zz64(12345), zz64(12345)},
                             {zz64(kInt64Max), zz64(kInt64Max)},
                             {zz64(kInt64Min), zz64(kInt64Min)},
                         });
    TestValidDataForType(
        FieldDescriptor::TYPE_STRING,
        {
            {delim(""), delim("")},
            {delim("Hello world!"), delim("Hello world!")},
            {delim("\'\"\?\\\a\b\f\n\r\t\v"),
             delim("\'\"\?\\\a\b\f\n\r\t\v")},       // escape
            {delim("谷歌"), delim("谷歌")},          // Google in Chinese
            {delim("\u8C37\u6B4C"), delim("谷歌")},  // unicode escape
            {delim("\u8c37\u6b4c"), delim("谷歌")},  // lowercase unicode
            {delim("\xF0\x9F\x98\x81"), delim("\xF0\x9F\x98\x81")},  // emoji: 😁
        });
    TestValidDataForType(FieldDescriptor::TYPE_BYTES,
                         {
                             {delim(""), delim("")},
                             {delim("Hello world!"), delim("Hello world!")},
                             {delim("\x01\x02"), delim("\x01\x02")},
                             {delim("\xfb"), delim("\xfb")},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_ENUM,
                         {
                             {varint(0), varint(0)},
                             {varint(1), varint(1)},
                             {varint(2), varint(2)},
                             {varint(-1), varint(-1)},
                             {varint(kInt64Max), varint(-1)},
                             {varint(kInt64Min + 1), varint(1)},
                         });
    TestValidDataForRepeatedScalarMessage();
    TestValidDataForType(
        FieldDescriptor::TYPE_MESSAGE,
        {
            {delim(""), delim("")},
            {delim(cat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1234))),
             delim(cat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1234)))},
        });

    TestValidDataForMapType(FieldDescriptor::TYPE_INT32,
                            FieldDescriptor::TYPE_INT32);
    TestValidDataForMapType(FieldDescriptor::TYPE_INT64,
                            FieldDescriptor::TYPE_INT64);
    TestValidDataForMapType(FieldDescriptor::TYPE_UINT32,
                            FieldDescriptor::TYPE_UINT32);
    TestValidDataForMapType(FieldDescriptor::TYPE_UINT64,
                            FieldDescriptor::TYPE_UINT64);
    TestValidDataForMapType(FieldDescriptor::TYPE_SINT32,
                            FieldDescriptor::TYPE_SINT32);
    TestValidDataForMapType(FieldDescriptor::TYPE_SINT64,
                            FieldDescriptor::TYPE_SINT64);
    TestValidDataForMapType(FieldDescriptor::TYPE_FIXED32,
                            FieldDescriptor::TYPE_FIXED32);
    TestValidDataForMapType(FieldDescriptor::TYPE_FIXED64,
                            FieldDescriptor::TYPE_FIXED64);
    TestValidDataForMapType(FieldDescriptor::TYPE_SFIXED32,
                            FieldDescriptor::TYPE_SFIXED32);
    TestValidDataForMapType(FieldDescriptor::TYPE_SFIXED64,
                            FieldDescriptor::TYPE_SFIXED64);
    TestValidDataForMapType(FieldDescriptor::TYPE_INT32,
                            FieldDescriptor::TYPE_FLOAT);
    TestValidDataForMapType(FieldDescriptor::TYPE_INT32,
                            FieldDescriptor::TYPE_DOUBLE);
    TestValidDataForMapType(FieldDescriptor::TYPE_BOOL,
                            FieldDescriptor::TYPE_BOOL);
    TestValidDataForMapType(FieldDescriptor::TYPE_STRING,
                            FieldDescriptor::TYPE_STRING);
    TestValidDataForMapType(FieldDescriptor::TYPE_STRING,
                            FieldDescriptor::TYPE_BYTES);
    TestValidDataForMapType(FieldDescriptor::TYPE_STRING,
                            FieldDescriptor::TYPE_ENUM);
    TestValidDataForMapType(FieldDescriptor::TYPE_STRING,
                            FieldDescriptor::TYPE_MESSAGE);
    // Additional test to check overwriting message value map.
    TestOverwriteMessageValueMap();

    TestValidDataForOneofType(FieldDescriptor::TYPE_UINT32);
    TestValidDataForOneofType(FieldDescriptor::TYPE_BOOL);
    TestValidDataForOneofType(FieldDescriptor::TYPE_UINT64);
    TestValidDataForOneofType(FieldDescriptor::TYPE_FLOAT);
    TestValidDataForOneofType(FieldDescriptor::TYPE_DOUBLE);
    TestValidDataForOneofType(FieldDescriptor::TYPE_STRING);
    TestValidDataForOneofType(FieldDescriptor::TYPE_BYTES);
    TestValidDataForOneofType(FieldDescriptor::TYPE_ENUM);
    TestValidDataForOneofType(FieldDescriptor::TYPE_MESSAGE);
    // Additional test to check merging oneof message.
    TestMergeOneofMessage();

    // TODO(haberman):
    // TestValidDataForType(FieldDescriptor::TYPE_GROUP

    // Unknown fields.
    {
      TestAllTypesProto3 messageProto3;
      TestAllTypesProto2 messageProto2;
      // TODO(yilunchong): update this behavior when unknown field's behavior
      // changed in open source. Also delete
      // Required.Proto3.ProtobufInput.UnknownVarint.ProtobufOutput
      // from failure list of python_cpp python java
      TestUnknownMessage(messageProto3, true);
      TestUnknownMessage(messageProto2, false);
    }

    //    RunJsonTests();
  }
  // Flag control performance tests to keep them internal and opt-in only
  if (performance_) {
    RunBinaryPerformanceTests();
    //    RunJsonPerformanceTests();
  }
}

void BinaryAndJsonConformanceSuite::RunBinaryPerformanceTests() {
  TestBinaryPerformanceForAlternatingUnknownFields();

  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_BOOL);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_DOUBLE);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_FLOAT);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_UINT32);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_UINT64);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_STRING);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_BYTES);

  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_BOOL);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_DOUBLE);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_FLOAT);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_UINT32);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_UINT64);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_STRING);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_BYTES);
}

// This is currently considered valid input by some languages but not others
void BinaryAndJsonConformanceSuite::
    TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
        FieldDescriptor::Type type, string field_value) {
  const string type_name =
      UpperCase(string(".") + FieldDescriptor::TypeName(type));
  for (int is_proto3 = 0; is_proto3 < 2; is_proto3++) {
    const FieldDescriptor* field =
        GetFieldForType(type, true, is_proto3, Packed::kFalse);
    string field_name = field->name();

    string message_field = "\"" + field_name + "\": [" + field_value + "]";
    string recursive_message =
        "\"recursive_message\": { " + message_field + "}";
    string input = "{";
    input.append(recursive_message);
    for (size_t i = 1; i < kPerformanceRepeatCount; i++) {
      input.append("," + recursive_message);
    }
    input.append("}");

    string textproto_message_field = field_name + ": " + field_value;
    string expected_textproto = "recursive_message { ";
    for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
      expected_textproto.append(textproto_message_field + " ");
    }
    expected_textproto.append("}");
    RunValidJsonTest(
        "TestJsonPerformanceMergeMessageWithRepeatedFieldForType" + type_name,
        RECOMMENDED, input, expected_textproto, is_proto3);
  }
}

}  // namespace protobuf
}  // namespace google
