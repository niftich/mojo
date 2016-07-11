// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_STRUCT_H_
#define MOJO_PUBLIC_C_BINDINGS_STRUCT_H_

#include <stddef.h>
#include <stdint.h>

#include "mojo/public/c/bindings/lib/type_descriptor.h"
#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

struct MojomStructHeader {
  // |num_bytes| includes the size of this struct header along with the
  // accompanying struct data. |num_bytes| must be rounded up to 8 bytes.
  uint32_t num_bytes;
  uint32_t version;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomStructHeader) == 8,
                   "struct MojomStructHeader must be 8 bytes.");

// Returns the number of bytes required to serialize |in_struct|.
// |in_type_desc| is the generated type descriptor that describes the locations
// of the pointers and handles in |in_struct|.
size_t MojomStruct_ComputeSerializedSize(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    const struct MojomStructHeader* in_struct);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_STRUCT_H_
