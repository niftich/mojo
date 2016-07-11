// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_UNION_H_
#define MOJO_PUBLIC_C_BINDINGS_UNION_H_

#include <stddef.h>
#include <stdint.h>

#include "mojo/public/c/bindings/lib/type_descriptor.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

struct MojomUnionLayout {
  // |num_bytes|, the number of bytes used in this union, includes the size of
  // the |num_bytes| and |tag| fields, along with data.
  uint32_t size;
  uint32_t tag;
  union {
    // To be used when the data is a pointer.
    union MojomPointer pointer;

    // This is here to force this union data to be at least 8 bytes.
    uint64_t force_size_;
  } data;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomUnionLayout) == 16,
                   "struct MojomUnionLayout must be 16 bytes.");

// Returns the number of bytes required to serialize |in_union_data|'s active
// field, not including the union layout (as described by |struct
// MojomUnionLayout|) itself.
// |in_type_desc| is the generated type that describes the tags of the pointer
// and handle types in the given union.
size_t MojomUnion_ComputeSerializedSize(
    const struct MojomTypeDescriptorUnion* in_type_desc,
    const struct MojomUnionLayout* in_union_data);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_UNION_H_
