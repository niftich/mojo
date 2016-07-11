// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_LIB_UTIL_H_
#define MOJO_PUBLIC_C_BINDINGS_LIB_UTIL_H_

#include "mojo/public/c/system/macros.h"

// Rounds-up |num| to 8. The result is undefined if this results in an overflow.
#define MOJOM_INTERNAL_ROUND_TO_8(num) (((num) + 7) & ~7)

// Represents the memory layout of a mojom pointer; it is a |ptr| (pointer to
// some memory) when unencoded, and refers to a relative |offset| when encoded.
union MojomPointer {
  void* ptr;
  uint64_t offset;
};
MOJO_STATIC_ASSERT(sizeof(union MojomPointer) == 8,
                   "union MojomPointer must be 8 bytes");

#endif  // MOJO_PUBLIC_C_BINDINGS_LIB_UTIL_H_
