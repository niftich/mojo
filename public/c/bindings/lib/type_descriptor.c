// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/lib/type_descriptor.h"

#include <assert.h>

#include "mojo/public/c/bindings/array.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/bindings/struct.h"
#include "mojo/public/c/bindings/union.h"

const struct MojomTypeDescriptorArray g_mojom_string_type_description = {
    MOJOM_TYPE_DESCRIPTOR_TYPE_POD,  // elem_type
    NULL,                            // elem_descriptor
    0,                               // num_elements
    false,                           // nullable
};

bool MojomType_IsPointer(enum MojomTypeDescriptorType type) {
 return type == MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR ||
        type == MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR ||
        type == MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR;
}

size_t MojomType_DispatchComputeSerializedSize(
    bool nullable,
    enum MojomTypeDescriptorType type,
    const void* type_desc,
    const void* data) {
  const void* data_ptr = data;
  size_t size = 0;
  switch (type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR: {
      data_ptr = ((const union MojomPointer*)data_ptr)->ptr;
      if (!nullable || data_ptr != NULL)
        return MojomStruct_ComputeSerializedSize(
            (const struct MojomTypeDescriptorStruct*)type_desc,
            (const struct MojomStructHeader*)data_ptr);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR:
      data_ptr = ((const union MojomPointer*)data_ptr)->ptr;
      if (!nullable || data_ptr != NULL)
        return MojomArray_ComputeSerializedSize(type_desc, data_ptr);
      break;
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR:
      data_ptr = ((const union MojomPointer*)data_ptr)->ptr;
      if (data_ptr != NULL)
        size = sizeof(struct MojomUnionLayout);
      // Fall through.
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION: {
      const struct MojomUnionLayout* udata = data_ptr;
      // Unions inside unions may be set to null by setting their pointer to
      // NULL, OR by setting the union's |size| to 0.
      if (!nullable || (udata && udata->size != 0)) {
        return size + MojomUnion_ComputeSerializedSize(
            (const struct MojomTypeDescriptorUnion*)type_desc,
            (const struct MojomUnionLayout*)data);
      }
      break;
    }
    default:
      // We should never end up here.
      assert(false);
  }
  return size;
}
