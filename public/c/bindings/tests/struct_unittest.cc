// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(vardhan): Needs a lot more testing.

#include "mojo/public/c/bindings/struct.h"

#include <string.h>

#include "mojo/public/c/bindings/array.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/interfaces/bindings/tests/rect.mojom-c.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom-c.h"
#include "mojo/public/interfaces/bindings/tests/test_unions.mojom-c.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

struct mojo_test_Rect MakeRect() {
  return {
    // header
    {
      sizeof(struct mojo_test_Rect), 0,
    },
    0u,
    0u,
    0u,
    0u
  };
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

TEST(StructSerializedSize, Basic) {
  struct mojo_test_Rect rect = MakeRect();
  size_t size = mojo_test_Rect_ComputeSerializedSize(&rect);
  EXPECT_EQ(8U + 16U, size);
}

TEST(StructSerializedSize, StructOfStructs) {
  mojo_test_Rect first = MakeRect();
  mojo_test_Rect second = MakeRect();
  mojo_test_RectPair pair = {
      // header
      {
          sizeof(struct mojo_test_RectPair), 0,
      },
      {&first},
      {&second},
  };
  size_t size = mojo_test_RectPair_ComputeSerializedSize(&pair);
  EXPECT_EQ(8U + 16U + 2 * (8U + 16U), size);
}

// Tests a struct that has:
//  - nullable string which isn't null.
//  - nullable array of rects, which isn't null.
TEST(StructSerializedSize, StructOfArrays) {
  char buffer_bytes[1000];
  MojomBuffer buf = {buffer_bytes, sizeof(buffer_bytes), 0};

  const char* kRegionName = "region";

  struct mojo_test_NamedRegion named_region = {
      // header
      {
          sizeof(struct mojo_test_NamedRegion), 0,
      },
      {NULL},
      {NULL},
  };
  named_region.name.ptr =
      MakeMojomStringFromCString(&buf, kRegionName, strlen(kRegionName));
  named_region.rects.ptr =
      MojomArray_New(&buf, 4, sizeof(union mojo_test_RectPtr));
  ASSERT_TRUE(NULL != named_region.rects.ptr);

  mojo_test_Rect rect0 = MakeRect();
  mojo_test_Rect rect1 = MakeRect();
  mojo_test_Rect rect2 = MakeRect();
  mojo_test_Rect rect3 = MakeRect();

  MOJOM_ARRAY_INDEX(named_region.rects.ptr, union mojo_test_RectPtr, 0)
      ->ptr = &rect0;
  MOJOM_ARRAY_INDEX(named_region.rects.ptr, union mojo_test_RectPtr, 1)
      ->ptr = &rect1;
  MOJOM_ARRAY_INDEX(named_region.rects.ptr, union mojo_test_RectPtr, 2)
      ->ptr = &rect2;
  MOJOM_ARRAY_INDEX(named_region.rects.ptr, union mojo_test_RectPtr, 3)
      ->ptr = &rect3;

  size_t size = mojo_test_NamedRegion_ComputeSerializedSize(&named_region);
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
}

// Tests a struct that has:
//  - nullable string which is null.
//  - nullable array of rects, which is null.
TEST(StructSerializedSize, StructOfNullArrays) {
  struct mojo_test_NamedRegion named_region = {
      // header
      {
          sizeof(struct mojo_test_NamedRegion), 0,
      },
      {NULL},
      {NULL},
  };

  size_t size = mojo_test_NamedRegion_ComputeSerializedSize(&named_region);
  EXPECT_EQ(8U +      // header
                8U +  // name pointer
                8U,   // rects pointer
            size);
}

TEST(StructSerializedSize, StructOfUnion) {
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
}

}  // namespace
