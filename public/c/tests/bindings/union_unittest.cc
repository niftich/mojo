// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(vardhan): Needs a lot more testing.

#include <mojo/bindings/union.h>

#include <mojo/bindings/array.h>

#include "mojo/public/c/tests/bindings/testing_util.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom-c.h"
#include "mojo/public/interfaces/bindings/tests/test_unions.mojom-c.h"
#include "third_party/gtest/include/gtest/gtest.h"

namespace {

struct mojo_test_StructOfUnionOfReferences* MakeStructOfUnionReference(
    struct MojomBuffer* buf) {
  auto* struct_with_union =
      static_cast<struct mojo_test_StructOfUnionOfReferences*>(
          MojomBuffer_Allocate(
              buf, sizeof(struct mojo_test_StructOfUnionOfReferences)));

  *struct_with_union = mojo_test_StructOfUnionOfReferences{
      // header
      {
          sizeof(struct mojo_test_StructOfUnionOfReferences), 0,
      },
      {
          0u, mojo_test_UnionOfReferences_Tag__UNKNOWN__, {},
      }  // null
  };
  return struct_with_union;
}

// Test serialization when a union points to a union.
TEST(UnionSerializationTest, UnionOfUnion) {
  char buffer_bytes[1000] = {0};
  struct MojomBuffer buf = {buffer_bytes, sizeof(buffer_bytes), 0};
  struct mojo_test_StructOfUnionOfReferences* struct_with_union =
      MakeStructOfUnionReference(&buf);

  EXPECT_EQ(8u + 1 * 16u,  // 1 union type (set to null)
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                struct_with_union));

  struct_with_union->u.size = 16u;
  EXPECT_EQ(8u + 1 * 16u,  // 1 union type (not null, but unknown)
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                struct_with_union));

  // Test when a union points to a union.
  struct mojo_test_PodUnion* pod_union =
      static_cast<struct mojo_test_PodUnion*>(
          MojomBuffer_Allocate(&buf, sizeof(struct mojo_test_PodUnion)));
  *pod_union = mojo_test_PodUnion{
      16u, mojo_test_PodUnion_Tag_f_int8, {13},
  };
  struct_with_union->u.tag = mojo_test_UnionOfReferences_Tag_pod_union;
  struct_with_union->u.data.f_pod_union.ptr = pod_union;
  EXPECT_EQ(8u + 1 * 16u  // 1 union type (set to PodUnion)
                + 16u,    // PodUnion is out of line.
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                struct_with_union));

  // We save the underlying (unencoded) buffer. We can compare the two after
  // deserialization to make sure deserialization is correct.
  char buffer_bytes_copy[sizeof(buffer_bytes)];
  memcpy(buffer_bytes_copy, buffer_bytes, sizeof(buffer_bytes_copy));

  mojo_test_StructOfUnionOfReferences_EncodePointersAndHandles(
      struct_with_union, buf.num_bytes_used, NULL);

  EXPECT_EQ(sizeof(struct mojo_test_StructOfUnionOfReferences) -
                offsetof(struct mojo_test_StructOfUnionOfReferences, u.data),
            struct_with_union->u.data.f_pod_union.offset);

  mojo_test_StructOfUnionOfReferences_DecodePointersAndHandles(
      struct_with_union, buf.num_bytes_used, NULL, 0);
  EXPECT_EQ(0, memcmp(buf.buf, buffer_bytes_copy, buf.num_bytes_used));

  {
    char buffer_bytes2[sizeof(buffer_bytes)] = {0};
    struct MojomBuffer buf2 = {buffer_bytes2, sizeof(buffer_bytes2), 0};
    CopyAndCompare(
        &buf2, struct_with_union, buf.num_bytes_used,
        mojo_test_StructOfUnionOfReferences_DeepCopy,
        mojo_test_StructOfUnionOfReferences_EncodePointersAndHandles,
        mojo_test_StructOfUnionOfReferences_DecodePointersAndHandles);
  }
}

// Test when a union points to a struct.
TEST(UnionSerializationTest, UnionOfStruct) {
  char buffer_bytes[1000];
  MojomBuffer buf = {buffer_bytes, sizeof(buffer_bytes), 0};

  struct mojo_test_StructOfUnionOfReferences* struct_with_union =
      MakeStructOfUnionReference(&buf);

  struct mojo_test_DummyStruct* dummy_struct =
      static_cast<struct mojo_test_DummyStruct*>(
          MojomBuffer_Allocate(&buf, sizeof(struct mojo_test_DummyStruct)));
  *dummy_struct = mojo_test_DummyStruct{
      // header
      {
          sizeof(struct mojo_test_DummyStruct),
          0u  // version
      },
      13,  // int8
      {},  // padding
  };
  struct_with_union->u.size = 16u;
  struct_with_union->u.tag = mojo_test_UnionOfReferences_Tag_dummy_struct;
  struct_with_union->u.data.f_dummy_struct.ptr = dummy_struct;

  EXPECT_EQ(8u + 1 * 16u  // 1 union type (set to DummyStruct)
                + 16u,    // DummyStruct is out of line.
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                struct_with_union));

  // We save the underlying (unencoded) buffer. We can compare the two after
  // deserialization to make sure deserialization is correct.
  char buffer_bytes_copy[sizeof(buffer_bytes)];
  memcpy(buffer_bytes_copy, buffer_bytes, sizeof(buffer_bytes_copy));

  mojo_test_StructOfUnionOfReferences_EncodePointersAndHandles(
      struct_with_union, buf.num_bytes_used, NULL);

  EXPECT_EQ(sizeof(struct mojo_test_StructOfUnionOfReferences) -
                offsetof(struct mojo_test_StructOfUnionOfReferences, u.data),
            struct_with_union->u.data.f_dummy_struct.offset);

  mojo_test_StructOfUnionOfReferences_DecodePointersAndHandles(
      struct_with_union, buf.num_bytes_used, NULL, 0);
  EXPECT_EQ(0, memcmp(buf.buf, buffer_bytes_copy, buf.num_bytes_used));

  {
    char buffer_bytes2[sizeof(buffer_bytes)] = {0};
    struct MojomBuffer buf2 = {buffer_bytes2, sizeof(buffer_bytes2), 0};
    CopyAndCompare(
        &buf2, struct_with_union, buf.num_bytes_used,
        mojo_test_StructOfUnionOfReferences_DeepCopy,
        mojo_test_StructOfUnionOfReferences_EncodePointersAndHandles,
        mojo_test_StructOfUnionOfReferences_DecodePointersAndHandles);
  }
}

// Test when a union points to an array of int32
TEST(UnionSerializationTest, UnionOfArray) {
  char buffer_bytes[1000];
  MojomBuffer buf = {buffer_bytes, sizeof(buffer_bytes), 0};
  struct mojo_test_StructOfUnionOfReferences* struct_with_union =
      MakeStructOfUnionReference(&buf);

  struct MojomArrayHeader* array_of_int32 =
      MojomArray_New(&buf, 5, sizeof(int32_t));
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 0) = 13;
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 1) = 13;
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 2) = 13;
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 3) = 13;
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 4) = 13;
  struct_with_union->u.size = 16u;
  struct_with_union->u.tag = mojo_test_UnionOfReferences_Tag_int_array;
  struct_with_union->u.data.f_int_array.ptr = array_of_int32;

  EXPECT_EQ(8u + 1 * 16u        // 1 union type (set to int array)
                + (8 + 4u * 5)  // mojom array of 5 int32s
                + 4u,           // padding for the mojom array
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                struct_with_union));

  // We save the underlying (unencoded) buffer. We can compare the two after
  // deserialization to make sure deserialization is correct.
  char buffer_bytes_copy[sizeof(buffer_bytes)];
  memcpy(buffer_bytes_copy, buffer_bytes, sizeof(buffer_bytes_copy));

  struct MojomHandleBuffer handle_buf = {NULL, 0u, 0u};
  mojo_test_StructOfUnionOfReferences_EncodePointersAndHandles(
      struct_with_union, buf.num_bytes_used, &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);

  EXPECT_EQ(sizeof(struct mojo_test_StructOfUnionOfReferences) -
                offsetof(struct mojo_test_StructOfUnionOfReferences, u.data),
            struct_with_union->u.data.f_int_array.offset);

  mojo_test_StructOfUnionOfReferences_DecodePointersAndHandles(
      struct_with_union, buf.num_bytes_used, NULL, 0);
  EXPECT_EQ(0, memcmp(buf.buf, buffer_bytes_copy, buf.num_bytes_used));

  {
    char buffer_bytes2[sizeof(buffer_bytes)] = {0};
    struct MojomBuffer buf2 = {buffer_bytes2, sizeof(buffer_bytes2), 0};
    CopyAndCompare(
        &buf2, struct_with_union, buf.num_bytes_used,
        mojo_test_StructOfUnionOfReferences_DeepCopy,
        mojo_test_StructOfUnionOfReferences_EncodePointersAndHandles,
        mojo_test_StructOfUnionOfReferences_DecodePointersAndHandles);
  }
}

}  // namespace
