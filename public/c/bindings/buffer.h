// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_BUFFER_H_
#define MOJO_PUBLIC_C_BINDINGS_BUFFER_H_

#include <stdint.h>

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

// |MojomBuffer| is used to track a buffer state for mojom serialization. The
// user must initialize this struct themselves. See the fields for details.
struct MojomBuffer {
  char* buf;
  // The number of bytes described by |buf|.
  uint32_t buf_size;
  // Must be initialized to 0. MojomBuffer_Allocate() will update it as it
  // consumes |buf|.
  uint32_t num_bytes_used;
};

// Allocates |num_bytes| (rounded up to 8 bytes) from |buf|. Returns NULL if
// there isn't enough space left to allocate.
void* MojomBuffer_Allocate(struct MojomBuffer* buf, uint32_t num_bytes);

// |MojomHandleBuffer| is used to track handle offsets during serialization.
// Handles are moved into the |handles| array, and are referred to by index
// into the array. The user must initialize this struct themselves. See the
// fields for details.
struct MojomHandleBuffer {
  // |handles| must contain enough space to store all the valid MojoHandles
  // encountered during serialization.
  MojoHandle* handles;
  // Size of the |handles| array.
  uint32_t num_handles;
  // The number of handles used so far in |handles|. As handles are moved into
  // |handles|, this counter is incremented. The caller can use it to determine
  // how many handles were moved into |handles| after serialization. This
  // counter must be initialized appropriately before the struct can be used.
  uint32_t num_handles_used;
};

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_BUFFER_H_
