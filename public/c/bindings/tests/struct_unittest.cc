// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(vardhan): Needs a lot more testing.

#include "mojo/public/c/bindings/struct.h"

#include <string.h>

#include "mojo/public/c/bindings/array.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/interfaces/bindings/tests/rect.mojom-c.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom-c.h"
#include "mojo/public/interfaces/bindings/tests/test_unions.mojom-c.h"
#include "third_party/gtest/include/gtest/gtest.h"

namespace {

#define BYTES_LEFT_AFTER_FIELD(type, field) \
  (sizeof(type) - offsetof(type, field))

struct mojo_test_Rect* MakeRect(struct MojomBuffer* buf) {
  struct mojo_test_Rect* r = static_cast<struct mojo_test_Rect*>(
      MojomBuffer_Allocate(buf, sizeof(struct mojo_test_Rect)));
  *r = mojo_test_Rect{// header
                      {
                          sizeof(struct mojo_test_Rect), 0,
                      },
                      0u,
                      0u,
                      0u,
                      0u};
  return r;
}

// TODO(vardhan): Move this into string.h/c if it proves useful again.
struct MojomStringHeader* MakeMojomStringFromCString(MojomBuffer* buf,
                                                     const char* chars,
                                                     size_t num_chars) {
  struct MojomArrayHeader* arr = MojomArray_New(buf, num_chars, 1);
  assert(NULL != arr);

  memcpy((char*)arr + sizeof(MojomStringHeader), chars, num_chars);
  return (struct MojomStringHeader*)arr;
}

TEST(StructSerializedSizeTest, Basic) {
  char buffer_bytes[1000];
  struct MojomBuffer buf = {buffer_bytes, sizeof(buffer_bytes), 0};
  struct mojo_test_Rect* rect = MakeRect(&buf);

  size_t size = mojo_test_Rect_ComputeSerializedSize(rect);
  EXPECT_EQ(8U + 16U, size);
}

TEST(StructSerializationTest, StructOfStructs) {
  char buffer_bytes[1000] = {0};
  struct MojomBuffer buf = {buffer_bytes, sizeof(buffer_bytes), 0};

  struct mojo_test_RectPair* pair = static_cast<struct mojo_test_RectPair*>(
      MojomBuffer_Allocate(&buf, sizeof(struct mojo_test_RectPair)));
  *pair = mojo_test_RectPair{
      {sizeof(struct mojo_test_RectPair), 0},
      {MakeRect(&buf)},  // first
      {MakeRect(&buf)},  // second
  };

  EXPECT_EQ(8U + 16U + 2 * (8U + 16U),
            mojo_test_RectPair_ComputeSerializedSize(pair));

  // We save the underlying (unencoded) buffer. We can compare the two after
  // deserialization to make sure deserialization is correct.
  char buffer_bytes_copy[sizeof(buffer_bytes)];
  memcpy(buffer_bytes_copy, buffer_bytes, sizeof(buffer_bytes_copy));

  struct MojomHandleBuffer handle_buf = {NULL, 0u, 0u};
  mojo_test_RectPair_EncodePointersAndHandles(pair, buf.num_bytes_used,
                                              &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);

  EXPECT_EQ(BYTES_LEFT_AFTER_FIELD(struct mojo_test_RectPair, first),
            pair->first.offset);
  EXPECT_EQ(BYTES_LEFT_AFTER_FIELD(struct mojo_test_RectPair, second) +
                sizeof(struct mojo_test_Rect),
            pair->second.offset);

  mojo_test_RectPair_DecodePointersAndHandles(
      reinterpret_cast<struct mojo_test_RectPair*>(buf.buf), buf.num_bytes_used,
      NULL, 0);
  EXPECT_EQ(0, memcmp(buf.buf, buffer_bytes_copy, buf.num_bytes_used));
}

// Tests a struct that has:
//  - nullable string which isn't null.
//  - nullable array of rects, which isn't null.
TEST(StructSerializationTest, StructOfArrays) {
  char buffer_bytes[1000];
  MojomBuffer buf = {buffer_bytes, sizeof(buffer_bytes), 0};

  const char* kRegionName = "region";

  struct mojo_test_NamedRegion* named_region =
      static_cast<struct mojo_test_NamedRegion*>(
          MojomBuffer_Allocate(&buf, sizeof(struct mojo_test_NamedRegion)));
  *named_region = mojo_test_NamedRegion{
      // header
      {
          sizeof(struct mojo_test_NamedRegion), 0,
      },
      {NULL},
      {NULL},
  };
  named_region->name.ptr =
      MakeMojomStringFromCString(&buf, kRegionName, strlen(kRegionName));
  // We save a pointer to |rects| so we can use it after it has been encoded
  // within |named_region|.
  auto rects = MojomArray_New(&buf, 4, sizeof(union mojo_test_RectPtr));
  named_region->rects.ptr = rects;
  ASSERT_TRUE(named_region->rects.ptr != NULL);

  MOJOM_ARRAY_INDEX(named_region->rects.ptr, union mojo_test_RectPtr, 0)
      ->ptr = MakeRect(&buf);
  MOJOM_ARRAY_INDEX(named_region->rects.ptr, union mojo_test_RectPtr, 1)
      ->ptr = MakeRect(&buf);
  MOJOM_ARRAY_INDEX(named_region->rects.ptr, union mojo_test_RectPtr, 2)
      ->ptr = MakeRect(&buf);
  MOJOM_ARRAY_INDEX(named_region->rects.ptr, union mojo_test_RectPtr, 3)
      ->ptr = MakeRect(&buf);

  size_t size = mojo_test_NamedRegion_ComputeSerializedSize(named_region);
  EXPECT_EQ(8U +            // header
                8U +        // name pointer
                8U +        // rects pointer
                8U +        // name header
                8U +        // name payload (rounded up)
                8U +        // rects header
                4 * 8U +    // rects payload (four pointers)
                4 * (8U +   // rect header
                     16U),  // rect payload (four ints)
            size);

  char buffer_bytes_copy[sizeof(buffer_bytes)];
  memcpy(buffer_bytes_copy, buffer_bytes, sizeof(buffer_bytes_copy));

  struct MojomHandleBuffer handle_buf = {NULL, 0u, 0u};
  mojo_test_NamedRegion_EncodePointersAndHandles(
      named_region, buf.num_bytes_used, &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);

  EXPECT_EQ(BYTES_LEFT_AFTER_FIELD(struct mojo_test_NamedRegion, name),
            named_region->name.offset);
  EXPECT_EQ(BYTES_LEFT_AFTER_FIELD(struct mojo_test_NamedRegion, rects) +
                MOJOM_INTERNAL_ROUND_TO_8(sizeof(struct MojomStringHeader) +
                                          strlen(kRegionName)),
            named_region->rects.offset);

  // Test the offsets encoded inside the rect array.
  EXPECT_EQ(sizeof(union mojo_test_RectPtr) * 4,
            MOJOM_ARRAY_INDEX(rects, union mojo_test_RectPtr, 0)->offset);
  EXPECT_EQ(sizeof(union mojo_test_RectPtr) * 3 + sizeof(struct mojo_test_Rect),
            MOJOM_ARRAY_INDEX(rects, union mojo_test_RectPtr, 1)->offset);
  EXPECT_EQ(
      sizeof(union mojo_test_RectPtr) * 2 + sizeof(struct mojo_test_Rect) * 2,
      MOJOM_ARRAY_INDEX(rects, union mojo_test_RectPtr, 2)->offset);
  EXPECT_EQ(sizeof(union mojo_test_RectPtr) + sizeof(struct mojo_test_Rect) * 3,
            MOJOM_ARRAY_INDEX(rects, union mojo_test_RectPtr, 3)->offset);

  mojo_test_NamedRegion_DecodePointersAndHandles(
      reinterpret_cast<struct mojo_test_NamedRegion*>(buf.buf),
      buf.num_bytes_used, NULL, 0);
  EXPECT_EQ(0, memcmp(buf.buf, buffer_bytes_copy, buf.num_bytes_used));
}

// Tests a struct that has:
//  - nullable string which is null.
//  - nullable array of rects, which is null.
TEST(StructSerializationTest, StructOfNullArrays) {
  struct mojo_test_NamedRegion named_region = {
      // header
      {
          sizeof(struct mojo_test_NamedRegion), 0,
      },
      {NULL},
      {NULL},
  };
  struct mojo_test_NamedRegion named_region_copy = named_region;

  size_t size = mojo_test_NamedRegion_ComputeSerializedSize(&named_region);
  EXPECT_EQ(8U +      // header
                8U +  // name pointer
                8U,   // rects pointer
            size);

  struct MojomHandleBuffer handle_buf = {NULL, 0u, 0u};
  mojo_test_NamedRegion_EncodePointersAndHandles(
      &named_region, sizeof(struct mojo_test_NamedRegion), &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);
  EXPECT_EQ(0u, named_region.name.offset);
  EXPECT_EQ(0u, named_region.rects.offset);

  mojo_test_NamedRegion_DecodePointersAndHandles(
      &named_region, sizeof(struct mojo_test_NamedRegion), NULL, 0);
  EXPECT_EQ(0, memcmp(&named_region, &named_region_copy,
                      sizeof(struct mojo_test_NamedRegion)));
}

TEST(StructSerializationTest, StructOfUnion) {
  struct mojo_test_SmallStructNonNullableUnion u = {
      // header
      {
          sizeof(struct mojo_test_SmallStructNonNullableUnion),
          0,  // version
      },
      // PodUnion
      {
          16ul,                           // size
          mojo_test_PodUnion_Tag_f_int8,  // tag,
          {0},                            // data
      },
  };

  struct mojo_test_SmallStructNonNullableUnion u_null = {
      // header
      {
          sizeof(struct mojo_test_SmallStructNonNullableUnion),
          0,  // version
      },
      // PodUnion
      {
          0ul,                                // size
          mojo_test_PodUnion_Tag__UNKNOWN__,  // tag,
          {0},                                // data
      },
  };

  EXPECT_EQ(8U +      // header
                16U,  // union
            mojo_test_SmallStructNonNullableUnion_ComputeSerializedSize(&u));
  EXPECT_EQ(
      8U +      // header
          16U,  // union
      mojo_test_SmallStructNonNullableUnion_ComputeSerializedSize(&u_null));

  // Encoding shouldn't have done anything to these structs (they have no
  // pointers or handles):
  struct MojomHandleBuffer handle_buf = {NULL, 0u, 0u};
  struct mojo_test_SmallStructNonNullableUnion u_copy = u;
  mojo_test_SmallStructNonNullableUnion_EncodePointersAndHandles(
      &u, sizeof(struct mojo_test_SmallStructNonNullableUnion), &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);
  EXPECT_EQ(0, memcmp(&u, &u_copy, sizeof(u)));

  // Similarly, decoding shouldn't change the struct at all:
  mojo_test_SmallStructNonNullableUnion_DecodePointersAndHandles(
      &u, sizeof(struct mojo_test_SmallStructNonNullableUnion), NULL, 0);
  EXPECT_EQ(0, memcmp(&u, &u_copy, sizeof(u)));

  struct mojo_test_SmallStructNonNullableUnion u_null_copy = u_null;
  mojo_test_SmallStructNonNullableUnion_EncodePointersAndHandles(
      &u_null, sizeof(struct mojo_test_SmallStructNonNullableUnion),
      &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);
  EXPECT_EQ(0, memcmp(&u_null, &u_null_copy, sizeof(u_null)));

  mojo_test_SmallStructNonNullableUnion_DecodePointersAndHandles(
      &u_null, sizeof(struct mojo_test_SmallStructNonNullableUnion), NULL, 0);
  EXPECT_EQ(0, memcmp(&u_null, &u_null_copy, sizeof(u_null)));
}

TEST(StructSerializationTest, StructWithHandle) {
  struct mojo_test_NullableHandleStruct handle_struct =
      mojo_test_NullableHandleStruct{
          // header
          {
              sizeof(struct mojo_test_NullableHandleStruct), 0,
          },
          MOJO_HANDLE_INVALID,
          13,
      };

  EXPECT_EQ(
      sizeof(struct mojo_test_NullableHandleStruct),
      mojo_test_NullableHandleStruct_ComputeSerializedSize(&handle_struct));

  MojoHandle handles[1];
  struct MojomHandleBuffer handle_buf = {handles, MOJO_ARRAYSIZE(handles), 0u};
  mojo_test_NullableHandleStruct_EncodePointersAndHandles(
      &handle_struct, sizeof(struct mojo_test_NullableHandleStruct),
      &handle_buf);
  EXPECT_EQ(0u, handle_buf.num_handles_used);
  EXPECT_EQ(static_cast<MojoHandle>(-1), handle_struct.h);

  mojo_test_NullableHandleStruct_DecodePointersAndHandles(
      &handle_struct, sizeof(struct mojo_test_NullableHandleStruct), handles,
      MOJO_ARRAYSIZE(handles));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_struct.h);

  handle_struct.h = 123;
  mojo_test_NullableHandleStruct_EncodePointersAndHandles(
      &handle_struct, sizeof(struct mojo_test_NullableHandleStruct),
      &handle_buf);
  EXPECT_EQ(1u, handle_buf.num_handles_used);
  EXPECT_EQ(static_cast<MojoHandle>(0), handle_struct.h);
  EXPECT_EQ(static_cast<MojoHandle>(123), handles[0]);

  mojo_test_NullableHandleStruct_DecodePointersAndHandles(
      &handle_struct, sizeof(struct mojo_test_NullableHandleStruct), handles,
      MOJO_ARRAYSIZE(handles));
  EXPECT_EQ(static_cast<MojoHandle>(123), handle_struct.h);
  EXPECT_EQ(MOJO_HANDLE_INVALID, handles[0]);
}

}  // namespace
