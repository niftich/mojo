// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_TESTS_BINDINGS_TESTING_UTIL_H_
#define MOJO_PUBLIC_C_TESTS_BINDINGS_TESTING_UTIL_H_

#include <mojo/bindings/buffer.h>
#include <mojo/system/handle.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "third_party/gtest/include/gtest/gtest.h"

// This will copy the supplied |in_struct| and compare it against the new
// copy, expecting them to be the same. It compares the encoded version to be
// sure that they are the same, since a unencoded version will have pointers
// pointing to separate objects in the two copies, whereas encoded versions will
// only have relative offsets for pointers. The new copy will remain encoded,
// and the original will be decoded.
// This function won't work for structs with handles, and will crash.
template <typename T>
void CopyAndCompare(
    MojomBuffer* buf,
    T* in_struct,
    size_t in_struct_size,
    T* (*copy_fn)(struct MojomBuffer* in_buffer, T* in_struct),
    void (*encode_fn)(T* inout_struct,
                      uint32_t size,
                      struct MojomHandleBuffer* inout_handle_buffer),
    void (*decode_fn)(T* inout_struct,
                      uint32_t size,
                      MojoHandle* handles,
                      uint32_t num_handles)) {
  T* out_struct = copy_fn(buf, in_struct);
  ASSERT_TRUE(out_struct);
  EXPECT_EQ(in_struct_size, buf->num_bytes_used);

  encode_fn(in_struct, in_struct_size, NULL);
  encode_fn(out_struct, buf->num_bytes_used, NULL);
  EXPECT_EQ(0, memcmp(in_struct, out_struct, buf->num_bytes_used));

  decode_fn(in_struct, in_struct_size, NULL, 0);
}

#endif  // MOJO_PUBLIC_C_TESTS_BINDINGS_TESTING_UTIL_H_
