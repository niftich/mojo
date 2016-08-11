// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_INCLUDE_MGL_MGL_ECHO_H_
#define MOJO_PUBLIC_C_INCLUDE_MGL_MGL_ECHO_H_

#include <MGL/mgl_types.h>
#include <mojo/macros.h>
#include <stdint.h>

MOJO_BEGIN_EXTERN_C

typedef void (*MGLEchoCallback)(void* closure);

void MGLEcho(MGLEchoCallback callback, void* closure);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_INCLUDE_MGL_MGL_ECHO_H_
