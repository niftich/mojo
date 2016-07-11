// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This tests the performance of message pipes via the C API.

#include "mojo/public/c/system/message_pipe.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <thread>

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/tests/perftest_utils.h"
#include "mojo/public/c/system/time.h"
#include "mojo/public/c/system/wait.h"
#include "mojo/public/cpp/test_support/test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(MessagePipePerftest, CreateAndClose) {
  mojo::test::IterateAndReportPerf("MessagePipe_CreateAndClose", nullptr, []() {
    MojoHandle h0;
    MojoHandle h1;
    MojoResult result = MojoCreateMessagePipe(nullptr, &h0, &h1);
    MOJO_ALLOW_UNUSED_LOCAL(result);
    assert(result == MOJO_RESULT_OK);
    result = MojoClose(h0);
    assert(result == MOJO_RESULT_OK);
    result = MojoClose(h1);
    assert(result == MOJO_RESULT_OK);
  });
}

TEST(MessagePipePerftest, WriteAndRead) {
  MojoHandle h0;
  MojoHandle h1;
  MojoResult result = MojoCreateMessagePipe(nullptr, &h0, &h1);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
  char buffer[10000] = {};
  uint32_t num_bytes = 0u;
  auto single_iteration = [&h0, &h1, &buffer, &num_bytes]() {
    MojoResult result = MojoWriteMessage(h0, buffer, num_bytes, nullptr, 0,
                                         MOJO_WRITE_MESSAGE_FLAG_NONE);
    MOJO_ALLOW_UNUSED_LOCAL(result);
    assert(result == MOJO_RESULT_OK);
    uint32_t read_bytes = num_bytes;
    result = MojoReadMessage(h1, buffer, &read_bytes, nullptr, nullptr,
                             MOJO_READ_MESSAGE_FLAG_NONE);
    assert(result == MOJO_RESULT_OK);
  };
  num_bytes = 10u;
  mojo::test::IterateAndReportPerf("MessagePipe_WriteAndRead", "10bytes",
                                   single_iteration);
  num_bytes = 100u;
  mojo::test::IterateAndReportPerf("MessagePipe_WriteAndRead", "100bytes",
                                   single_iteration);
  num_bytes = 1000u;
  mojo::test::IterateAndReportPerf("MessagePipe_WriteAndRead", "1000bytes",
                                   single_iteration);
  num_bytes = 10000u;
  mojo::test::IterateAndReportPerf("MessagePipe_WriteAndRead", "10000bytes",
                                   single_iteration);
  result = MojoClose(h0);
  assert(result == MOJO_RESULT_OK);
  result = MojoClose(h1);
  assert(result == MOJO_RESULT_OK);
}

TEST(MessagePipePerftest, EmptyRead) {
  MojoHandle h0;
  MojoHandle h1;
  MojoResult result = MojoCreateMessagePipe(nullptr, &h0, &h1);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
  mojo::test::IterateAndReportPerf("MessagePipe_EmptyRead", nullptr, [&h0]() {
    MojoResult result = MojoReadMessage(h0, nullptr, nullptr, nullptr, nullptr,
                                        MOJO_READ_MESSAGE_FLAG_MAY_DISCARD);
    MOJO_ALLOW_UNUSED_LOCAL(result);
    assert(result == MOJO_RESULT_SHOULD_WAIT);
  });
  result = MojoClose(h0);
  assert(result == MOJO_RESULT_OK);
  result = MojoClose(h1);
  assert(result == MOJO_RESULT_OK);
}

void DoMessagePipeThreadedTest(unsigned num_writers,
                               unsigned num_readers,
                               uint32_t num_bytes) {
  assert(num_writers > 0u);
  assert(num_readers > 0u);

  MojoHandle h0;
  MojoHandle h1;
  MojoResult result = MojoCreateMessagePipe(nullptr, &h0, &h1);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);

  // Each |writers[i]| will write its final result to |num_writes[i]|.
  std::vector<std::thread> writers(num_writers);
  std::vector<int64_t> num_writes(num_writers, 0);

  // Similarly for |readers[i]| and |num_reads[i]|.
  std::vector<std::thread> readers(num_readers);
  std::vector<int64_t> num_reads(num_readers, 0);

  // Start time here, just before we fire off the threads.
  const MojoTimeTicks start_time = MojoGetTimeTicksNow();

  // Interleave the starts.
  for (unsigned i = 0u; i < num_writers || i < num_readers; i++) {
    if (i < num_writers) {
      int64_t* final_num_writes = &num_writes[i];
      writers[i] = std::thread([num_bytes, h0, final_num_writes]() {
        int64_t num_writes = 0;
        char buffer[10000];
        assert(num_bytes <= sizeof(buffer));

        // TODO(vtl): Should I throttle somehow?
        for (;;) {
          MojoResult result = MojoWriteMessage(
              h0, buffer, num_bytes, nullptr, 0u, MOJO_WRITE_MESSAGE_FLAG_NONE);
          if (result == MOJO_RESULT_OK) {
            num_writes++;
            continue;
          }

          // We failed to write.
          // Either |h0| or its peer was closed.
          assert(result == MOJO_RESULT_INVALID_ARGUMENT ||
                 result == MOJO_RESULT_FAILED_PRECONDITION);
          break;
        }
        *final_num_writes = num_writes;
      });
    }
    if (i < num_readers) {
      int64_t* final_num_reads = &num_reads[i];
      readers[i] = std::thread([h1, final_num_reads]() {
        int64_t num_reads = 0;
        char buffer[10000];

        for (;;) {
          uint32_t num_bytes = static_cast<uint32_t>(sizeof(buffer));
          MojoResult result =
              MojoReadMessage(h1, buffer, &num_bytes, nullptr, nullptr,
                              MOJO_READ_MESSAGE_FLAG_NONE);
          if (result == MOJO_RESULT_OK) {
            num_reads++;
            continue;
          }

          if (result == MOJO_RESULT_SHOULD_WAIT) {
            result = MojoWait(h1, MOJO_HANDLE_SIGNAL_READABLE,
                              MOJO_DEADLINE_INDEFINITE, nullptr);
            if (result == MOJO_RESULT_OK) {
              // Go to the top of the loop to read again.
              continue;
            }
          }

          // We failed to read and possibly failed to wait.
          // Either |h1| or its peer was closed.
          assert(result == MOJO_RESULT_INVALID_ARGUMENT ||
                 result == MOJO_RESULT_FAILED_PRECONDITION);
          break;
        }
        *final_num_reads = num_reads;
      });
    }
  }

  mojo::test::Sleep(mojo::test::kPerftestTimeMicroseconds);

  // Close both handles to make writers and readers stop immediately.
  result = MojoClose(h0);
  assert(result == MOJO_RESULT_OK);
  result = MojoClose(h1);
  assert(result == MOJO_RESULT_OK);

  // Join everything.
  for (auto& writer : writers)
    writer.join();
  for (auto& reader : readers)
    reader.join();

  // Stop time here.
  MojoTimeTicks end_time = MojoGetTimeTicksNow();

  // Add up write and read counts, and destroy the threads.
  int64_t total_num_writes = 0;
  for (auto n : num_writes)
    total_num_writes += n;
  int64_t total_num_reads = 0;
  for (auto n : num_reads)
    total_num_reads += n;

  char sub_test_name[200];
  sprintf(sub_test_name, "%uw_%ur_%ubytes", num_writers, num_readers,
          static_cast<unsigned>(num_bytes));
  mojo::test::LogPerfResult("MessagePipe_Threaded_Writes", sub_test_name,
                            1000000.0 * static_cast<double>(total_num_writes) /
                                (end_time - start_time),
                            "writes/second");
  mojo::test::LogPerfResult("MessagePipe_Threaded_Reads", sub_test_name,
                            1000000.0 * static_cast<double>(total_num_reads) /
                                (end_time - start_time),
                            "reads/second");
}

TEST(MessagePipePerftest, Threaded) {
  DoMessagePipeThreadedTest(1u, 1u, 100u);
  DoMessagePipeThreadedTest(2u, 2u, 100u);
  DoMessagePipeThreadedTest(3u, 3u, 100u);
  DoMessagePipeThreadedTest(10u, 10u, 100u);
  DoMessagePipeThreadedTest(10u, 1u, 100u);
  DoMessagePipeThreadedTest(1u, 10u, 100u);

  // For comparison of overhead:
  DoMessagePipeThreadedTest(1u, 1u, 10u);
  // 100 was done above.
  DoMessagePipeThreadedTest(1u, 1u, 1000u);
  DoMessagePipeThreadedTest(1u, 1u, 10000u);

  DoMessagePipeThreadedTest(3u, 3u, 10u);
  // 100 was done above.
  DoMessagePipeThreadedTest(3u, 3u, 1000u);
  DoMessagePipeThreadedTest(3u, 3u, 10000u);
}

}  // namespace
