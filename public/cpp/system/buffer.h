// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides a C++ wrapping around the Mojo C API for shared buffers,
// replacing the prefix of "Mojo" with a "mojo" namespace, and using more
// strongly-typed representations of |MojoHandle|s.
//
// Please see "mojo/public/c/include/mojo/system/buffer.h" for complete
// documentation of the API.

#ifndef MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_
#define MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_

#include <assert.h>
#include <mojo/system/buffer.h>

#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {

// A strongly-typed representation of a |MojoHandle| referring to a shared
// buffer.
class SharedBufferHandle : public Handle {
 public:
  SharedBufferHandle() {}
  explicit SharedBufferHandle(MojoHandle value) : Handle(value) {}

  // Copying and assignment allowed.
};

static_assert(sizeof(SharedBufferHandle) == sizeof(Handle),
              "Bad size for C++ SharedBufferHandle");

typedef ScopedHandleBase<SharedBufferHandle> ScopedSharedBufferHandle;
static_assert(sizeof(ScopedSharedBufferHandle) == sizeof(SharedBufferHandle),
              "Bad size for C++ ScopedSharedBufferHandle");

// Creates a shared buffer. See |MojoCreateSharedBuffer()| for complete
// documentation.
inline MojoResult CreateSharedBuffer(
    const MojoCreateSharedBufferOptions* options,
    uint64_t num_bytes,
    ScopedSharedBufferHandle* shared_buffer) {
  assert(shared_buffer);
  SharedBufferHandle handle;
  MojoResult rv =
      MojoCreateSharedBuffer(options, num_bytes, handle.mutable_value());
  // Reset even on failure (reduces the chances that a "stale"/incorrect handle
  // will be used).
  shared_buffer->reset(handle);
  return rv;
}

// Maps a part of a buffer (specified by |buffer|, |offset|, and |num_bytes|)
// into memory. See |MojoMapBuffer()| for complete documentation.
template <class BufferHandleType>
inline MojoResult MapBuffer(BufferHandleType buffer,
                            uint64_t offset,
                            uint64_t num_bytes,
                            void** pointer,
                            MojoMapBufferFlags flags) {
  assert(buffer.is_valid());
  return MojoMapBuffer(buffer.value(), offset, num_bytes, pointer, flags);
}

// Unmaps a part of a buffer that was previously mapped with |MapBuffer()|.
// See |MojoUnmapBuffer()| for complete documentation.
inline MojoResult UnmapBuffer(void* pointer) {
  assert(pointer);
  return MojoUnmapBuffer(pointer);
}

// A wrapper class that automatically creates a shared buffer and owns the
// handle.
class SharedBuffer {
 public:
  explicit SharedBuffer(uint64_t num_bytes);
  SharedBuffer(uint64_t num_bytes,
               const MojoCreateSharedBufferOptions& options);
  ~SharedBuffer();

  ScopedSharedBufferHandle handle;
};

inline SharedBuffer::SharedBuffer(uint64_t num_bytes) {
  MojoResult result = CreateSharedBuffer(nullptr, num_bytes, &handle);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
}

inline SharedBuffer::SharedBuffer(
    uint64_t num_bytes,
    const MojoCreateSharedBufferOptions& options) {
  MojoResult result = CreateSharedBuffer(&options, num_bytes, &handle);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
}

inline SharedBuffer::~SharedBuffer() {
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_
