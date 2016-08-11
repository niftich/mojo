// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains structs used for constructing type descriptors for
// generated mojom types. A type descriptor for a mojom struct is a table
// describing the byte-offsets of all the pointers and handles in the struct,
// and has references to the type descriptorss that further describe the
// pointers. The table is used for doing all computations for the struct --
// determining serialized size, encoding and decoding recursively, etc. A type
// descriptor is generated for each struct, union, array and map. Mojom maps are
// just mojom structs with two mojom arrays. We denote it as a separate type,
// but it is described the same way as a mojom struct.
//
// The user is not expected to construct type descriptors -- a bindings
// generator will do this when it generates bindings for a mojom file.
//
// A type descriptor for a mojom struct is a |MojomTypeDescriptorStruct|
// containing an array of entries of types of |MojomTypeDescriptorStructEntry|.
// Similarly, unions are described with |MojomTypeDescriptorUnion| with entries
// of |MojomTypeDescriptorUnionEntry|. Arrays are described with
// |MojomTypeDescriptorArray|.

#ifndef MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERNAL_TYPE_DESCRIPTOR_H_
#define MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERNAL_TYPE_DESCRIPTOR_H_

#include <mojo/bindings/buffer.h>
#include <mojo/bindings/internal/util.h>
#include <mojo/bindings/validation.h>
#include <mojo/macros.h>
#include <mojo/system/handle.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

MOJO_BEGIN_EXTERN_C

// This enum is used in a type descriptor entry for the |elem_type| field, and
// indicates which type the accompanying |elem_descriptor| is describing (if it
// is describing a reference type), or to indicate that it's a handle type.
// Values that correspond to an |elem_descriptor|'s pointer type:
// - If MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR, then |elem_descriptor| points to
//   a |MojomTypeDescriptorStruct|.
// - If MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR, then |elem_descriptor| points to
//   a |MojomTypeDescriptorStruct|.
// - If MOJOM_TYPE_DESCRIPTOR_TYPE_UNION or
//   MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR, then |elem_descriptor| points to a
//   |MojomTypeDescriptorUnion|.
// - If MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR, then |elem_descriptor| points to a
//   |MojomTypeDescriptorArray|.
// - For any other value, |elem_descriptor| is NULL.
enum MojomTypeDescriptorType {
  // Note: A map is a mojom struct with 2 mojom arrays, so we don't have a
  // separate descriptor type for it.
  MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR = 0,
  MOJOM_TYPE_DESCRIPTOR_TYPE_MAP_PTR = 1,
  MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR = 2,
  // A MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR only occurs inside a
  // |MojomTypeDescriptorUnion|, since union fields inside unions are encoded as
  // pointers to an out-of-line union.
  MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR = 3,
  // A union that is not inside a union is inlined, and described by
  // MOJOM_TYPE_DESCRIPTOR_TYPE_UNION.
  MOJOM_TYPE_DESCRIPTOR_TYPE_UNION = 4,
  MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE = 5,
  MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE = 6,
  // This is only used in an array descriptor, and serves as a way to terminate
  // a chain of array descriptors; the last entry in the chain always contains a
  // plain-old-data type.
  MOJOM_TYPE_DESCRIPTOR_TYPE_POD = 7,
};

struct MojomTypeDescriptorStructVersion {
  uint32_t version;
  uint32_t num_bytes;
};

// Mojom structs are described using this struct.
struct MojomTypeDescriptorStruct {
  uint32_t num_versions;
  // Increasing ordered by version.
  struct MojomTypeDescriptorStructVersion* versions;
  // |entries| is an array of |num_entries|, each describing a field of
  // reference or handle type.
  uint32_t num_entries;
  const struct MojomTypeDescriptorStructEntry* entries;
};

// This struct is used to describe each entry in a mojom struct. Each entry
// indicates whether it is describing a reference type, or a handle type through
// the field |elem_type|. |elem_descriptor| points to the type descriptor
// describing the field, if the type has one (handles don't have type
// descriptors). |offset| indicates the starting byte offset of the element
// within the struct.
struct MojomTypeDescriptorStructEntry {
  // See comments for |MojomTypeDescriptorType| on possible values and
  // corresponding behaviour with |elem_descriptor|.
  enum MojomTypeDescriptorType elem_type;
  // Use |elem_type| to decide which type |elem_descriptor| should be casted to.
  const void* elem_descriptor;
  // |offset| does not account for the struct header. Offset 0 always refers to
  // the first element.
  uint32_t offset;
  // Corresponds to the '[MinVersion]' attribute in mojom IDL. This determines
  // if this field should be ignored if its min_version < version of the struct
  // we are dealing with.
  uint32_t min_version;
  // Is this field nullable?
  bool nullable;
};

// Mojom unions are described using this struct.
struct MojomTypeDescriptorUnion {
  // Number of fields in the union.
  uint32_t num_fields;
  // Number of elements in the |entries| array below.
  uint32_t num_entries;
  // |entries| only includes entries for pointer and handle types.
  const struct MojomTypeDescriptorUnionEntry* entries;
};

// Like |MojomTypeDescriptorStructEntry|, this variant is used to construct a
// type descriptor for a union. Instead of an offset, it describes a union field
// by its |tag|.
struct MojomTypeDescriptorUnionEntry {
  // See comments for |MojomTypeDescriptorType| on possible values and
  // corresponding behaviour with |elem_descriptor|.
  enum MojomTypeDescriptorType elem_type;
  const void* elem_descriptor;
  // The tag of the union field.
  uint32_t tag;
  bool nullable;
};

// Describes a mojom array. To describe an array, we don't need a table of
// entries, since arrays can only describe 1 type. However, that one type can
// recursively be an array (e.g, array<array<int>>), in which case a chain of
// array entries are constructed.
struct MojomTypeDescriptorArray {
  enum MojomTypeDescriptorType elem_type;
  const void* elem_descriptor;
  // How many elements is this array expected to hold?
  // 0 means unspecified.
  uint32_t num_elements;
  // Size of each element, in bits (since bools are 1-bit in size). This is
  // needed to validate the size of an array.
  uint32_t elem_num_bits;
  bool nullable;
};

// This describes a mojom string.
// A mojom string is a mojom array of chars without a fixed-sized.
extern const struct MojomTypeDescriptorArray g_mojom_string_type_description;

// Returns true if |type| is a "pointer" type: an array or a struct, whose data
// is referenced by a pointer (or an offset, if serialized).
// Since unions being pointer types depends on their container, we don't include
// them here.
bool MojomType_IsPointer(enum MojomTypeDescriptorType type);

// This helper function, depending on |type|, calls the appropriate
// *_ComputeSerializedSize(|type_desc|, |data|).
size_t MojomType_DispatchComputeSerializedSize(
    enum MojomTypeDescriptorType type,
    const void* type_desc,
    bool nullable,
    const void* data);

// This helper function, depending on |in_elem_type|, calls the appropriate
// *_EncodePointersAndHandles(...). If |in_elem_type| describes a pointer, it
// first encodes the pointer before calling the associated
// *_EncodePointersAndHandles(...). If |in_elem_type| describes a handle, it
// encodes the handle into |inout_handles|.
void MojomType_DispatchEncodePointersAndHandles(
    enum MojomTypeDescriptorType in_elem_type,
    const void* in_type_desc,
    bool in_nullable,
    void* inout_buf,
    uint32_t in_buf_size,
    struct MojomHandleBuffer* inout_handles_buffer);

// This helper function, depending on |in_elem_type|, calls the appropriate
// *_DecodePointersAndHandles(...). If |in_elem_type| describes a pointer, it
// first decodes the offset into a pointer before calling the associated
// *_DecodePointersAndHandles(...). If |in_elem_type| describes a handle, it
// decodes the handle by looking up it up in |inout_handles|.
void MojomType_DispatchDecodePointersAndHandles(
    enum MojomTypeDescriptorType in_elem_type,
    const void* in_type_desc,
    bool in_nullable,
    void* inout_buf,
    uint32_t in_buf_size,
    MojoHandle* inout_handles,
    uint32_t in_num_handles);

MojomValidationResult MojomType_DispatchValidate(
    enum MojomTypeDescriptorType in_elem_type,
    const void* in_type_desc,
    bool in_nullable,
    const void* in_buf,
    uint32_t in_buf_size,
    uint32_t in_num_handles,
    struct MojomValidationContext* inout_context);

// This helper function, depending on |in_elem_type|, calls the appropriate
// *_DeepCopy(...). The result of that call is then assigned to |out_data|. If
// |in_type_type| describes a pointer to a union, it allocates space for the
// new union before dispatching a call to MojomUnion_DeepCopy. Returns false if
// the copy failed due to insufficient space in the buffer.
bool MojomType_DispatchDeepCopy(struct MojomBuffer* buffer,
                                enum MojomTypeDescriptorType in_elem_type,
                                const void* in_type_desc,
                                const void* in_data,
                                void* out_data);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_INCLUDE_MOJO_BINDINGS_INTERNAL_TYPE_DESCRIPTOR_H_
