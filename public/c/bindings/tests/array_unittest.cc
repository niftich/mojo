// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(vardhan): Needs a lot more testing.

#include "mojo/public/c/bindings/array.h"

#include <stddef.h>

#include "mojo/public/c/bindings/struct.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom-c.h"
#include "mojo/public/interfaces/bindings/tests/test_unions.mojom-c.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Tests MojomArray_New().
TEST(ArrayTest, New) {
  char bytes_buffer[1000];
  struct MojomBuffer buf = {bytes_buffer, sizeof(bytes_buffer), 0};

  struct MojomArrayHeader* arr = NULL;
  arr = MojomArray_New(&buf, 3, sizeof(uint32_t));
  EXPECT_EQ(buf.buf, (char*)arr);

  // Test that things are rounded up if data is not already multiple-of-8.
  EXPECT_EQ(8u                          // array header
                + 3 * sizeof(uint32_t)  // space for 3 uint32_ts.
                + 4u,                   // padding to round up to 8 bytes.
            buf.num_bytes_used);

  EXPECT_EQ(buf.num_bytes_used, arr->num_bytes);
  EXPECT_EQ(3ul, arr->num_elements);

  // Test failure when we try to allocate too much.
  EXPECT_EQ(NULL, MojomArray_New(&buf, UINT32_MAX, sizeof(uint32_t)));
  EXPECT_EQ(NULL, MojomArray_New(&buf, 1000, sizeof(uint32_t)));

  // Test the simple case (no rounding necessary).
  buf.num_bytes_used = 0;
  arr = MojomArray_New(&buf, 4, sizeof(uint32_t));
  EXPECT_EQ(8u                           // array header
                + 4 * sizeof(uint32_t),  // space for 4 uint32_ts
            buf.num_bytes_used);
  EXPECT_EQ(buf.num_bytes_used, arr->num_bytes);
  EXPECT_EQ(4ul, arr->num_elements);
}

// Tests serialized size of an array of unions.
TEST(ArraySerializedSize, ArrayOfUnions) {
  struct mojo_test_SmallStruct small_struct = {
      // header
      {
          sizeof(mojo_test_SmallStruct),
          0,  // version
      },
      {NULL},                                      // dummy_struct
      {0, mojo_test_PodUnion_Tag__UNKNOWN__, {}},  // pod_union
      {NULL},                                      // pod_union_array
      {NULL},                                      // nullable_pod_union_array
      {NULL},                                      // s_array
      {NULL},                                      // pod_union_map
      {NULL},                                      // nullable_pod_union_map
  };

  EXPECT_EQ(8u + 6 * 8u     // 6 references types
                + 1 * 16u,  // 1 union type
            mojo_test_SmallStruct_ComputeSerializedSize(&small_struct));

  char bytes_buffer[1000];
  MojomBuffer buf = {bytes_buffer, sizeof(bytes_buffer), 0};
  small_struct.nullable_pod_union_array.ptr =
      MojomArray_New(&buf, 2, sizeof(struct mojo_test_PodUnion));

  // 0th element is NULL.
  MOJOM_ARRAY_INDEX(small_struct.nullable_pod_union_array.ptr,
                    struct mojo_test_PodUnion, 0)
      ->size = 0;
  // 1st element is not NULL.
  {
    struct mojo_test_PodUnion* e1 =
        MOJOM_ARRAY_INDEX(small_struct.nullable_pod_union_array.ptr,
                          struct mojo_test_PodUnion, 0);
    e1->size = 16;
    e1->tag = mojo_test_PodUnion_Tag_f_int8;
    e1->data.f_f_int8 = 13;
  }

  EXPECT_EQ(8u + 6 * 8u            // 6 references types
                + 1 * 16u          // 1 union type
                + (8u + 16u * 2),  // array of 2 unions
            mojo_test_SmallStruct_ComputeSerializedSize(&small_struct));
}

// Tests serialized size of an array of arrays.
TEST(ArraySerializedSize, ArrayOfArrays) {
  struct mojo_test_ArrayOfArrays arr = {
      //  header
      {
          sizeof(struct mojo_test_ArrayOfArrays), 0,
      },
      {NULL},
      {NULL},
  };

  char bytes[1000];
  MojomBuffer buf = {bytes, sizeof(bytes), 0};
  arr.a.ptr = MojomArray_New(&buf, 2, sizeof(union MojomArrayHeaderPtr));
  MOJOM_ARRAY_INDEX(arr.a.ptr, union MojomArrayHeaderPtr, 0)->ptr = NULL;
  MOJOM_ARRAY_INDEX(arr.a.ptr, union MojomArrayHeaderPtr, 1)->ptr = NULL;

  EXPECT_EQ(24u + (8u + 2 * 8u)  // a (with 2 null arrays)
                + 0u,            // b (null altogether)
            mojo_test_ArrayOfArrays_ComputeSerializedSize(&arr));

  // fill in |a| with array<int32> of size 2.
  struct MojomArrayHeader* array_int32 =
      MojomArray_New(&buf, 2, sizeof(int32_t));
  MOJOM_ARRAY_INDEX(arr.a.ptr, union MojomArrayHeaderPtr, 0)->ptr = array_int32;
  *MOJOM_ARRAY_INDEX(array_int32, int32_t, 0) = 13;
  *MOJOM_ARRAY_INDEX(array_int32, int32_t, 1) = 13;

  EXPECT_EQ(24u + (8u + 2 * 8u)  // a (with 2 arrays, 1 of them NULL)
                + 0u             // b (null altogether)
                + (8u + 8u),     // first array<int> in a
            mojo_test_ArrayOfArrays_ComputeSerializedSize(&arr));
}

}  // namespace
