// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C wait set API (the functions declared in
// mojo/public/c/system/wait_set.h).

#include "mojo/public/c/system/wait_set.h"

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/result.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const MojoHandleRights kDefaultWaitSetHandleRights =
    MOJO_HANDLE_RIGHT_READ | MOJO_HANDLE_RIGHT_WRITE |
    MOJO_HANDLE_RIGHT_GET_OPTIONS | MOJO_HANDLE_RIGHT_SET_OPTIONS;

TEST(WaitSetTest, InvalidHandle) {
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWaitSetAdd(MOJO_HANDLE_INVALID, MOJO_HANDLE_INVALID,
                           MOJO_HANDLE_SIGNAL_READABLE, 123u, nullptr));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWaitSetRemove(MOJO_HANDLE_INVALID, 123u));
  uint32_t num_results = 10u;
  MojoWaitSetResult results[10] = {};
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWaitSetWait(MOJO_HANDLE_INVALID, MOJO_DEADLINE_INDEFINITE,
                            &num_results, results, nullptr));

  // Also check |MojoWaitSetAdd()| with a valid handle to be added.
  MojoHandle mph0 = MOJO_HANDLE_INVALID;
  MojoHandle mph1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &mph0, &mph1));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWaitSetAdd(MOJO_HANDLE_INVALID, mph0,
                           MOJO_HANDLE_SIGNAL_READABLE, 123u, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mph0));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mph1));
}

TEST(WaitSetTest, Create) {
  // Invalid options.
  {
    static constexpr MojoCreateWaitSetOptions kOptions = {};
    MojoHandle h = MOJO_HANDLE_INVALID;
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoCreateWaitSet(&kOptions, &h));
    EXPECT_EQ(MOJO_HANDLE_INVALID, h);
  }

  // Options with unknown flags.
  {
    static constexpr MojoCreateWaitSetOptions kOptions = {
        static_cast<uint32_t>(sizeof(MojoCreateWaitSetOptions)),
        ~static_cast<MojoCreateWaitSetOptionsFlags>(0),
    };
    MojoHandle h = MOJO_HANDLE_INVALID;
    EXPECT_EQ(MOJO_RESULT_UNIMPLEMENTED, MojoCreateWaitSet(&kOptions, &h));
    EXPECT_EQ(MOJO_HANDLE_INVALID, h);
  }

  // With non-null options.
  {
    static constexpr MojoCreateWaitSetOptions kOptions = {
        static_cast<uint32_t>(sizeof(MojoCreateWaitSetOptions)),
        static_cast<MojoCreateWaitSetOptionsFlags>(0),
    };
    MojoHandle h = MOJO_HANDLE_INVALID;
    EXPECT_EQ(MOJO_RESULT_OK, MojoCreateWaitSet(&kOptions, &h));
    EXPECT_NE(h, MOJO_HANDLE_INVALID);

    // Should have the correct rights.
    MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
    EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h, &rights));
    EXPECT_EQ(kDefaultWaitSetHandleRights, rights);

    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h));
  }

  // With null options.
  {
    MojoHandle h = MOJO_HANDLE_INVALID;
    EXPECT_EQ(MOJO_RESULT_OK, MojoCreateWaitSet(nullptr, &h));
    EXPECT_NE(h, MOJO_HANDLE_INVALID);

    // Should have the correct rights.
    MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
    EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h, &rights));
    EXPECT_EQ(kDefaultWaitSetHandleRights, rights);

    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h));
  }
}

TEST(WaitSetTest, Add) {
  MojoHandle h = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateWaitSet(nullptr, &h));
  EXPECT_NE(h, MOJO_HANDLE_INVALID);

  // Add invalid handle.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWaitSetAdd(h, MOJO_HANDLE_INVALID, MOJO_HANDLE_SIGNAL_READABLE,
                           0u, nullptr));

  // Some handles that we can add.
  MojoHandle mph0 = MOJO_HANDLE_INVALID;
  MojoHandle mph1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &mph0, &mph1));

  // Add with invalid options.
  {
    static constexpr MojoWaitSetAddOptions kOptions = {};
    EXPECT_EQ(
        MOJO_RESULT_INVALID_ARGUMENT,
        MojoWaitSetAdd(h, mph0, MOJO_HANDLE_SIGNAL_READABLE, 0u, &kOptions));
  }

  // Add with options with unknown flags.
  {
    static constexpr MojoWaitSetAddOptions kOptions = {
        static_cast<uint32_t>(sizeof(MojoWaitSetAddOptions)),
        ~static_cast<MojoWaitSetAddOptionsFlags>(0),
    };
    EXPECT_EQ(
        MOJO_RESULT_UNIMPLEMENTED,
        MojoWaitSetAdd(h, mph0, MOJO_HANDLE_SIGNAL_READABLE, 0u, &kOptions));
  }

  // Add with options.
  {
    static constexpr MojoWaitSetAddOptions kOptions = {
        static_cast<uint32_t>(sizeof(MojoWaitSetAddOptions)),
        static_cast<MojoWaitSetAddOptionsFlags>(0),
    };
    EXPECT_EQ(
        MOJO_RESULT_OK,
        MojoWaitSetAdd(h, mph0, MOJO_HANDLE_SIGNAL_READABLE, 0u, &kOptions));
  }

  // Add with null options.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitSetAdd(h, mph1, MOJO_HANDLE_SIGNAL_WRITABLE, 1u, nullptr));

  // Add a handle that's already present, with a different cookie.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitSetAdd(h, mph0, MOJO_HANDLE_SIGNAL_READABLE, 2u, nullptr));

  // Try to add a cookie that's already present.
  EXPECT_EQ(MOJO_RESULT_ALREADY_EXISTS,
            MojoWaitSetAdd(h, mph1, MOJO_HANDLE_SIGNAL_READABLE, 0u, nullptr));

  // Can close things in a wait set.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mph0));

  // Can close a wait set with unclosed handles in it.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mph1));
}

TEST(WaitSetTest, Remove) {
  MojoHandle h = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateWaitSet(nullptr, &h));
  EXPECT_NE(h, MOJO_HANDLE_INVALID);

  // Some handles that we can add.
  MojoHandle mph0 = MOJO_HANDLE_INVALID;
  MojoHandle mph1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &mph0, &mph1));

  // Try to remove something that's not there.
  EXPECT_EQ(MOJO_RESULT_NOT_FOUND, MojoWaitSetRemove(h, 12u));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitSetAdd(h, mph0, MOJO_HANDLE_SIGNAL_READABLE, 12u, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitSetAdd(h, mph1, MOJO_HANDLE_SIGNAL_READABLE, 34u, nullptr));

  // Remove something.
  EXPECT_EQ(MOJO_RESULT_OK, MojoWaitSetRemove(h, 12u));

  // Can't remove it again.
  EXPECT_EQ(MOJO_RESULT_NOT_FOUND, MojoWaitSetRemove(h, 12u));

  // Now can add it again.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitSetAdd(h, mph0, MOJO_HANDLE_SIGNAL_WRITABLE, 12u, nullptr));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mph0));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mph1));
}

// Helper to check if an array of |MojoWaitSetResult|s has a result |r| for the
// given cookie, in which case:
//    - |r.wait_result| must equal |wait_result|.
//    - If |wait_result| is |MOJO_RESULT_OK| or
//      |MOJO_RESULT_FAILED_PRECONDITION|, then
//        - |r.signals_state.satisfied_signals & signals| must equal
//          |signals_state.satisfied_signals & signals|, and
//        - |r.signals_state.satisfiable & signals| must equal
//          |signals_state.satisfiable_signals & signals|.
//    - Otherwise, |r.signals_state| must equals |signals_state|.
// (This doesn't check that the result is unique; you should check |num_results|
// versus the expect number and exhaustively check every expected result.)
bool CheckHasResult(uint32_t num_results,
                    const MojoWaitSetResult* results,
                    uint64_t cookie,
                    MojoHandleSignals signals,
                    MojoResult wait_result,
                    const MojoHandleSignalsState& signals_state) {
  for (uint32_t i = 0; i < num_results; i++) {
    if (results[i].cookie == cookie) {
      EXPECT_EQ(wait_result, results[i].wait_result) << cookie;
      EXPECT_EQ(0u, results[i].reserved) << cookie;
      if (wait_result == MOJO_RESULT_OK ||
          wait_result == MOJO_RESULT_FAILED_PRECONDITION) {
        EXPECT_EQ(signals_state.satisfied_signals & signals,
                  results[i].signals_state.satisfied_signals & signals)
            << cookie;
        EXPECT_EQ(signals_state.satisfiable_signals & signals,
                  results[i].signals_state.satisfiable_signals & signals)
            << cookie;
      } else {
        EXPECT_EQ(signals_state.satisfied_signals,
                  results[i].signals_state.satisfied_signals)
            << cookie;
        EXPECT_EQ(signals_state.satisfiable_signals,
                  results[i].signals_state.satisfiable_signals)
            << cookie;
      }
      return true;
    }
  }
  return false;
}

TEST(WaitSetTest, Wait) {
  MojoHandle h = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateWaitSet(nullptr, &h));
  EXPECT_NE(h, MOJO_HANDLE_INVALID);

  // Nothing in the wait set.
  {
    uint32_t num_results = 10u;
    MojoWaitSetResult results[10] = {};
    uint32_t max_results = 1234u;
    EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
              MojoWaitSetWait(h, static_cast<MojoDeadline>(0), &num_results,
                              results, &max_results));
    EXPECT_EQ(10u, num_results);
    EXPECT_EQ(1234u, max_results);
  }

  // Ditto, with non-zero deadline and null |max_results|.
  {
    uint32_t num_results = 10u;
    MojoWaitSetResult results[10] = {};
    EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
              MojoWaitSetWait(h, static_cast<MojoDeadline>(1000), &num_results,
                              results, nullptr));
    EXPECT_EQ(10u, num_results);
  }

  // Some handles that we can add.
  MojoHandle mph0 = MOJO_HANDLE_INVALID;
  MojoHandle mph1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &mph0, &mph1));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitSetAdd(h, mph0, MOJO_HANDLE_SIGNAL_READABLE, 1u, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitSetAdd(h, mph1, MOJO_HANDLE_SIGNAL_READABLE, 2u, nullptr));

  // Will still time out.
  {
    uint32_t num_results = 10u;
    MojoWaitSetResult results[10] = {};
    EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
              MojoWaitSetWait(h, static_cast<MojoDeadline>(0), &num_results,
                              results, nullptr));
    EXPECT_EQ(10u, num_results);
  }

  // Write to |mph1|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteMessage(mph1, nullptr, 0, nullptr, 0,
                                             MOJO_WRITE_MESSAGE_FLAG_NONE));

  // Should get cookie 1.
  {
    uint32_t num_results = 10u;
    MojoWaitSetResult results[10] = {};
    uint32_t max_results = 1234u;
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWaitSetWait(h, static_cast<MojoDeadline>(0), &num_results,
                              results, &max_results));
    EXPECT_EQ(1u, num_results);
    EXPECT_TRUE(CheckHasResult(
        num_results, results, 1u, MOJO_HANDLE_SIGNAL_READABLE, MOJO_RESULT_OK,
        MojoHandleSignalsState{
            MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
            MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE}));
    EXPECT_EQ(1u, max_results);
  }

  // Non-zero deadline, null |max_results|; should still get cookie 1.
  {
    uint32_t num_results = 10u;
    MojoWaitSetResult results[10] = {};
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWaitSetWait(h, static_cast<MojoDeadline>(1000), &num_results,
                              results, nullptr));
    EXPECT_EQ(1u, num_results);
    EXPECT_TRUE(CheckHasResult(
        num_results, results, 1u, MOJO_HANDLE_SIGNAL_READABLE, MOJO_RESULT_OK,
        MojoHandleSignalsState{
            MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
            MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE}));
  }

  // Zero |num_results|.
  {
    uint32_t num_results = 0u;
    uint32_t max_results = 1234u;
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWaitSetWait(h, static_cast<MojoDeadline>(0), &num_results,
                              nullptr, &max_results));
    EXPECT_EQ(0u, num_results);
    EXPECT_EQ(1u, max_results);
  }

  // Add another entry waiting for readability on |mph0|.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitSetAdd(h, mph0, MOJO_HANDLE_SIGNAL_READABLE |
                                        MOJO_HANDLE_SIGNAL_WRITABLE,
                           3u, nullptr));

  {
    uint32_t num_results = 10u;
    uint32_t max_results = 1234u;
    MojoWaitSetResult results[10] = {};
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWaitSetWait(h, MOJO_DEADLINE_INDEFINITE, &num_results,
                              results, &max_results));
    EXPECT_EQ(2u, num_results);
    EXPECT_TRUE(CheckHasResult(
        num_results, results, 1u, MOJO_HANDLE_SIGNAL_READABLE, MOJO_RESULT_OK,
        MojoHandleSignalsState{
            MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
            MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE}));
    EXPECT_TRUE(CheckHasResult(
        num_results, results, 3u,
        MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
        MOJO_RESULT_OK,
        MojoHandleSignalsState{
            MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
            MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE}));
    EXPECT_EQ(2u, max_results);
  }

  // Close |mph0|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mph0));

  {
    uint32_t num_results = 10u;
    MojoWaitSetResult results[10] = {};
    EXPECT_EQ(MOJO_RESULT_OK, MojoWaitSetWait(h, MOJO_DEADLINE_INDEFINITE,
                                              &num_results, results, nullptr));
    EXPECT_EQ(3u, num_results);
    EXPECT_TRUE(
        CheckHasResult(num_results, results, 1u, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_RESULT_CANCELLED, MojoHandleSignalsState()));
    EXPECT_TRUE(CheckHasResult(
        num_results, results, 2u, MOJO_HANDLE_SIGNAL_WRITABLE,
        MOJO_RESULT_FAILED_PRECONDITION, MojoHandleSignalsState()));
    EXPECT_TRUE(CheckHasResult(
        num_results, results, 3u,
        MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
        MOJO_RESULT_CANCELLED, MojoHandleSignalsState()));
  }

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mph1));
}

// TODO(vtl): Add threaded tests, especially those that actually ... wait.

}  // namespace
