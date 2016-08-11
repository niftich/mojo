// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERFACE_H_
#define MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERFACE_H_

#include <mojo/macros.h>
#include <mojo/system/handle.h>
#include <stdint.h>

MOJO_BEGIN_EXTERN_C

// |MojomInterfaceData| represents a handle to an interface; in particular, this
// is the client side of the interface where messages are sent and responses are
// received. When encoded, |handle| represents the index into an array of
// handles. The encoded version of an invalid handle is '-1' of MojoHandle type.
struct MojomInterfaceData {
  MojoHandle handle;
  uint32_t version;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomInterfaceData) == 8,
                   "struct MojomInterfaceData must be 8 bytes.");

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERFACE_H_
