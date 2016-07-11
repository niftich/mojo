// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/struct.h"

#include <assert.h>

#include "mojo/public/c/bindings/lib/type_descriptor.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/bindings/union.h"

size_t MojomStruct_ComputeSerializedSize(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    const struct MojomStructHeader* in_struct) {
  assert(in_struct);
  assert(in_type_desc);

  size_t size = in_struct->num_bytes;
  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorStructEntry* entry =
        &(in_type_desc->entries[i]);

    if (!MojomType_IsPointer(entry->elem_type) &&
        entry->elem_type != MOJOM_TYPE_DESCRIPTOR_TYPE_UNION)
      continue;

    if (in_struct->version < entry->min_version)
      continue;

    size += MojomType_DispatchComputeSerializedSize(
        entry->nullable, entry->elem_type, entry->elem_descriptor,
        (char*)in_struct + sizeof(struct MojomStructHeader) + entry->offset);
  }
  return size;
}
