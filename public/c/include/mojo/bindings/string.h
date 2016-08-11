// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_STRING_H_
#define MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_STRING_H_

#include <mojo/bindings/array.h>
#include <mojo/bindings/buffer.h>

// A mojom string is a mojom array of UTF-8 chars that need not be
// null-terminated.
struct MojomStringHeader {
  struct MojomArrayHeader chars;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomStringHeader) ==
                       sizeof(struct MojomArrayHeader),
                   "MojomStringHeader must be just a MojomArrayHeader.");

union MojomStringHeaderPtr {
  struct MojomStringHeader* ptr;
  uint64_t offset;
};
MOJO_STATIC_ASSERT(sizeof(union MojomStringHeaderPtr) == 8,
                   "MojomStringHeaderPtr must be 8 byes.");

#endif  // MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_STRING_H_
