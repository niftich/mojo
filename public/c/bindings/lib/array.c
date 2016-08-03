// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/array.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "mojo/public/c/bindings/buffer.h"
#include "mojo/public/c/bindings/interface.h"
#include "mojo/public/c/bindings/lib/type_descriptor.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/bindings/union.h"

struct MojomArrayHeader* MojomArray_New(struct MojomBuffer* buf,
                                        uint32_t num_elements,
                                        uint32_t element_byte_size) {
  assert(buf);

  uint64_t num_bytes = sizeof(struct MojomArrayHeader) +
                       (uint64_t)num_elements * element_byte_size;
  if (num_bytes > UINT32_MAX)
    return NULL;

  struct MojomArrayHeader* arr =
      (struct MojomArrayHeader*)MojomBuffer_Allocate(buf, (uint32_t)num_bytes);
  if (arr == NULL)
    return NULL;

  assert((uintptr_t)arr + MOJOM_INTERNAL_ROUND_TO_8(num_bytes) ==
         (uintptr_t)buf->buf + buf->num_bytes_used);

  arr->num_elements = num_elements;
  arr->num_bytes = MOJOM_INTERNAL_ROUND_TO_8((uint32_t)num_bytes);

  return arr;
}

// Gets the |index|th element (whose type is described by |type|) of |array|.
// Only supports non-POD types.
static void* array_index_by_type(const struct MojomArrayHeader* array,
                                 enum MojomTypeDescriptorType type,
                                 size_t index) {
  switch (type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR:
      return MOJOM_ARRAY_INDEX(array, union MojomPointer, index);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION:
      return MOJOM_ARRAY_INDEX(array, struct MojomUnionLayout, index);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
      return MOJOM_ARRAY_INDEX(array, MojoHandle, index);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE:
      return MOJOM_ARRAY_INDEX(array, struct MojomInterfaceData, index);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      // This is a type that isn't supported in an array.
      assert(0);
      break;
  }
  return NULL;
}

size_t MojomArray_ComputeSerializedSize(
    const struct MojomTypeDescriptorArray* in_type_desc,
    const struct MojomArrayHeader* in_array) {
  assert(in_array);
  assert(in_type_desc);

  size_t size = in_array->num_bytes;
  if (!MojomType_IsPointer(in_type_desc->elem_type) &&
      in_type_desc->elem_type != MOJOM_TYPE_DESCRIPTOR_TYPE_UNION)
    return size;

  for (uint32_t i = 0; i < in_array->num_elements; i++) {
    size += MojomType_DispatchComputeSerializedSize(
        in_type_desc->elem_type,
        in_type_desc->elem_descriptor,
        in_type_desc->nullable,
        array_index_by_type(in_array, in_type_desc->elem_type, i));
  }

  return size;
}

void MojomArray_EncodePointersAndHandles(
    const struct MojomTypeDescriptorArray* in_type_desc,
    struct MojomArrayHeader* inout_array,
    uint32_t in_array_size,
    struct MojomHandleBuffer* inout_handles_buffer) {
  assert(in_type_desc);
  assert(inout_array);
  assert(in_array_size >= sizeof(struct MojomArrayHeader));
  assert(in_array_size >= inout_array->num_bytes);

  // Nothing to encode for POD types.
  if (in_type_desc->elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_POD)
    return;

  for (size_t i = 0; i < inout_array->num_elements; i++) {
    char* elem_data =
        array_index_by_type(inout_array, in_type_desc->elem_type, i);
    assert(elem_data < (char*)inout_array + in_array_size);

    MojomType_DispatchEncodePointersAndHandles(
        in_type_desc->elem_type,
        in_type_desc->elem_descriptor,
        in_type_desc->nullable,
        elem_data,
        in_array_size - (uint32_t)(elem_data - (char*)inout_array),
        inout_handles_buffer);
  }
}

void MojomArray_DecodePointersAndHandles(
    const struct MojomTypeDescriptorArray* in_type_desc,
    struct MojomArrayHeader* inout_array,
    uint32_t in_array_size,
    MojoHandle* inout_handles,
    uint32_t in_num_handles) {
  assert(in_type_desc);
  assert(inout_array);
  assert(inout_handles != NULL || in_num_handles == 0);

  // Nothing to encode for POD types.
  if (in_type_desc->elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_POD)
    return;

  for (size_t i = 0; i < inout_array->num_elements; i++) {
    char* elem_data =
        array_index_by_type(inout_array, in_type_desc->elem_type, i);
    assert(elem_data < (char*)inout_array + in_array_size);

    MojomType_DispatchDecodePointersAndHandles(
        in_type_desc->elem_type,
        in_type_desc->elem_descriptor,
        in_type_desc->nullable,
        elem_data,
        in_array_size - (uint32_t)(elem_data - (char*)inout_array),
        inout_handles,
        in_num_handles);
  }
}

// Rounds up to nearest byte.
static uint64_t bits_to_bytes(uint64_t bits) {
  return (bits + 7) / 8;
}

static MojomValidationResult validate_array_header(
    const struct MojomTypeDescriptorArray* in_type_desc,
    const struct MojomArrayHeader* in_array,
    uint32_t in_buf_size) {
  if (in_buf_size < sizeof(struct MojomArrayHeader))
    return MOJOM_VALIDATION_ILLEGAL_MEMORY_RANGE;

  if (in_array->num_bytes < sizeof(struct MojomArrayHeader))
    return MOJOM_VALIDATION_UNEXPECTED_ARRAY_HEADER;

  if (in_array->num_bytes > in_buf_size)
    return MOJOM_VALIDATION_ILLEGAL_MEMORY_RANGE;

  if (in_type_desc->num_elements != 0 &&
      in_array->num_elements != in_type_desc->num_elements)
    return MOJOM_VALIDATION_UNEXPECTED_ARRAY_HEADER;

  // Array size is less than what we need to fit the elements.
  if (in_array->num_bytes <
      sizeof(struct MojomArrayHeader) +
          bits_to_bytes((uint64_t)in_type_desc->elem_num_bits *
                        (uint64_t)in_array->num_elements)) {
    return MOJOM_VALIDATION_UNEXPECTED_ARRAY_HEADER;
  }

  return MOJOM_VALIDATION_ERROR_NONE;
}

MojomValidationResult MojomArray_Validate(
    const struct MojomTypeDescriptorArray* in_type_desc,
    const struct MojomArrayHeader* in_array,
    uint32_t in_array_size,
    uint32_t in_num_handles,
    struct MojomValidationContext* inout_context) {
  assert(in_type_desc);
  assert(in_array);

  MojomValidationResult result =
      validate_array_header(in_type_desc, in_array, in_array_size);
  if (result != MOJOM_VALIDATION_ERROR_NONE)
    return result;

  // From here on out, all pointers need to point past the end of this struct.
  inout_context->next_pointer = (char*)in_array + in_array->num_bytes;

  // Nothing to validate for POD types.
  if (in_type_desc->elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_POD)
    return MOJOM_VALIDATION_ERROR_NONE;

  for (size_t i = 0; i < in_array->num_elements; i++) {
    char* elem_data =
        array_index_by_type(in_array, in_type_desc->elem_type, i);

    MojomValidationResult result = MojomType_DispatchValidate(
        in_type_desc->elem_type,
        in_type_desc->elem_descriptor,
        in_type_desc->nullable,
        elem_data,
        in_array_size - (uint32_t)(elem_data - (char*)in_array),
        in_num_handles,
        inout_context);
    if (result != MOJOM_VALIDATION_ERROR_NONE)
      return result;
  }

  return MOJOM_VALIDATION_ERROR_NONE;
}
