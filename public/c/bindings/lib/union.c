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
        entry->elem_type, entry->elem_descriptor, entry->nullable,
        &(in_union_data->data.pointer.ptr));
  }
  return 0;
}

void MojomUnion_EncodePointersAndHandles(
    const struct MojomTypeDescriptorUnion* in_type_desc,
    struct MojomUnionLayout* inout_union,
    uint32_t in_buf_size,
    struct MojomHandleBuffer* inout_handles_buffer) {
  assert(in_buf_size >= sizeof(struct MojomUnionLayout));

  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorUnionEntry* entry =
        &(in_type_desc->entries[i]);

    if (inout_union->tag != entry->tag)
      continue;

    if (entry->elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_POD)
      continue;

    MojomType_DispatchEncodePointersAndHandles(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        &inout_union->data,
        in_buf_size - ((char*)&inout_union->data - (char*)inout_union),
        inout_handles_buffer);
  }
}

void MojomUnion_DecodePointersAndHandles(
    const struct MojomTypeDescriptorUnion* in_type_desc,
    struct MojomUnionLayout* inout_union,
    uint32_t in_union_size,
    MojoHandle* inout_handles,
    uint32_t in_num_handles) {
  assert(in_union_size >= sizeof(struct MojomUnionLayout));
  assert(inout_handles != NULL || in_num_handles == 0);

  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorUnionEntry* entry =
        &(in_type_desc->entries[i]);

    if (inout_union->tag != entry->tag)
      continue;

    if (entry->elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_POD)
      continue;

    MojomType_DispatchDecodePointersAndHandles(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        &inout_union->data,
        in_union_size - ((char*)&inout_union->data - (char*)inout_union),
        inout_handles,
        in_num_handles);
  }
}

MojomValidationResult MojomUnion_Validate(
    const struct MojomTypeDescriptorUnion* in_type_desc,
    bool in_nullable,
    const struct MojomUnionLayout* in_union,
    uint32_t in_union_size,
    uint32_t in_num_handles,
    struct MojomValidationContext* inout_context) {
  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorUnionEntry* entry =
        &(in_type_desc->entries[i]);

    if (in_union->tag != entry->tag)
      continue;

    if (entry->elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_POD)
      continue;

    if (!in_nullable && in_union->size != sizeof(struct MojomUnionLayout))
      return MOJOM_VALIDATION_UNEXPECTED_NULL_UNION;

    MojomValidationResult result = MojomType_DispatchValidate(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        &(in_union->data),
        in_union_size - ((char*)&(in_union->data) - (char*)in_union),
        in_num_handles,
        inout_context);
    if (result != MOJOM_VALIDATION_ERROR_NONE)
      return result;
  }
  return MOJOM_VALIDATION_ERROR_NONE;
}
