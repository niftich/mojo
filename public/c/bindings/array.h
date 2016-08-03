// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_ARRAY_H_
#define MOJO_PUBLIC_C_BINDINGS_ARRAY_H_

#include <stdint.h>

#include "mojo/public/c/bindings/buffer.h"
#include "mojo/public/c/bindings/lib/type_descriptor.h"
#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

// The fields below are just the header of a mojom array. The bytes that
// immediately follow this struct consist of |num_bytes -
// sizeof(MojomArrayHeader)| bytes describing |num_elements| elements of the
// array.
struct MojomArrayHeader {
  // num_bytes includes the size of this struct along with the accompanying
  // array bytes that follow these fields.
  uint32_t num_bytes;
  uint32_t num_elements;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomArrayHeader) == 8,
                   "struct MojomArrayHeader must be 8 bytes.");

// This union is used to represent references to a mojom array.
union MojomArrayHeaderPtr {
  // |ptr| is used to access the array when it hasn't been encoded yet.
  struct MojomArrayHeader* ptr;
  // |offset| is used to access the array after it has been encoded.
  uint64_t offset;
};
MOJO_STATIC_ASSERT(sizeof(union MojomArrayHeaderPtr) == 8,
                   "union MojomArrayHeaderPtr must be 8 bytes.");

// Allocates enough space using the given |buffer| for |num_elements|, each of
// which is |element_byte_size| bytes in size. Returns NULL on failure.
struct MojomArrayHeader* MojomArray_New(struct MojomBuffer* buffer,
                                        uint32_t num_elements,
                                        uint32_t element_byte_size);

// This is a macro for accessing a particular element in a mojom array. Given
// |base|, which pointers to a |struct MojomArrayHeader|, extracts the |index|th
// element, where each element is |sizeof(type)| bytes.
#define MOJOM_ARRAY_INDEX(base, type, index)                 \
  ((type*)((char*)(base) + sizeof(struct MojomArrayHeader) + \
           sizeof(type) * (index)))

// Returns the number of bytes required to serialize this mojom array.
// |in_type_desc| is the generated descriptor entry that describes |in_array|.
// The user isn't expected to call this function directly, but this will
// probably be called when |ComputeSerializedSize()|ing a user-defined mojom
// struct.
size_t MojomArray_ComputeSerializedSize(
    const struct MojomTypeDescriptorArray* in_type_desc,
    const struct MojomArrayHeader* in_array_data);

// Encodes the mojom array described by the |inout_array| buffer; note that any
// references from the array are also in the buffer backed by |inout_array|, and
// they are recursively encoded. Encodes all pointers to relative offsets, and
// encodes all handles by moving them into |inout_handles_buffer| and encoding
// the index into the handle.
// |in_type_desc|: Describes the pointer and handle fields of the mojom array.
// |inout_array|: Contains the array, and any other references outside the
//                array.
// |in_array_size|: Size of the buffer backed by |inout_array| in bytes.
// |inout_handles_buffer|:
//   A buffer used to record handles during encoding. The |num_handles_used|
//   field can be used to determine how many handles were moved into this
//   buffer after this function returns.
void MojomArray_EncodePointersAndHandles(
    const struct MojomTypeDescriptorArray* in_type_desc,
    struct MojomArrayHeader* inout_array,
    uint32_t in_array_size,
    struct MojomHandleBuffer* inout_handles_buffer);

// Decodes the mojom array described by the |inout_array| buffer; note that any
// references from the array are also in the buffer backed by |inout_array|, and
// they are recursively decoded. Decodes all offset to pointers, and decodes all
// handles by moving them out of |inout_handles| array using the encoded index.
// |in_type_desc|: Describes the pointer and handle fields of the mojom array.
// |inout_array|: Contains the array, and any other references outside the
//                array.
// |in_array_size|: Size of the buffer backed by |inout_array|.
// |inout_handles|: Mojo handles are moved out of this array, and are references
// by index in |inout_buf|.
// |in_num_handles|: Size in # of number elements available in |inout_handles|.
void MojomArray_DecodePointersAndHandles(
    const struct MojomTypeDescriptorArray* in_type_desc,
    struct MojomArrayHeader* inout_array,
    uint32_t in_array_size,
    MojoHandle* inout_handles,
    uint32_t in_num_handles);

// Validates the mojom array described by the |in_struct| buffer. Any
// references from the array are also recursively validated, and are expected
// to be in the same buffer backing |in_array|.
// |in_type_desc|: Describes the pointer and handle fields of the mojom array.
// |in_array|: Buffer containing the array, and any other references outside of
//             the array
// |in_array_size|: Size of the buffer backed by |in_array| in bytes.
// |in_num_handles|: Number of valid handles expected to be referenced from
//                   |in_array|.
// |inout_context|: An initialized context that contains the expected location
//                  of the next pointer and next offset. This is used to
//                  validate that no two pointers or handles are shared.
MojomValidationResult MojomArray_Validate(
    const struct MojomTypeDescriptorArray* in_type_desc,
    const struct MojomArrayHeader* in_array,
    uint32_t in_array_size,
    uint32_t in_num_handles,
    struct MojomValidationContext* inout_context);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_ARRAY_H_
