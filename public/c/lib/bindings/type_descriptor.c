// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/bindings/internal/type_descriptor.h>

#include <assert.h>
#include <mojo/bindings/array.h>
#include <mojo/bindings/interface.h>
#include <mojo/bindings/internal/util.h>
#include <mojo/bindings/map.h>
#include <mojo/bindings/struct.h>
#include <mojo/bindings/union.h>
#include <stddef.h>

const struct MojomTypeDescriptorArray g_mojom_string_type_description = {
  .elem_type = MOJOM_TYPE_DESCRIPTOR_TYPE_POD,
  .elem_descriptor = NULL,
  .num_elements = 0,
  .elem_num_bits = 8,
  .nullable = false,
};

// The encoding of a MojoHandle is an index into an array of Handles. A
// null/invalid handle is encoded as index (which is unsigned) "-1", which
// equates to the highest possible index.
static const MojoHandle kEncodedHandleInvalid = (MojoHandle)-1;

bool MojomType_IsPointer(enum MojomTypeDescriptorType type) {
  return type == MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR ||
         type == MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR ||
         type == MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR ||
         type == MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR;
}

size_t MojomType_DispatchComputeSerializedSize(
    enum MojomTypeDescriptorType type,
    const void* type_desc,
    bool nullable,
    const void* data) {
  const void* data_ptr = data;
  size_t size = 0;
  switch (type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR:
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
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      // We should never end up here.
      assert(false);
      break;
  }
  return size;
}

static void encode_pointer(union MojomPointer* pointer, uint32_t max_offset) {
  if (pointer->ptr == NULL) {
    pointer->offset = 0;
  } else {
    assert((char*)pointer->ptr > (char*)pointer);
    assert((size_t)((char*)pointer->ptr - (char*)pointer) < max_offset);
    pointer->offset = (char*)(pointer->ptr) - (char*)pointer;
  }
}

static void decode_pointer(union MojomPointer* pointer) {
  if (pointer->offset == 0) {
    pointer->ptr = NULL;
  } else {
    pointer->ptr = (char*)pointer + pointer->offset;
  }
}

static void encode_handle(bool nullable, MojoHandle* handle,
                          struct MojomHandleBuffer* handles_buffer) {
  assert(handle);
  assert(handles_buffer);
  assert(handles_buffer->handles);

  if (*handle == MOJO_HANDLE_INVALID) {
    assert(nullable);
    *handle = kEncodedHandleInvalid;
  } else {
    assert(handles_buffer->num_handles_used < handles_buffer->num_handles);

    handles_buffer->handles[handles_buffer->num_handles_used] = *handle;
    *handle = handles_buffer->num_handles_used;
    handles_buffer->num_handles_used++;
  }
}

// *handle is an index into inout_handles, or is encoded NULL.
static void decode_handle(MojoHandle* handle,
                          MojoHandle inout_handles[], uint32_t in_num_handles) {
  assert(handle);
  assert(inout_handles);

  if (*handle == kEncodedHandleInvalid) {
    *handle = MOJO_HANDLE_INVALID;
  } else {
    assert(*handle < in_num_handles);
    MojoHandle index = *handle;
    *handle = inout_handles[index];
    inout_handles[index] = MOJO_HANDLE_INVALID;
  }
}

void MojomType_DispatchEncodePointersAndHandles(
    enum MojomTypeDescriptorType in_elem_type,
    const void* in_type_desc,
    bool in_nullable,
    void* inout_buf,
    uint32_t in_buf_size,
    struct MojomHandleBuffer* inout_handles_buffer) {
  assert(inout_buf);

  void* union_buf = inout_buf;
  switch (in_elem_type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR: {
      struct MojomStructHeader* inout_struct =
          ((union MojomPointer*)inout_buf)->ptr;
      encode_pointer(inout_buf, in_buf_size);
      if (!in_nullable || inout_struct != NULL)
        MojomStruct_EncodePointersAndHandles(
            (const struct MojomTypeDescriptorStruct*)in_type_desc,
            inout_struct,
            in_buf_size - ((char*)inout_struct - (char*)inout_buf),
            inout_handles_buffer);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR: {
      struct MojomArrayHeader* inout_array =
                ((union MojomPointer*)inout_buf)->ptr;
      encode_pointer(inout_buf, in_buf_size);
      if (!in_nullable || inout_array != NULL)
        MojomArray_EncodePointersAndHandles(
            (const struct MojomTypeDescriptorArray*)in_type_desc,
            inout_array,
            in_buf_size - ((char*)inout_array - (char*)inout_buf),
            inout_handles_buffer);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR:
      union_buf = ((union MojomPointer*)inout_buf)->ptr;
      encode_pointer(inout_buf, in_buf_size);
      // Fall through
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION: {
      struct MojomUnionLayout* u_data = union_buf;
      if (!in_nullable || (u_data != NULL && u_data->size != 0))
        MojomUnion_EncodePointersAndHandles(
            (const struct MojomTypeDescriptorUnion*)in_type_desc,
            inout_buf,
            in_buf_size,
            inout_handles_buffer);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
      encode_handle(in_nullable, (MojoHandle*)inout_buf, inout_handles_buffer);
      break;
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE: {
      struct MojomInterfaceData* interface = inout_buf;
      encode_handle(in_nullable, &interface->handle, inout_handles_buffer);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      // We shouldn't ever end up here.
      assert(false);
      break;
  }
}

void MojomType_DispatchDecodePointersAndHandles(
    enum MojomTypeDescriptorType in_elem_type,
    const void* in_type_desc,
    bool in_nullable,
    void* inout_buf,
    uint32_t in_buf_size,
    MojoHandle* inout_handles,
    uint32_t in_num_handles) {
  assert(inout_buf);

  void* union_buf = inout_buf;
  switch (in_elem_type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR: {
      decode_pointer(inout_buf);
      struct MojomStructHeader* inout_struct =
          ((union MojomPointer*)inout_buf)->ptr;
      assert(inout_struct == NULL ||
             (char*)inout_struct < ((char*)inout_buf) + in_buf_size);
      if (!in_nullable || inout_struct != NULL)
        MojomStruct_DecodePointersAndHandles(
            (const struct MojomTypeDescriptorStruct*)in_type_desc,
            inout_struct,
            in_buf_size - ((char*)inout_struct - (char*)inout_buf),
            inout_handles,
            in_num_handles);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR: {
      decode_pointer(inout_buf);
      struct MojomArrayHeader* inout_array =
                ((union MojomPointer*)inout_buf)->ptr;
      assert(inout_array == NULL ||
             (char*)inout_array < ((char*)inout_buf) + in_buf_size);
      if (!in_nullable || inout_array != NULL)
        MojomArray_DecodePointersAndHandles(
            (const struct MojomTypeDescriptorArray*)in_type_desc,
            inout_array,
            in_buf_size - ((char*)inout_array - (char*)inout_buf),
            inout_handles,
            in_num_handles);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR:
      decode_pointer(inout_buf);
      union_buf = ((union MojomPointer*)inout_buf)->ptr;
      assert(union_buf == NULL ||
             (char*)union_buf < ((char*)inout_buf) + in_buf_size);
      // Fall through
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION: {
      struct MojomUnionLayout* u_data = union_buf;
      if (!in_nullable || (u_data != NULL && u_data->size != 0))
        MojomUnion_DecodePointersAndHandles(
            (const struct MojomTypeDescriptorUnion*)in_type_desc,
            inout_buf,
            in_buf_size,
            inout_handles,
            in_num_handles);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
      decode_handle((MojoHandle*)inout_buf, inout_handles,
                    in_num_handles);
      break;
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE: {
      struct MojomInterfaceData* interface = inout_buf;
      decode_handle(&interface->handle, inout_handles,
                    in_num_handles);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      // We shouldn't ever end up here.
      assert(false);
      break;
  }
}

// Validates that the offset (|pointer->offset|) points to a new memory region,
// i.e. one that hasn't been referenced yet. If so, moves the expected offset
// (for the next pointer) forward.
static MojomValidationResult validate_pointer(
    const union MojomPointer* pointer,
    size_t max_offset,
    bool is_nullable,
    struct MojomValidationContext* inout_context) {
  // Offset must be <= UINT32_MAX and within range.
  if (pointer->offset > max_offset || pointer->offset > UINT32_MAX)
    return MOJOM_VALIDATION_ILLEGAL_POINTER;

  if (pointer->offset != 0) {
    if ((char*)pointer + pointer->offset < inout_context->next_pointer)
      return MOJOM_VALIDATION_ILLEGAL_MEMORY_RANGE;

    inout_context->next_pointer = (char*)pointer + pointer->offset;
  }

  // Offset must be 8-byte aligned: this check is sufficient, given that all
  // objects are rounded to 8-bytes.
  if ((pointer->offset & 7) != 0)
    return MOJOM_VALIDATION_MISALIGNED_OBJECT;

  if (!is_nullable && pointer->offset == 0)
    return MOJOM_VALIDATION_UNEXPECTED_NULL_POINTER;

  return MOJOM_VALIDATION_ERROR_NONE;
}

static MojomValidationResult validate_handle(
    MojoHandle encoded_handle, uint32_t num_handles, bool is_nullable,
    struct MojomValidationContext* inout_context) {
  if (!is_nullable && encoded_handle == kEncodedHandleInvalid)
    return MOJOM_VALIDATION_UNEXPECTED_INVALID_HANDLE;

  if (encoded_handle != kEncodedHandleInvalid) {
    if (encoded_handle >= num_handles ||
        encoded_handle < inout_context->next_handle_index)
      return MOJOM_VALIDATION_ILLEGAL_HANDLE;

    inout_context->next_handle_index = encoded_handle + 1;
  }

  return MOJOM_VALIDATION_ERROR_NONE;
}

MojomValidationResult MojomType_DispatchValidate(
    enum MojomTypeDescriptorType in_elem_type, const void* in_type_desc,
    bool in_nullable, const void* in_buf, uint32_t in_buf_size,
    uint32_t in_num_handles, struct MojomValidationContext* inout_context) {
  assert(in_buf);

  struct MojomUnionLayout* union_data = (struct MojomUnionLayout*)in_buf;
  switch (in_elem_type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR: {
      union MojomPointer* pointer = (union MojomPointer*)in_buf;
      MojomValidationResult result =
          validate_pointer(pointer, in_buf_size, in_nullable, inout_context);
      if (result != MOJOM_VALIDATION_ERROR_NONE || pointer->offset == 0)
        return result;

      result = MojomStruct_Validate(
          (const struct MojomTypeDescriptorStruct*)in_type_desc,
          (const struct MojomStructHeader*)((char*)in_buf + pointer->offset),
          in_buf_size - pointer->offset, in_num_handles, inout_context);

      if (result == MOJOM_VALIDATION_ERROR_NONE &&
          in_elem_type == MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR) {
        return MojomMap_Validate(
            (const struct MojomTypeDescriptorStruct*)in_type_desc,
            (const struct MojomStructHeader*)((char*)in_buf + pointer->offset),
            in_buf_size - pointer->offset, in_num_handles, inout_context);
      }

      return result;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR: {
      union MojomPointer* pointer = (union MojomPointer*)in_buf;
      MojomValidationResult result =
          validate_pointer(pointer, in_buf_size, in_nullable, inout_context);
      if (result != MOJOM_VALIDATION_ERROR_NONE || pointer->offset == 0)
        return result;

      return MojomArray_Validate(
          (const struct MojomTypeDescriptorArray*)in_type_desc,
          (const struct MojomArrayHeader*)((char*)in_buf + pointer->offset),
          in_buf_size - pointer->offset, in_num_handles, inout_context);
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR: {
      union MojomPointer* pointer = (union MojomPointer*)in_buf;
      MojomValidationResult result =
          validate_pointer(pointer, in_buf_size, in_nullable, inout_context);
      if (result != MOJOM_VALIDATION_ERROR_NONE || pointer->offset == 0)
        return result;

      // Since this union is a pointer, we update |next_pointer| to be past the
      // union data.
      inout_context->next_pointer += sizeof(struct MojomUnionLayout);

      union_data = (struct MojomUnionLayout*)((char*)in_buf + pointer->offset);
      // Fall through.
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION:
      if (union_data->size == 0) {
        return in_nullable ? MOJOM_VALIDATION_ERROR_NONE
                           : MOJOM_VALIDATION_UNEXPECTED_NULL_UNION;
      }

      return MojomUnion_Validate(
          (const struct MojomTypeDescriptorUnion*)in_type_desc, in_nullable,
          union_data, in_buf_size - ((char*)union_data - (char*)in_buf),
          in_num_handles, inout_context);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
      return validate_handle(*(const MojoHandle*)in_buf, in_num_handles,
                             in_nullable, inout_context);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE:
      return validate_handle(((const struct MojomInterfaceData*)in_buf)->handle,
                             in_num_handles, in_nullable, inout_context);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      break;
  }
  return MOJOM_VALIDATION_ERROR_NONE;
}

bool MojomType_DispatchDeepCopy(struct MojomBuffer* buffer,
                        enum MojomTypeDescriptorType in_elem_type,
                        const void* in_type_desc,
                        const void* in_data,
                        void* out_data) {
  assert(in_data);

  const struct MojomUnionLayout* in_union_data =
      (const struct MojomUnionLayout*)in_data;
  struct MojomUnionLayout* out_union_data = (struct MojomUnionLayout*)out_data;
  const union MojomPointer* in_pointer = in_data;
  switch (in_elem_type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR:
      if (in_pointer->ptr == NULL) {
        ((union MojomPointer*)out_data)->ptr = NULL;
        break;
      }
      return MojomStruct_DeepCopy(
          buffer, (const struct MojomTypeDescriptorStruct*)in_type_desc,
          ((union MojomPointer*)in_data)->ptr,
          out_data);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR:
      if (in_pointer->ptr == NULL) {
        ((union MojomPointer*)out_data)->ptr = NULL;
        break;
      }
      return MojomArray_DeepCopy(
          buffer, (const struct MojomTypeDescriptorArray*)in_type_desc,
          ((union MojomPointer*)in_data)->ptr,
          out_data);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR:
      in_union_data = ((const union MojomPointer*)in_data)->ptr;
      if (in_union_data == NULL) {
        ((union MojomPointer*)out_data)->ptr = NULL;
        break;
      }
      out_union_data =
          MojomBuffer_Allocate(buffer, sizeof(struct MojomUnionLayout));
      if (out_union_data == NULL)
        return false;
      ((union MojomPointer*)out_data)->ptr = out_union_data;
      // Fall through.
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION:
      return MojomUnion_DeepCopy(buffer,
                          (const struct MojomTypeDescriptorUnion*)in_type_desc,
                          in_union_data,
                          out_union_data);
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      break;
  }

  return true;
}
