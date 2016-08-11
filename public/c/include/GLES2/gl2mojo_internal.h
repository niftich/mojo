// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains Mojo-specific GLES2 declarations.

#ifndef GPU_GLES2_GL2MOJO_INTERNAL_H_
#define GPU_GLES2_GL2MOJO_INTERNAL_H_

#include <GLES2/gl2platform.h>

#define GL_CONTEXT_LOST 0x300E
#define GL_PATH_MODELVIEW_CHROMIUM 0x1700
#define GL_PATH_PROJECTION_CHROMIUM 0x1701
#define GL_PATH_MODELVIEW_MATRIX_CHROMIUM 0x0BA6
#define GL_PATH_PROJECTION_MATRIX_CHROMIUM 0x0BA7

#define GLES2_GET_FUN(name) MojoGLES2gl##name

#include <GLES2/gl2mojo_autogen.h>

#endif  // GPU_GLES2_GL2MOJO_INTERNAL_H_
