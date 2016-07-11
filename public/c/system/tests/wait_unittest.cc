// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C wait API (the functions declared in
// mojo/public/c/system/wait.h).

#include "mojo/public/c/system/wait.h"

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(WaitTest, InvalidHandle) {
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWait(MOJO_HANDLE_INVALID, ~MOJO_HANDLE_SIGNAL_NONE, 1000000u,
                     nullptr));

  const MojoHandle h = MOJO_HANDLE_INVALID;
  MojoHandleSignals sig = ~MOJO_HANDLE_SIGNAL_NONE;
  EXPECT_EQ(
      MOJO_RESULT_INVALID_ARGUMENT,
      MojoWaitMany(&h, &sig, 1u, MOJO_DEADLINE_INDEFINITE, nullptr, nullptr));
}

TEST(WaitTest, WaitManyNoHandles) {
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoWaitMany(nullptr, nullptr, 0u, static_cast<MojoDeadline>(0),
                         nullptr, nullptr));

  // The |result_index| argument is optional, so make sure it doesn't touch it
  // even if it's non-null.
  // TODO(vtl): The same is true for the |signals_states| argument.
  uint32_t result_index = static_cast<uint32_t>(-1);
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoWaitMany(nullptr, nullptr, 0u, static_cast<MojoDeadline>(1000),
                         &result_index, nullptr));
  EXPECT_EQ(static_cast<uint32_t>(-1), result_index);
}

// TODO(vtl): Write tests that actually test waiting.

}  // namespace
