// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/bindings/union.h>

#include <assert.h>
#include <mojo/bindings/internal/type_descriptor.h>
#include <string.h>

#define UNION_TAG_UNKNOWN ((uint32_t)0xFFFFFFFF)

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
      break;

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
      break;

    MojomType_DispatchEncodePointersAndHandles(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        &inout_union->data,
        in_buf_size - ((char*)&inout_union->data - (char*)inout_union),
        inout_handles_buffer);

    break;
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
      break;

    MojomType_DispatchDecodePointersAndHandles(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        &inout_union->data,
        in_union_size - ((char*)&inout_union->data - (char*)inout_union),
        inout_handles,
        in_num_handles);

    break;
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
      break;

    if (!in_nullable && in_union->size != sizeof(struct MojomUnionLayout))
      return MOJOM_VALIDATION_UNEXPECTED_NULL_UNION;

    return MojomType_DispatchValidate(
        entry->elem_type,
        entry->elem_descriptor,
        entry->nullable,
        &(in_union->data),
        in_union_size - ((char*)&(in_union->data) - (char*)in_union),
        in_num_handles,
        inout_context);
  }
  return MOJOM_VALIDATION_ERROR_NONE;
}

bool MojomUnion_DeepCopy(struct MojomBuffer* buffer,
                         const struct MojomTypeDescriptorUnion* in_type_desc,
                         const struct MojomUnionLayout* in_union_data,
                         struct MojomUnionLayout* out_union_data) {
  memcpy(out_union_data, in_union_data, sizeof(struct MojomUnionLayout));

  // Unions with size 0 are null.
  if (in_union_data->size == 0)
    return true;

  for (size_t i = 0; i < in_type_desc->num_entries; i++) {
    const struct MojomTypeDescriptorUnionEntry* entry =
        &(in_type_desc->entries[i]);
    if (in_union_data->tag != entry->tag)
      continue;

    // We should skip non-pointer types.
    if (entry->elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_POD)
      return true;

    return MojomType_DispatchDeepCopy(
        buffer, entry->elem_type, entry->elem_descriptor,
        &(in_union_data->data), &(out_union_data->data));
  }
  // We reach here if we don't recognize the tag. If it's the UNKNOWN tag, it's
  // not a failure. If it's an unrecognized tag (erroneous or because this union
  // is from a future-version), we don't know how to copy it, so the copy is a
  // failure.
  return in_union_data->tag < in_type_desc->num_fields ||
         in_union_data->tag == UNION_TAG_UNKNOWN;
}
