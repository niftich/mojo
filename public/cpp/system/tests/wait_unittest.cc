// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C++ wrappers in mojo/public/cpp/system/wait.h.

#include "mojo/public/cpp/system/wait.h"

#include <vector>

#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

TEST(WaitTest, InvalidArgs) {
  ScopedHandle h;

  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            Wait(h.get(), ~MOJO_HANDLE_SIGNAL_NONE, 1000000, nullptr));

  std::vector<Handle> wh;
  wh.push_back(h.get());
  std::vector<MojoHandleSignals> sigs;
  sigs.push_back(~MOJO_HANDLE_SIGNAL_NONE);
  WaitManyResult wait_many_result =
      WaitMany(wh, sigs, MOJO_DEADLINE_INDEFINITE, nullptr);
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, wait_many_result.result);
  EXPECT_TRUE(wait_many_result.IsIndexValid());
  EXPECT_FALSE(wait_many_result.AreSignalsStatesValid());
}

TEST(WaitTest, TimeOut) {
  // |Wait()|:
  {
    // Need a valid handle to wait on for |Wait()|.
    MessagePipe mp;
    EXPECT_EQ(
        MOJO_RESULT_DEADLINE_EXCEEDED,
        Wait(mp.handle0.get(), MOJO_HANDLE_SIGNAL_READABLE, 100u, nullptr));
  }

  // |WaitMany()|:
  {
    std::vector<Handle> wh;
    std::vector<MojoHandleSignals> sigs;
    WaitManyResult wait_many_result = WaitMany(wh, sigs, 100u, nullptr);
    EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED, wait_many_result.result);
    EXPECT_FALSE(wait_many_result.IsIndexValid());
    EXPECT_TRUE(wait_many_result.AreSignalsStatesValid());
  }
}

TEST(WaitTest, WaitManyResult) {
  {
    WaitManyResult wmr(MOJO_RESULT_OK);
    EXPECT_FALSE(wmr.IsIndexValid());
    EXPECT_TRUE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_OK, wmr.result);
  }

  {
    WaitManyResult wmr(MOJO_RESULT_FAILED_PRECONDITION);
    EXPECT_FALSE(wmr.IsIndexValid());
    EXPECT_TRUE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, wmr.result);
  }

  {
    WaitManyResult wmr(MOJO_RESULT_INVALID_ARGUMENT);
    EXPECT_FALSE(wmr.IsIndexValid());
    EXPECT_FALSE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, wmr.result);
  }

  // These should be like "invalid argument".
  EXPECT_FALSE(
      WaitManyResult(MOJO_RESULT_RESOURCE_EXHAUSTED).AreSignalsStatesValid());
  EXPECT_FALSE(WaitManyResult(MOJO_RESULT_BUSY).AreSignalsStatesValid());

  {
    WaitManyResult wmr(MOJO_RESULT_OK, 5u);
    EXPECT_TRUE(wmr.IsIndexValid());
    EXPECT_TRUE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_OK, wmr.result);
    EXPECT_EQ(5u, wmr.index);
  }

  {
    WaitManyResult wmr(MOJO_RESULT_FAILED_PRECONDITION, 5u);
    EXPECT_TRUE(wmr.IsIndexValid());
    EXPECT_TRUE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, wmr.result);
    EXPECT_EQ(5u, wmr.index);
  }
}

}  // namespace
}  // namespace mojo
