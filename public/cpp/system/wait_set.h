// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides a C++ wrapping around the Mojo C API for wait sets,
// replacing the prefix of "Mojo" with a "mojo" namespace, and using more
// strongly-typed representations of |MojoHandle|s.
//
// Please see "mojo/public/c/include/mojo/system/wait_set.h" for complete
// documentation of the API.

#ifndef MOJO_PUBLIC_CPP_SYSTEM_WAIT_SET_H_
#define MOJO_PUBLIC_CPP_SYSTEM_WAIT_SET_H_

#include <assert.h>
#include <mojo/system/wait_set.h>

#include <vector>

#include "mojo/public/cpp/system/handle.h"

namespace mojo {

// A strongly-typed representation of a |MojoHandle| to a wait set.
class WaitSetHandle : public Handle {
 public:
  WaitSetHandle() {}
  explicit WaitSetHandle(MojoHandle value) : Handle(value) {}

  // Copying and assignment allowed.
};

static_assert(sizeof(WaitSetHandle) == sizeof(Handle),
              "Bad size for C++ WaitSetHandle");

typedef ScopedHandleBase<WaitSetHandle> ScopedWaitSetHandle;
static_assert(sizeof(ScopedWaitSetHandle) == sizeof(WaitSetHandle),
              "Bad size for C++ ScopedWaitSetHandle");

// Creates a new wait set. See |MojoCreateWaitSet()| for complete documentation.
inline MojoResult CreateWaitSet(const MojoCreateWaitSetOptions* options,
                                ScopedWaitSetHandle* wait_set) {
  assert(wait_set);
  WaitSetHandle wait_set_handle;
  MojoResult rv = MojoCreateWaitSet(options, wait_set_handle.mutable_value());
  // Reset even on failure (reduces the chances that a "stale"/incorrect handle
  // will be used).
  wait_set->reset(wait_set_handle);
  return rv;
}

inline MojoResult WaitSetAdd(WaitSetHandle wait_set,
                             Handle handle,
                             MojoHandleSignals signals,
                             uint64_t cookie,
                             const struct MojoWaitSetAddOptions* options) {
  return MojoWaitSetAdd(wait_set.value(), handle.value(), signals, cookie,
                        options);
}

inline MojoResult WaitSetRemove(WaitSetHandle wait_set, uint64_t cookie) {
  return MojoWaitSetRemove(wait_set.value(), cookie);
}

inline MojoResult WaitSetWait(WaitSetHandle wait_set,
                              MojoDeadline deadline,
                              std::vector<MojoWaitSetResult>* results,
                              uint32_t* max_results) {
  if (!results) {
    return MojoWaitSetWait(wait_set.value(), deadline, 0u, nullptr,
                           max_results);
  }

  assert(results->capacity() <= static_cast<uint32_t>(-1));
  uint32_t num_results = static_cast<uint32_t>(results->capacity());
  results->resize(num_results);
  MojoResult rv = MojoWaitSetWait(wait_set.value(), deadline, &num_results,
                                  results->data(), max_results);
  results->resize((rv == MOJO_RESULT_OK) ? num_results : 0u);
  return rv;
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_WAIT_SET_H_
