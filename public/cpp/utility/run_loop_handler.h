// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_UTILITY_RUN_LOOP_HANDLER_H_
#define MOJO_PUBLIC_CPP_UTILITY_RUN_LOOP_HANDLER_H_

#include <mojo/result.h>
#include <stdint.h>

#include "mojo/public/cpp/system/handle.h"

namespace mojo {

// Used by RunLoop to notify when a handle is either ready or has become
// invalid.
class RunLoopHandler {
 public:
  using Id = uint64_t;

  // Called when wait for the handle is ready (i.e., waiting yields
  // MOJO_RESULT_OK).
  virtual void OnHandleReady(Id id) = 0;

  // Called when waiting for the handle is no longer valid (i.e., waiting yields
  // the error |result|). This is also called with MOJO_RESULT_ABORTED if the
  // RunLoop is destroyed.
  virtual void OnHandleError(Id id, MojoResult result) = 0;

 protected:
  virtual ~RunLoopHandler() {}
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_UTILITY_RUN_LOOP_HANDLER_H_
