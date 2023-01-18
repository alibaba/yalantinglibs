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

#ifndef CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H
#define CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H

#include "conformance_test.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {

class BinaryAndJsonConformanceSuite : public ConformanceTestSuite {
 public:
  BinaryAndJsonConformanceSuite() {}

 private:
  void RunSuiteImpl() override;
  void RunBinaryPerformanceTests();
  void RunValidJsonTest(const std::string& test_name, ConformanceLevel level,
                        const std::string& input_json,
                        const std::string& equivalent_text_format,
                        bool is_proto3);
  void RunValidProtobufTest(const std::string& test_name,
                            ConformanceLevel level,
                            const std::string& input_protobuf,
                            const std::string& equivalent_text_format,
                            bool is_proto3);
  void RunValidBinaryProtobufTest(const std::string& test_name,
                                  ConformanceLevel level,
                                  const std::string& input_protobuf,
                                  bool is_proto3);
  void RunValidBinaryProtobufTest(const std::string& test_name,
                                  ConformanceLevel level,
                                  const std::string& input_protobuf,
                                  const std::string& expected_protobuf,
                                  bool is_proto3);
  void RunBinaryPerformanceMergeMessageWithField(const std::string& test_name,
                                                 const std::string& field_proto,
                                                 bool is_proto3);

  void RunValidProtobufTestWithMessage(
      const std::string& test_name, ConformanceLevel level,
      const Message* input, const std::string& equivalent_text_format,
      bool is_proto3);

  bool ParseResponse(const conformance::ConformanceResponse& response,
                     const ConformanceRequestSetting& setting,
                     Message* test_message) override;
  void ExpectParseFailureForJson(const std::string& test_name,
                                 ConformanceLevel level,
                                 const std::string& input_json);
  void ExpectSerializeFailureForJson(const std::string& test_name,
                                     ConformanceLevel level,
                                     const std::string& text_format);
  void ExpectParseFailureForProtoWithProtoVersion(const std::string& proto,
                                                  const std::string& test_name,
                                                  ConformanceLevel level,
                                                  bool is_proto3);
  void ExpectParseFailureForProto(const std::string& proto,
                                  const std::string& test_name,
                                  ConformanceLevel level);
  void ExpectHardParseFailureForProto(const std::string& proto,
                                      const std::string& test_name,
                                      ConformanceLevel level);
  void TestPrematureEOFForType(google::protobuf::FieldDescriptor::Type type);
  void TestIllegalTags();
  template <class MessageType>
  void TestOneofMessage(MessageType& message, bool is_proto3);
  template <class MessageType>
  void TestUnknownMessage(MessageType& message, bool is_proto3);
  void TestValidDataForType(
      google::protobuf::FieldDescriptor::Type,
      std::vector<std::pair<std::string, std::string>> values);
  void TestValidDataForRepeatedScalarMessage();
  void TestValidDataForMapType(google::protobuf::FieldDescriptor::Type,
                               google::protobuf::FieldDescriptor::Type);
  void TestValidDataForOneofType(google::protobuf::FieldDescriptor::Type);
  void TestMergeOneofMessage();
  void TestOverwriteMessageValueMap();
  void TestBinaryPerformanceForAlternatingUnknownFields();
  void TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      google::protobuf::FieldDescriptor::Type);
  void TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      google::protobuf::FieldDescriptor::Type);
  void TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      google::protobuf::FieldDescriptor::Type, std::string field_value);

  std::unique_ptr<google::protobuf::util::TypeResolver> type_resolver_;
  std::string type_url_;
};

}  // namespace protobuf
}  // namespace google

#endif  // CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H
