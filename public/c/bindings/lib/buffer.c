// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/buffer.h"

#include <assert.h>
#include <stddef.h>

#include "mojo/public/c/bindings/lib/util.h"

void* MojomBuffer_Allocate(struct MojomBuffer* buf, uint32_t num_bytes) {
  assert(buf);

  const uint32_t bytes_used = buf->num_bytes_used;
  const uint64_t size = MOJOM_INTERNAL_ROUND_TO_8((uint64_t)num_bytes);
  if (bytes_used + size > buf->buf_size)
    return NULL;

  buf->num_bytes_used += size;
  return buf->buf + bytes_used;
}
