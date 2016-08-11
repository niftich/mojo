// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/bindings/map.h>

#include <mojo/macros.h>

struct MojomMapLayout {
  struct MojomStructHeader header_;
  union MojomArrayHeaderPtr keys;
  union MojomArrayHeaderPtr values;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomMapLayout) == 24u,
                   "MojomMapLayout is an invalid size.");

MojomValidationResult MojomMap_Validate(
    const struct MojomTypeDescriptorStruct* in_type_desc,
    const struct MojomStructHeader* in_struct,
    uint32_t in_buf_size,
    uint32_t in_num_handles,
    struct MojomValidationContext* inout_context) {
  // A mojom map consists of 2 arrays (pointers), both of equal size.
  const struct MojomMapLayout* map = (const struct MojomMapLayout*)in_struct;
  struct MojomArrayHeader* keys_arr =
      (struct MojomArrayHeader*)((char*)map +
                                   (offsetof(struct MojomMapLayout, keys) +
                                    map->keys.offset));
  struct MojomArrayHeader* values_arr =
      (struct MojomArrayHeader*)((char*)map +
                                   (offsetof(struct MojomMapLayout, values) +
                                    map->values.offset));

  if (keys_arr->num_elements != values_arr->num_elements)
    return MOJOM_VALIDATION_DIFFERENT_SIZED_ARRAYS_IN_MAP;

  return MOJOM_VALIDATION_ERROR_NONE;
}
