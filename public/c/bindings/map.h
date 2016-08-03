// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_MAP_H_
#define MOJO_PUBLIC_C_BINDINGS_MAP_H_

#include <stdint.h>

#include "mojo/public/c/bindings/array.h"
#include "mojo/public/c/bindings/lib/type_descriptor.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/bindings/struct.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

// A mojom Map is a mojom struct with two MojomArrayHeaders: keys and values.
// - Number of elements in keys == number of elements values.
// - ith key corresponds to the ith value.
struct MojomMapHeader {
  struct MojomStructHeader header;
  union MojomArrayHeaderPtr keys;
  union MojomArrayHeaderPtr values;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomMapHeader) == 24,
                   "struct MojomMapHeader must be 24 bytes.");

union MojomMapHeaderPtr {
  struct MojomMapHeader* ptr;
  uint64_t offset;
};
MOJO_STATIC_ASSERT(sizeof(union MojomMapHeaderPtr) == 8,
                   "MojomMapHeaderPtr must be 8 bytes.");

MojomValidationResult MojomMap_Validate(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    const struct MojomStructHeader* in_struct,
    uint32_t in_buf_size,
    uint32_t in_num_handles,
    struct MojomValidationContext* inout_context);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_MAP_H_
