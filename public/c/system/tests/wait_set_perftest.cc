// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This tests the performance of wait sets via the C API.

#include "mojo/public/c/system/wait_set.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <atomic>
#include <thread>
#include <vector>

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/tests/perftest_utils.h"
#include "mojo/public/cpp/test_support/test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Creates a wait set, creates a bunch of message pipes, and populates the wait
// set with entries. There will be |num_entries| entries; the |i|-th entry will
// be given cookie |i| and will be watching for readability on the message pipe
// endpoint |h0s->at(i)| whose corresponding endpoint is |h1s->at(i)|.
void CreateAndPopulateWaitSet(unsigned num_entries,
                              MojoHandle* wait_set,
                              std::vector<MojoHandle>* h0s,
                              std::vector<MojoHandle>* h1s) {
  MojoResult result = MojoCreateWaitSet(nullptr, wait_set);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);

  assert(h0s->empty());
  h0s->resize(num_entries);
  assert(h1s->empty());
  h1s->resize(num_entries);

  for (unsigned i = 0; i < num_entries; i++) {
    result = MojoCreateMessagePipe(nullptr, &h0s->at(i), &h1s->at(i));
    assert(result == MOJO_RESULT_OK);

    result = MojoWaitSetAdd(*wait_set, h0s->at(i), MOJO_HANDLE_SIGNAL_READABLE,
                            static_cast<uint64_t>(i), nullptr);
    assert(result == MOJO_RESULT_OK);
  }
}

void Close(MojoHandle h) {
  MojoResult result = MojoClose(h);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
}

void DoWaitSetPretriggeredWaitTest(unsigned num_entries) {
  MojoHandle wait_set = MOJO_HANDLE_INVALID;
  std::vector<MojoHandle> h0s;
  std::vector<MojoHandle> h1s;
  CreateAndPopulateWaitSet(num_entries, &wait_set, &h0s, &h1s);

  char sub_test_name[100];
  sprintf(sub_test_name, "%uentries", num_entries);
  // We'll write to |h1s[n % num_entries]|.
  uint64_t n = 0;
  mojo::test::IterateAndReportPerf(
      "WaitSet_PretriggeredWait", sub_test_name,
      [num_entries, wait_set, &h0s, &h1s, &n]() {
        MojoResult result =
            MojoWriteMessage(h1s[n % num_entries], nullptr, 0, nullptr, 0,
                             MOJO_WRITE_MESSAGE_FLAG_NONE);
        MOJO_ALLOW_UNUSED_LOCAL(result);
        assert(result == MOJO_RESULT_OK);

        uint32_t num_results = 4;
        MojoWaitSetResult results[4];  // Note: Don't initialize |results[]|.
        result = MojoWaitSetWait(wait_set, static_cast<MojoDeadline>(1000),
                                 &num_results, results, nullptr);
        assert(result == MOJO_RESULT_OK);
        assert(num_results == 1u);
        assert(results[0].cookie == n % num_entries);

        result =
            MojoReadMessage(h0s[results[0].cookie], nullptr, nullptr, nullptr,
                            nullptr, MOJO_READ_MESSAGE_FLAG_MAY_DISCARD);
        assert(result == MOJO_RESULT_OK);

        n++;
      });

  for (auto h : h0s)
    Close(h);
  for (auto h : h1s)
    Close(h);
  Close(wait_set);
}

TEST(WaitSetPerftest, PretriggeredWait) {
  DoWaitSetPretriggeredWaitTest(10u);
  DoWaitSetPretriggeredWaitTest(100u);
  DoWaitSetPretriggeredWaitTest(1000u);
  DoWaitSetPretriggeredWaitTest(10000u);
}

void DoWaitSetThreadedWaitTest(unsigned num_entries) {
  MojoHandle wait_set = MOJO_HANDLE_INVALID;
  std::vector<MojoHandle> h0s;
  std::vector<MojoHandle> h1s;
  CreateAndPopulateWaitSet(num_entries, &wait_set, &h0s, &h1s);

  std::atomic<bool> done(false);
  // Number of outstanding message writes. We don't want the writer/waker thread
  // to get far ahead of the reader/waiter (main) thread.
  // TODO(vtl): Maybe also parametrize these tests based on the number of
  // outstanding writes?
  std::atomic<unsigned> outstanding(0u);

  std::thread writer_thread([num_entries, &h1s, &done, &outstanding]() {
    for (unsigned n = 0u; !done.load(); n = (n + 13u) % num_entries) {
      while (outstanding.load() > 5u) {
        // Assume a zero sleep is a yield.
        mojo::test::Sleep(static_cast<MojoTimeTicks>(0));
        if (done.load())
          return;
      }

      outstanding.fetch_add(1u);
      MojoResult result = MojoWriteMessage(h1s[n], nullptr, 0, nullptr, 0,
                                           MOJO_WRITE_MESSAGE_FLAG_NONE);
      MOJO_ALLOW_UNUSED_LOCAL(result);
      assert(result == MOJO_RESULT_OK);
    }
  });

  char sub_test_name[100];
  sprintf(sub_test_name, "%uentries", num_entries);
  // TODO(vtl): This reports iterations (wake-ups). Possibly, we should also
  // measure the number of results handled (total or per second).
  mojo::test::IterateAndReportPerf(
      "WaitSet_ThreadedWait", sub_test_name,
      [num_entries, wait_set, &h0s, &outstanding]() {
        uint32_t num_results = 10;
        MojoWaitSetResult results[10];  // Note: Don't initialize |results[]|.
        MojoResult result = MojoWaitSetWait(wait_set, MOJO_DEADLINE_INDEFINITE,
                                            &num_results, results, nullptr);
        MOJO_ALLOW_UNUSED_LOCAL(result);
        assert(result == MOJO_RESULT_OK);
        assert(num_results >= 1u);
        for (uint32_t i = 0u; i < num_results; i++) {
          assert(results[i].cookie < num_entries);
          result =
              MojoReadMessage(h0s[results[i].cookie], nullptr, nullptr, nullptr,
                              nullptr, MOJO_READ_MESSAGE_FLAG_MAY_DISCARD);
          assert(result == MOJO_RESULT_OK);
          outstanding.fetch_sub(1u);
        }
      });
  done.store(true);
  writer_thread.join();

  for (auto h : h0s)
    Close(h);
  for (auto h : h1s)
    Close(h);
  Close(wait_set);
}

TEST(WaitSetPerftest, ThreadedWait) {
  DoWaitSetThreadedWaitTest(10u);
  DoWaitSetThreadedWaitTest(100u);
  DoWaitSetThreadedWaitTest(1000u);
  DoWaitSetThreadedWaitTest(10000u);
}

}  // namespace
