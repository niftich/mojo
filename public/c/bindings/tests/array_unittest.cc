// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(vardhan): Needs a lot more testing.

#include "mojo/public/c/bindings/array.h"

#include <stddef.h>

#include "mojo/public/c/bindings/struct.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom-c.h"
#include "mojo/public/interfaces/bindings/tests/test_unions.mojom-c.h"
#include "third_party/gtest/include/gtest/gtest.h"

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
TEST(ArraySerializationTest, ArrayOfUnions) {
  char bytes_buffer[1000] = {0};
  MojomBuffer buf = {bytes_buffer, sizeof(bytes_buffer), 0};

  struct mojo_test_SmallStruct* small_struct =
      static_cast<struct mojo_test_SmallStruct*>(
          MojomBuffer_Allocate(&buf, sizeof(struct mojo_test_SmallStruct)));
  *small_struct = mojo_test_SmallStruct{
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
            mojo_test_SmallStruct_ComputeSerializedSize(small_struct));

  small_struct->nullable_pod_union_array.ptr =
      MojomArray_New(&buf, 2, sizeof(struct mojo_test_PodUnion));

  // 0th element is NULL.
  MOJOM_ARRAY_INDEX(small_struct->nullable_pod_union_array.ptr,
                    struct mojo_test_PodUnion, 0)
      ->size = 0;
  // 1st element is not NULL.
  struct mojo_test_PodUnion* e1 = MOJOM_ARRAY_INDEX(
      small_struct->nullable_pod_union_array.ptr, struct mojo_test_PodUnion, 0);
  e1->size = 16;
  e1->tag = mojo_test_PodUnion_Tag_f_int8;
  e1->data.f_f_int8 = 13;

  EXPECT_EQ(8u + 6 * 8u            // 6 references types
                + 1 * 16u          // 1 union type
                + (8u + 16u * 2),  // array of 2 unions
            mojo_test_SmallStruct_ComputeSerializedSize(small_struct));

  // Save the underlying buffer before encoding, so we can decode+compare
  // later.
  char bytes_buffer_copy[sizeof(bytes_buffer)];
  memcpy(bytes_buffer_copy, bytes_buffer, sizeof(bytes_buffer));

  struct MojomHandleBuffer handle_buf = {NULL, 0u, 0u};
  mojo_test_SmallStruct_EncodePointersAndHandles(
      small_struct, buf.num_bytes_used, &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);

  // The null pointers should now be 0-offsets:
  EXPECT_EQ(0u, small_struct->dummy_struct.offset);
  EXPECT_EQ(0u, small_struct->pod_union_array.offset);
  EXPECT_EQ(0u, small_struct->s_array.offset);
  EXPECT_EQ(0u, small_struct->pod_union_map.offset);
  EXPECT_EQ(0u, small_struct->nullable_pod_union_map.offset);

  // Test the pod_union_array offset:
  EXPECT_EQ(
      sizeof(struct mojo_test_SmallStruct) -
          offsetof(struct mojo_test_SmallStruct, nullable_pod_union_array),
      small_struct->nullable_pod_union_array.offset);

  mojo_test_SmallStruct_DecodePointersAndHandles(small_struct,
                                                 buf.num_bytes_used, NULL, 0);
  EXPECT_EQ(0, memcmp(buf.buf, bytes_buffer_copy, buf.num_bytes_used));
}

// Tests serialized size of an array of arrays.
TEST(ArraySerializationTest, ArrayOfArrays) {
  char bytes_buffer[1000] = {0};
  MojomBuffer buf = {bytes_buffer, sizeof(bytes_buffer), 0};

  struct mojo_test_ArrayOfArrays* arr =
      static_cast<struct mojo_test_ArrayOfArrays*>(
          MojomBuffer_Allocate(&buf, sizeof(struct mojo_test_ArrayOfArrays)));
  *arr = mojo_test_ArrayOfArrays{
      //  header
      {
          sizeof(struct mojo_test_ArrayOfArrays), 0,
      },
      {NULL},
      {NULL},
  };

  auto* a_array = MojomArray_New(&buf, 2, sizeof(union MojomArrayHeaderPtr));
  arr->a.ptr = a_array;
  MOJOM_ARRAY_INDEX(arr->a.ptr, union MojomArrayHeaderPtr, 0)->ptr = NULL;
  MOJOM_ARRAY_INDEX(arr->a.ptr, union MojomArrayHeaderPtr, 1)->ptr = NULL;

  EXPECT_EQ(24u + (8u + 2 * 8u)  // a (with 2 null arrays)
                + 0u,            // b (null altogether)
            mojo_test_ArrayOfArrays_ComputeSerializedSize(arr));

  // fill in |a| with array<int32> of size 2.
  struct MojomArrayHeader* array_int32 =
      MojomArray_New(&buf, 2, sizeof(int32_t));
  MOJOM_ARRAY_INDEX(arr->a.ptr, union MojomArrayHeaderPtr, 0)
      ->ptr = array_int32;
  *MOJOM_ARRAY_INDEX(array_int32, int32_t, 0) = 13;
  *MOJOM_ARRAY_INDEX(array_int32, int32_t, 1) = 13;

  EXPECT_EQ(24u + (8u + 2 * 8u)  // a (with 2 arrays, 1 of them NULL)
                + 0u             // b (null altogether)
                + (8u + 8u),     // first array<int> in a
            mojo_test_ArrayOfArrays_ComputeSerializedSize(arr));

  // Save the underlying buffer before encoding, so we can decode+compare
  // later.
  char bytes_buffer_copy[sizeof(bytes_buffer)];
  memcpy(bytes_buffer_copy, bytes_buffer, sizeof(bytes_buffer));

  struct MojomHandleBuffer handle_buf = {NULL, 0u, 0u};
  mojo_test_ArrayOfArrays_EncodePointersAndHandles(arr, buf.num_bytes_used,
                                                   &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);

  EXPECT_EQ(sizeof(struct mojo_test_ArrayOfArrays) -
                offsetof(struct mojo_test_ArrayOfArrays, a),
            arr->a.offset);

  // Array of int32 should occur at the end of the array of 2 arrays -- so the
  // offset is 2 pointer-sizes (1st one pointers to the array of int32, second
  // one is NULL).
  EXPECT_EQ(2 * sizeof(union MojomArrayHeaderPtr),
            MOJOM_ARRAY_INDEX(a_array, union MojomArrayHeaderPtr, 0)->offset);
  EXPECT_EQ(0u,
            MOJOM_ARRAY_INDEX(a_array, union MojomArrayHeaderPtr, 1)->offset);

  mojo_test_ArrayOfArrays_DecodePointersAndHandles(arr, buf.num_bytes_used,
                                                   NULL, 0);
  EXPECT_EQ(0, memcmp(buf.buf, bytes_buffer_copy, buf.num_bytes_used));
}

// Tests serialization of an array of handles.
TEST(ArraySerializationTest, ArrayOfHandles) {
  char buffer_bytes[1000] = {0};
  MojomBuffer buf = {buffer_bytes, sizeof(buffer_bytes), 0};

  struct mojo_test_StructWithNullableHandles* handle_struct =
      static_cast<struct mojo_test_StructWithNullableHandles*>(
          MojomBuffer_Allocate(
              &buf, sizeof(struct mojo_test_StructWithNullableHandles)));
  *handle_struct = mojo_test_StructWithNullableHandles{
      // header
      {sizeof(struct mojo_test_StructWithNullableHandles), 0},
      // These fields will be initialized below.
      MOJO_HANDLE_INVALID,
      {0},
      {NULL},
  };

  handle_struct->h = 10;
  auto* array_h = MojomArray_New(&buf, 3, sizeof(MojoHandle));
  handle_struct->array_h.ptr = array_h;
  *MOJOM_ARRAY_INDEX(handle_struct->array_h.ptr, MojoHandle, 0) = 20;
  *MOJOM_ARRAY_INDEX(handle_struct->array_h.ptr, MojoHandle, 1) =
      MOJO_HANDLE_INVALID;
  *MOJOM_ARRAY_INDEX(handle_struct->array_h.ptr, MojoHandle, 2) = 30;

  EXPECT_EQ(
      8u + 4u + 4u + 8u + 4u + (8u + 3 * 4u),
      mojo_test_StructWithNullableHandles_ComputeSerializedSize(handle_struct));

  // Save the underlying buffer before encoding, so we can decode+compare
  // later.
  char buffer_bytes_copy[sizeof(buffer_bytes)];
  memcpy(buffer_bytes_copy, buffer_bytes, sizeof(buffer_bytes));

  MojoHandle handles[5] = {};
  struct MojomHandleBuffer handle_buf = {handles, MOJO_ARRAYSIZE(handles), 0u};
  mojo_test_StructWithNullableHandles_EncodePointersAndHandles(
      handle_struct, buf.num_bytes_used, &handle_buf);
  EXPECT_EQ(3u, handle_buf.num_handles_used);
  EXPECT_EQ(static_cast<MojoHandle>(10u), handles[0]);
  EXPECT_EQ(static_cast<MojoHandle>(20u), handles[1]);
  EXPECT_EQ(static_cast<MojoHandle>(30u), handles[2]);

  EXPECT_EQ(0u, handle_struct->h);
  EXPECT_EQ(1u, *MOJOM_ARRAY_INDEX(array_h, MojoHandle, 0));
  EXPECT_EQ(static_cast<MojoHandle>(-1),
            *MOJOM_ARRAY_INDEX(array_h, MojoHandle, 1));
  EXPECT_EQ(2u, *MOJOM_ARRAY_INDEX(array_h, MojoHandle, 2));

  EXPECT_EQ(sizeof(struct mojo_test_StructWithNullableHandles) -
                offsetof(struct mojo_test_StructWithNullableHandles, array_h),
            handle_struct->array_h.offset);

  mojo_test_StructWithNullableHandles_DecodePointersAndHandles(
      handle_struct, buf.num_bytes_used, handles, MOJO_ARRAYSIZE(handles));
  EXPECT_EQ(0, memcmp(buf.buf, buffer_bytes_copy, buf.num_bytes_used));

  // Check that the handles in the handles array are invalidated:
  EXPECT_EQ(MOJO_HANDLE_INVALID, handles[0]);
  EXPECT_EQ(MOJO_HANDLE_INVALID, handles[1]);
  EXPECT_EQ(MOJO_HANDLE_INVALID, handles[2]);
}

}  // namespace
