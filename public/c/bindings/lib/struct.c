// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/struct.h"

#include <assert.h>

#include "mojo/public/c/bindings/lib/type_descriptor.h"
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
        entry->elem_type, entry->elem_descriptor, entry->nullable,
        (char*)in_struct + sizeof(struct MojomStructHeader) + entry->offset);
  }
  return size;
}

void MojomStruct_EncodePointersAndHandles(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    struct MojomStructHeader* inout_struct,
    uint32_t in_struct_size,
    struct MojomHandleBuffer* inout_handles_buffer) {
  assert(in_type_desc);
  assert(inout_struct);
  assert(in_struct_size >= sizeof(struct MojomStructHeader));

  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorStructEntry* entry =
        &(in_type_desc->entries[i]);

    if (inout_struct->version < entry->min_version)
      continue;

    assert(sizeof(struct MojomStructHeader) + entry->offset < in_struct_size);
    void* elem_data = ((char*)inout_struct + sizeof(struct MojomStructHeader) +
                       entry->offset);

    MojomType_DispatchEncodePointersAndHandles(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        elem_data,
        in_struct_size - ((char*)elem_data - (char*)inout_struct),
        inout_handles_buffer);
  }
}

void MojomStruct_DecodePointersAndHandles(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    struct MojomStructHeader* inout_struct,
    uint32_t in_struct_size,
    MojoHandle* inout_handles,
    uint32_t in_num_handles) {
  assert(in_type_desc);
  assert(inout_struct);
  assert(inout_handles != NULL || in_num_handles == 0);

  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorStructEntry* entry =
        &(in_type_desc->entries[i]);

    if (inout_struct->version < entry->min_version)
      continue;

    assert(sizeof(struct MojomStructHeader) + entry->offset < in_struct_size);
    void* elem_data = ((char*)inout_struct + sizeof(struct MojomStructHeader) +
                       entry->offset);

    MojomType_DispatchDecodePointersAndHandles(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        elem_data,
        in_struct_size - ((char*)elem_data - (char*)inout_struct),
        inout_handles,
        in_num_handles);
  }
}

static bool is_valid_size_for_version(
    const struct MojomStructHeader* in_struct,
    const struct MojomTypeDescriptorStructVersion versions[],
    uint32_t num_versions) {
  // Scan for a version size in reverse order (assuming structs are often newer
  // versions than old). Seek to the most recent version that |in_struct| is
  // compatible with.
  uint32_t i = num_versions - 1;
  for (; i > 0 && versions[i].version > in_struct->version; i--);

  if (in_struct->version == versions[i].version) {
    return in_struct->num_bytes == versions[i].num_bytes;
  } else {
    return in_struct->num_bytes >= versions[i].num_bytes;
  }
}

MojomValidationResult MojomStruct_Validate(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    const struct MojomStructHeader* in_struct,
    uint32_t in_struct_size,
    uint32_t in_num_handles,
    struct MojomValidationContext* inout_context) {
  assert(in_type_desc);
  assert(in_struct);

  // Struct header validation.
  if (in_struct_size < sizeof(struct MojomStructHeader))
    return MOJOM_VALIDATION_ILLEGAL_MEMORY_RANGE;

  if (in_struct->num_bytes > in_struct_size)
    return MOJOM_VALIDATION_ILLEGAL_MEMORY_RANGE;

  if (!is_valid_size_for_version(in_struct, in_type_desc->versions,
                                 in_type_desc->num_versions)) {
    return MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER;
  }

  if ((in_struct->num_bytes & 7) != 0)
    return MOJOM_VALIDATION_MISALIGNED_OBJECT;

  // From here on out, all pointers need to point past the end of this struct.
  inout_context->next_pointer = (char*)in_struct + in_struct->num_bytes;

  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorStructEntry* entry =
        &(in_type_desc->entries[i]);

    if (in_struct->version < entry->min_version)
        continue;

    void* elem_data = ((char*)in_struct + sizeof(struct MojomStructHeader) +
                       entry->offset);
    MojomValidationResult result = MojomType_DispatchValidate(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        elem_data,
        in_struct_size - ((char*)elem_data - (char*)in_struct),
        in_num_handles,
        inout_context);
    if (result != MOJOM_VALIDATION_ERROR_NONE)
      return result;
  }

  return MOJOM_VALIDATION_ERROR_NONE;
}
