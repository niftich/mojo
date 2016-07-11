// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/array.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "mojo/public/c/bindings/buffer.h"
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
    const void* elem_data = NULL;
    if (in_type_desc->elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_UNION) {
      elem_data = MOJOM_ARRAY_INDEX(in_array, struct MojomUnionLayout, i);
    } else {
      elem_data = MOJOM_ARRAY_INDEX(in_array, union MojomPointer, i);
    }
    size += MojomType_DispatchComputeSerializedSize(
        in_type_desc->nullable, in_type_desc->elem_type,
        in_type_desc->elem_descriptor, elem_data);
  }

  return size;
}
