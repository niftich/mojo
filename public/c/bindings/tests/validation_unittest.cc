// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(vardhan): There is some duplicate code here borrowed from
// mojo/public/cpp/bindings/tests/validation_unittest.cc; extract it into a
// library, and share?

#include <stdio.h>
#include <functional>
#include <string>

#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/bindings/message.h"
#include "mojo/public/c/bindings/struct.h"
#include "mojo/public/cpp/bindings/tests/validation_util.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/test_support/test_support.h"
#include "mojo/public/interfaces/bindings/tests/validation_test_interfaces.mojom-c.h"
#include "third_party/gtest/include/gtest/gtest.h"

using mojo::test::EnumerateSourceRootRelativeDirectory;

namespace mojo {
namespace test {
namespace {

const char* MojomValidationResultToString(MojomValidationResult error) {
  switch (error) {
    case MOJOM_VALIDATION_ERROR_NONE:
      return "PASS";
    case MOJOM_VALIDATION_MISALIGNED_OBJECT:
      return "VALIDATION_ERROR_MISALIGNED_OBJECT";
    case MOJOM_VALIDATION_ILLEGAL_MEMORY_RANGE:
      return "VALIDATION_ERROR_ILLEGAL_MEMORY_RANGE";
    case MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER:
      return "VALIDATION_ERROR_UNEXPECTED_STRUCT_HEADER";
    case MOJOM_VALIDATION_UNEXPECTED_ARRAY_HEADER:
      return "VALIDATION_ERROR_UNEXPECTED_ARRAY_HEADER";
    case MOJOM_VALIDATION_ILLEGAL_HANDLE:
      return "VALIDATION_ERROR_ILLEGAL_HANDLE";
    case MOJOM_VALIDATION_UNEXPECTED_INVALID_HANDLE:
      return "VALIDATION_ERROR_UNEXPECTED_INVALID_HANDLE";
    case MOJOM_VALIDATION_ILLEGAL_POINTER:
      return "VALIDATION_ERROR_ILLEGAL_POINTER";
    case MOJOM_VALIDATION_UNEXPECTED_NULL_POINTER:
      return "VALIDATION_ERROR_UNEXPECTED_NULL_POINTER";
    case MOJOM_VALIDATION_MESSAGE_HEADER_INVALID_FLAGS:
      return "VALIDATION_ERROR_MESSAGE_HEADER_INVALID_FLAGS";
    case MOJOM_VALIDATION_MESSAGE_HEADER_MISSING_REQUEST_ID:
      return "VALIDATION_ERROR_MESSAGE_HEADER_MISSING_REQUEST_ID";
    case MOJOM_VALIDATION_MESSAGE_HEADER_UNKNOWN_METHOD:
      return "VALIDATION_ERROR_MESSAGE_HEADER_UNKNOWN_METHOD";
    case MOJOM_VALIDATION_DIFFERENT_SIZED_ARRAYS_IN_MAP:
      return "VALIDATION_ERROR_DIFFERENT_SIZED_ARRAYS_IN_MAP";
    case MOJOM_VALIDATION_UNEXPECTED_NULL_UNION:
      return "VALIDATION_ERROR_UNEXPECTED_NULL_UNION";
  }

  return "Unknown error";
}

void RunValidationTests(
    const std::string& prefix,
    std::function<MojomValidationResult(const std::vector<uint8_t>&, size_t)>
        validate_fn) {
  std::vector<std::string> tests = validation_util::GetMatchingTests(prefix);
  for (size_t i = 0; i < tests.size(); ++i) {
    std::vector<uint8_t> message_data;
    size_t num_handles;
    std::string expected;
    ASSERT_TRUE(validation_util::ReadTestCase(tests[i], &message_data,
                                              &num_handles, &expected));
    // Validate this message. Should be PASS or ValidationErrorToString()
    MojomValidationResult result = validate_fn(message_data, num_handles);
    EXPECT_EQ(expected, MojomValidationResultToString(result)) << tests[i];
  }
}

// Emits a case (as part of a switch{} block) for validating a request for the
// given method, and sets the |result|.
// TODO(vardhan): Should this be code generated? Seems like everyone will have
// to do this.
#define CASE_INTERFACE_METHOD_REQUEST(method_struct, data, data_size,        \
                                      num_handles, result, expects_response) \
  case method_struct##__Ordinal: {                                           \
    (result) = expects_response                                              \
                   ? MojomMessage_ValidateRequestExpectingResponse(data)     \
                   : MojomMessage_ValidateRequestWithoutResponse(data);      \
    if ((result) == MOJOM_VALIDATION_ERROR_NONE) {                           \
      const struct MojomMessage* msg = (const struct MojomMessage*)(data);   \
      (result) = method_struct##_Request_Validate(                           \
          (const struct method_struct##_Request*)((char*)data +              \
                                                  msg->header.num_bytes),    \
          data_size - msg->header.num_bytes, (num_handles));                 \
    }                                                                        \
    break;                                                                   \
  }

#define CASE_INTERFACE_METHOD_RESPONSE(method_struct, data, data_size,     \
                                       num_handles, result)                \
  case method_struct##__Ordinal: {                                         \
    (result) = MojomMessage_ValidateResponse(data);                        \
    if ((result) == MOJOM_VALIDATION_ERROR_NONE) {                         \
      const struct MojomMessage* msg = (const struct MojomMessage*)(data); \
      (result) = method_struct##_Response_Validate(                        \
          (const struct method_struct##_Response*)((char*)data +           \
                                                   msg->header.num_bytes), \
          data_size - msg->header.num_bytes, (num_handles));               \
    }                                                                      \
    break;                                                                 \
  }
MojomValidationResult DispatchConformanceTestInterface_Request_Validate(
    const std::vector<uint8_t>& data,
    size_t num_handles) {
  const struct MojomMessage* msg = (const struct MojomMessage*)data.data();
  const char* msg_data = (const char*)data.data();
  uint32_t msg_size = data.size();

  MojomValidationResult result =
      MojomMessage_ValidateHeader(msg_data, msg_size);
  if (result != MOJOM_VALIDATION_ERROR_NONE)
    return result;
  switch (msg->ordinal) {
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method0,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method1,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method2,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method3,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method4,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method5,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method6,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method7,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method8,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method9,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method10,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method11,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method12,
                                  msg_data, msg_size, num_handles, result, true)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method13,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method14,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_ConformanceTestInterface_Method15,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    default:
      result = MOJOM_VALIDATION_MESSAGE_HEADER_UNKNOWN_METHOD;
      break;
  }
  return result;
}

MojomValidationResult DispatchConformanceTestInterface_Response_Validate(
    const std::vector<uint8_t>& data,
    size_t num_handles) {
  const struct MojomMessage* msg = (const struct MojomMessage*)data.data();
  const char* msg_data = (const char*)data.data();
  uint32_t msg_size = data.size();

  MojomValidationResult result =
      MojomMessage_ValidateHeader(msg_data, msg_size);
  if (result != MOJOM_VALIDATION_ERROR_NONE)
    return result;
  switch (msg->ordinal) {
    CASE_INTERFACE_METHOD_RESPONSE(mojo_test_ConformanceTestInterface_Method12,
                                   msg_data, msg_size, num_handles, result)
    default:
      result = MOJOM_VALIDATION_MESSAGE_HEADER_UNKNOWN_METHOD;
      break;
  }
  return result;
}

MojomValidationResult DispatchBoundsCheckTestInterface_Request_Validate(
    const std::vector<uint8_t>& data,
    size_t num_handles) {
  const struct MojomMessage* msg = (const struct MojomMessage*)data.data();
  const char* msg_data = (const char*)data.data();
  uint32_t msg_size = data.size();

  MojomValidationResult result =
      MojomMessage_ValidateHeader(msg_data, msg_size);
  if (result != MOJOM_VALIDATION_ERROR_NONE)
    return result;
  switch (msg->ordinal) {
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_BoundsCheckTestInterface_Method0,
                                  msg_data, msg_size, num_handles, result, true)
    CASE_INTERFACE_METHOD_REQUEST(mojo_test_BoundsCheckTestInterface_Method1,
                                  msg_data, msg_size, num_handles, result,
                                  false)
    default:
      result = MOJOM_VALIDATION_MESSAGE_HEADER_UNKNOWN_METHOD;
      break;
  }
  return result;
}

MojomValidationResult DispatchBoundsCheckTestInterface_Response_Validate(
    const std::vector<uint8_t>& data,
    size_t num_handles) {
  const struct MojomMessage* msg = (const struct MojomMessage*)data.data();
  const char* msg_data = (const char*)data.data();
  uint32_t msg_size = data.size();

  MojomValidationResult result =
      MojomMessage_ValidateHeader(msg_data, msg_size);
  if (result != MOJOM_VALIDATION_ERROR_NONE)
    return result;
  switch (msg->ordinal) {
    CASE_INTERFACE_METHOD_RESPONSE(mojo_test_BoundsCheckTestInterface_Method0,
                                   msg_data, msg_size, num_handles, result)
    default:
      result = MOJOM_VALIDATION_MESSAGE_HEADER_UNKNOWN_METHOD;
      break;
  }
  return result;
}

TEST(ValidationTest, Conformance) {
  RunValidationTests("conformance_",
                     DispatchConformanceTestInterface_Request_Validate);
}

TEST(ValidationTest, ResponseConformance) {
  RunValidationTests("resp_conformance_",
                     DispatchConformanceTestInterface_Response_Validate);
}

TEST(ValidationTest, BoundsCheck) {
  RunValidationTests("boundscheck_",
                     DispatchBoundsCheckTestInterface_Request_Validate);
}

TEST(ValidationTest, ResponseBoundsCheck) {
  RunValidationTests("resp_boundscheck_",
                     DispatchBoundsCheckTestInterface_Response_Validate);
}

// TODO(vardhan): Tests for "integration_" files.

}  // namespace
}  // namespace test
}  // namespace mojo
