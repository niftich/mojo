// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERNAL_UTIL_H_
#define MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERNAL_UTIL_H_

#include <mojo/macros.h>
#include <stdint.h>

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

// This struct represents the state we need to keep when validating an encoded
// mojom type.
struct MojomValidationContext {
  // |next_handle_index| is a counter that represents the next-available index
  // into the handle array. Previous indices have already been used up. This
  // counter is non-decreasing as it is used throughout validation.
  uint32_t next_handle_index;

  // As we validate, we keep track of the point where pointers can point to,
  // since two pointers may not point to the same memory region.
  char* next_pointer;

  // TODO(vardhan): Include an error string? How big should it be?
};

#endif  // MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERNAL_UTIL_H_
