// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_STRUCT_H_
#define MOJO_PUBLIC_C_BINDINGS_STRUCT_H_

#include <stddef.h>
#include <stdint.h>

#include "mojo/public/c/bindings/buffer.h"
#include "mojo/public/c/bindings/lib/type_descriptor.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

struct MojomStructHeader {
  // |num_bytes| includes the size of this struct header along with the
  // accompanying struct data. |num_bytes| must be rounded up to 8 bytes.
  uint32_t num_bytes;
  uint32_t version;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomStructHeader) == 8,
                   "struct MojomStructHeader must be 8 bytes.");

// Returns the number of bytes required to serialize |in_struct|.
// |in_type_desc| is the generated type descriptor that describes the locations
// of the pointers and handles in |in_struct|.
size_t MojomStruct_ComputeSerializedSize(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    const struct MojomStructHeader* in_struct);

// Encodes the mojom struct described by the |inout_struct| buffer; note that
// any references from the struct are also in the buffer backed by
// |inout_struct|, and they are recursively encoded. Encodes all pointers to
// relative offsets, and encodes all handles by moving them into
// |inout_handles_buffer| encoding the index into the handle.
// |in_type_desc|: Describes the pointer and handle fields of the mojom struct.
// |inout_struct|: Contains the struct, and any other references outside of the
//                 struct.
// |in_struct_size|:  Size of the buffer backed by |inout_struct| in bytes.
// |inout_handles_buffer|:
//   A buffer used to record handles during encoding. The |num_handles_used|
//   field can be used to determine how many handles were moved into this
//   buffer.
void MojomStruct_EncodePointersAndHandles(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    struct MojomStructHeader* inout_struct,
    uint32_t in_struct_size,
    struct MojomHandleBuffer* inout_handles_buffer);

// Decodes the mojom struct described by the |inout_struct| buffer; note that
// any references from the struct are also in |inout_struct|, and they are
// recursively decoded. Decodes all offsets to pointers, and decodes all handles
// by moving them out of |inout_handles| array using the encoded index into the
// array.
// |in_type_desc|: Describes the pointer and handle fields of the mojom struct.
// |inout_struct|: Contains the struct, and any other references outside of the
//                 struct.
// |in_struct_size|: Size of the buffer backed by |inout_struct| in bytes.
// |inout_handles|: Mojo handles are in this array, and are referenced by index
//                  in |inout_buf|.
// |in_num_handles|: Size in # of number elements available in |inout_handles|.
void MojomStruct_DecodePointersAndHandles(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    struct MojomStructHeader* inout_struct,
    uint32_t in_struct_size,
    MojoHandle* inout_handles,
    uint32_t in_num_handles);

// Validates the mojom struct described by the |in_struct| buffer. Any
// references from the struct are also recursively validated, and are expected
// to be in the same buffer backing |in_struct|.
// |in_type_desc|: Describes the pointer and handle fields of the mojom struct.
// |inout_struct|: Buffer containing the struct, and any other references
//                 outside of the struct.
// |in_struct_size|: Size of the buffer backed by |inout_struct| in bytes.
// |in_num_handles|: Number of valid handles expected to be referenced from
//                   |in_struct|.
// |inout_context|: An initialized context that contains the expected location
//                  of the next pointer and next offset. This is used to
//                  validate that no two pointers or handles are shared.
MojomValidationResult MojomStruct_Validate(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    const struct MojomStructHeader* in_struct,
    uint32_t in_struct_size,
    uint32_t in_num_handles,
    struct MojomValidationContext* inout_context);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_STRUCT_H_
