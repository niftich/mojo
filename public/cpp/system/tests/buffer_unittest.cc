// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C++ wrappers in mojo/public/cpp/system/buffer.h.

#include "mojo/public/cpp/system/buffer.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

TEST(BufferTest, BasicSharedBuffer) {
  // Cursory compilation test of |MakeScopedHandle()| with shared buffer
  // handles.
  EXPECT_FALSE(MakeScopedHandle(SharedBufferHandle()).is_valid());

  ScopedSharedBufferHandle shared_buffer;
  EXPECT_EQ(MOJO_RESULT_OK, CreateSharedBuffer(nullptr, 100u, &shared_buffer));
  EXPECT_TRUE(shared_buffer.is_valid());

  void* pointer = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK, MapBuffer(shared_buffer.get(), 0, 100u, &pointer,
                                      MOJO_MAP_BUFFER_FLAG_NONE));
  EXPECT_NE(pointer, nullptr);

  // Just try writing to it.
  *static_cast<char*>(pointer) = 'x';

  shared_buffer.reset();

  // Can still write to it even after the handle is closed.
  *static_cast<char*>(pointer) = 'y';

  EXPECT_EQ(MOJO_RESULT_OK, UnmapBuffer(pointer));
}

}  // namespace
}  // namespace mojo
