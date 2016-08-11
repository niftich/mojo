// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_SYSTEM_WAIT_H_
#define MOJO_PUBLIC_CPP_SYSTEM_WAIT_H_

#include <mojo/result.h>
#include <mojo/system/handle.h>
#include <mojo/system/time.h>
#include <mojo/system/wait.h>
#include <stdint.h>

#include <vector>

#include "mojo/public/cpp/system/handle.h"

namespace mojo {

inline MojoResult Wait(Handle handle,
                       MojoHandleSignals signals,
                       MojoDeadline deadline,
                       MojoHandleSignalsState* signals_state) {
  return MojoWait(handle.value(), signals, deadline, signals_state);
}

const uint32_t kInvalidWaitManyIndexValue = static_cast<uint32_t>(-1);

// Simplify the interpretation of the output from |MojoWaitMany()|.
struct WaitManyResult {
  explicit WaitManyResult(MojoResult mojo_wait_many_result)
      : result(mojo_wait_many_result), index(kInvalidWaitManyIndexValue) {}

  WaitManyResult(MojoResult mojo_wait_many_result, uint32_t result_index)
      : result(mojo_wait_many_result), index(result_index) {}

  // A valid handle index is always returned if |WaitMany()| succeeds, but may
  // or may not be returned if |WaitMany()| returns an error. Use this helper
  // function to check if |index| is a valid index into the handle array.
  bool IsIndexValid() const { return index != kInvalidWaitManyIndexValue; }

  // The |signals_states| array is always returned by |WaitMany()| on success,
  // but may or may not be returned if |WaitMany()| returns an error. Use this
  // helper function to check if |signals_states| holds valid data.
  bool AreSignalsStatesValid() const {
    return result != MOJO_RESULT_INVALID_ARGUMENT &&
           result != MOJO_RESULT_RESOURCE_EXHAUSTED &&
           result != MOJO_RESULT_BUSY;
  }

  MojoResult result;
  uint32_t index;
};

// |HandleType| should be |Handle| or a "trivial" subclass thereof, like
// |MessagePipeHandle|, etc.
template <class HandleType>
inline WaitManyResult WaitMany(
    const std::vector<HandleType>& handles,
    const std::vector<MojoHandleSignals>& signals,
    MojoDeadline deadline,
    std::vector<MojoHandleSignalsState>* signals_states) {
  // We rely on being able to treat a vector of |HandleType|s as if it's an
  // array of |MojoHandle|s.
  static_assert(sizeof(HandleType) == sizeof(Handle),
                "HandleType is not the same size as Handle");

  if (signals.size() != handles.size() ||
      (signals_states && signals_states->size() != signals.size()))
    return WaitManyResult(MOJO_RESULT_INVALID_ARGUMENT);
  if (handles.size() >= kInvalidWaitManyIndexValue)
    return WaitManyResult(MOJO_RESULT_RESOURCE_EXHAUSTED);

  if (handles.size() == 0) {
    return WaitManyResult(
        MojoWaitMany(nullptr, nullptr, 0, deadline, nullptr, nullptr));
  }

  uint32_t result_index = kInvalidWaitManyIndexValue;
  MojoResult result = MojoWaitMany(
      &handles[0].value(), signals.data(),
      static_cast<uint32_t>(handles.size()), deadline, &result_index,
      signals_states ? signals_states->data() : nullptr);
  return WaitManyResult(result, result_index);
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_WAIT_H_
