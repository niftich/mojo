// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(vardhan): Needs a lot more testing.

#include "mojo/public/c/bindings/union.h"

#include "mojo/public/c/bindings/array.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom-c.h"
#include "mojo/public/interfaces/bindings/tests/test_unions.mojom-c.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Tests serialized size of a union of reference types.
TEST(UnionSerializedSize, UnionOfReferences) {
  struct mojo_test_StructOfUnionOfReferences struct_with_union = {
      // header
      {
          sizeof(struct mojo_test_StructOfUnionOfReferences), 0,
      },
      {
          0u, mojo_test_UnionOfReferences_Tag__UNKNOWN__,
      }  // null
  };

  EXPECT_EQ(8u + 1 * 16u,  // 1 union type (set to null)
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                &struct_with_union));

  struct_with_union.u.size = 16u;
  EXPECT_EQ(8u + 1 * 16u,  // 1 union type (not null, but unknown)
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                &struct_with_union));

  // Test when a union points to a union.
  struct mojo_test_PodUnion pod_union = {
      16u, mojo_test_PodUnion_Tag_f_int8, {13},
  };
  struct_with_union.u.tag = mojo_test_UnionOfReferences_Tag_pod_union;
  struct_with_union.u.data.f_pod_union.ptr = &pod_union;
  EXPECT_EQ(8u + 1 * 16u  // 1 union type (set to PodUnion)
                + 16u,    // PodUnion is out of line.
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                &struct_with_union));

  // Test when a union points to a struct.
  struct mojo_test_DummyStruct dummy_struct = {
      // header
      {
          sizeof(struct mojo_test_DummyStruct),
          0u  // version
      },
      13,  // int8
  };
  struct_with_union.u.tag = mojo_test_UnionOfReferences_Tag_dummy_struct;
  struct_with_union.u.data.f_dummy_struct.ptr = &dummy_struct;

  EXPECT_EQ(8u + 1 * 16u  // 1 union type (set to DummyStruct)
                + 16u,    // DummyStruct is out of line.
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                &struct_with_union));

  // Test when a union points to an array of int32
  char bytes[1000];
  MojomBuffer buf = {bytes, sizeof(bytes), 0};
  struct MojomArrayHeader* array_of_int32 =
      MojomArray_New(&buf, 5, sizeof(int32_t));
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 0) = 13;
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 1) = 13;
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 2) = 13;
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 3) = 13;
  *MOJOM_ARRAY_INDEX(array_of_int32, int32_t, 4) = 13;
  struct_with_union.u.tag = mojo_test_UnionOfReferences_Tag_int_array;
  struct_with_union.u.data.f_int_array.ptr = array_of_int32;

  EXPECT_EQ(8u + 1 * 16u        // 1 union type (set to int array)
                + (8 + 4u * 5)  // mojom array of 5 int32s
                + 4u,           // padding for the mojom array
            mojo_test_StructOfUnionOfReferences_ComputeSerializedSize(
                &struct_with_union));
}

}  // namespace
