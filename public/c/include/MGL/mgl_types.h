// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_INCLUDE_MGL_TYPES_H_
#define MOJO_PUBLIC_C_INCLUDE_MGL_TYPES_H_

#include <mojo/macros.h>

MOJO_BEGIN_EXTERN_C

typedef struct MGLContextPrivate* MGLContext;
typedef void (*MGLContextLostCallback)(void* closure);
typedef void (*MGLSignalSyncPointCallback)(void* closure);

// This is a generic function pointer type, which must be cast to the proper
// type and calling convention before use.
typedef void (*MGLMustCastToProperFunctionPointerType)(void);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_INCLUDE_MGL_TYPES_H_
