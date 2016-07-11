// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/union.h"

#include <assert.h>

#include "mojo/public/c/bindings/lib/type_descriptor.h"

size_t MojomUnion_ComputeSerializedSize(
    const struct MojomTypeDescriptorUnion* in_type_desc,
    const struct MojomUnionLayout* in_union_data) {
  assert(in_type_desc);
  assert(in_union_data);

  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorUnionEntry* entry =
        &(in_type_desc->entries[i]);
    if (in_union_data->tag != entry->tag)
      continue;

    // We should skip non-pointer types.
    if (!MojomType_IsPointer(entry->elem_type))
      continue;

    return MojomType_DispatchComputeSerializedSize(
        entry->nullable, entry->elem_type, entry->elem_descriptor,
        &(in_union_data->data.pointer.ptr));
  }
  return 0;
}
