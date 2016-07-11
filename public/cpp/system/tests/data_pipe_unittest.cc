// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C++ wrappers in mojo/public/cpp/system/data_pipe.h.

#include "mojo/public/cpp/system/data_pipe.h"

#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/system/wait.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

TEST(DataPipe, Basic) {
  // Cursory compilation tests of |MakeScopedHandle()| with data pipe handles.
  EXPECT_FALSE(MakeScopedHandle(DataPipeProducerHandle()).is_valid());
  EXPECT_FALSE(MakeScopedHandle(DataPipeConsumerHandle()).is_valid());

  ScopedDataPipeProducerHandle ph;
  ScopedDataPipeConsumerHandle ch;

  ASSERT_EQ(MOJO_RESULT_OK, CreateDataPipe(nullptr, &ph, &ch));
  ASSERT_TRUE(ph.get().is_valid());
  ASSERT_TRUE(ch.get().is_valid());

  uint32_t read_threshold = 123u;
  EXPECT_EQ(MOJO_RESULT_OK,
            GetDataPipeConsumerOptions(ch.get(), &read_threshold));
  EXPECT_EQ(0u, read_threshold);

  EXPECT_EQ(MOJO_RESULT_OK, SetDataPipeConsumerOptions(ch.get(), 2u));

  EXPECT_EQ(MOJO_RESULT_OK,
            GetDataPipeConsumerOptions(ch.get(), &read_threshold));
  EXPECT_EQ(2u, read_threshold);

  // Write a byte.
  static const char kA = 'A';
  uint32_t num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(ph.get(), &kA, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));

  // Waiting for "read threshold" should fail. (Wait a nonzero amount, in case
  // there's some latency.)
  MojoHandleSignalsState state;
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, &state));
  // ... but it should be readable.
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // Do a two-phase write of another byte.
  void* write_buffer = nullptr;
  num_bytes = 0u;
  ASSERT_EQ(MOJO_RESULT_OK,
            BeginWriteDataRaw(ph.get(), &write_buffer, &num_bytes,
                              MOJO_WRITE_DATA_FLAG_NONE));
  ASSERT_TRUE(write_buffer);
  ASSERT_GT(num_bytes, 0u);
  static_cast<char*>(write_buffer)[0] = 'B';
  EXPECT_EQ(MOJO_RESULT_OK, EndWriteDataRaw(ph.get(), 1u));

  // Now waiting for "read threshold" should succeed. (Wait a nonzero amount, in
  // case there's some latency.)
  state = MojoHandleSignalsState();
  EXPECT_EQ(MOJO_RESULT_OK,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // Read a byte.
  char read_byte = 'x';
  num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK, ReadDataRaw(ch.get(), &read_byte, &num_bytes,
                                        MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);
  EXPECT_EQ('A', read_byte);

  // Waiting for "read threshold" should now fail.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0, nullptr));

  // Reset the read threshold/options.
  EXPECT_EQ(MOJO_RESULT_OK, SetDataPipeConsumerOptionsToDefault(ch.get()));

  // Waiting for "read threshold" should now succeed.
  EXPECT_EQ(MOJO_RESULT_OK,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0, nullptr));

  // Do a two-phase read.
  const void* read_buffer = nullptr;
  num_bytes = 0u;
  ASSERT_EQ(MOJO_RESULT_OK, BeginReadDataRaw(ch.get(), &read_buffer, &num_bytes,
                                             MOJO_READ_DATA_FLAG_NONE));
  ASSERT_TRUE(read_buffer);
  ASSERT_EQ(1u, num_bytes);
  EXPECT_EQ('B', static_cast<const char*>(read_buffer)[0]);
  EXPECT_EQ(MOJO_RESULT_OK, EndReadDataRaw(ch.get(), 1u));

  // Waiting for "read" should now fail (time out).
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, nullptr));

  // Close the producer.
  ph.reset();

  // Waiting for "read" should now fail.
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, nullptr));
}

}  // namespace mojo
}  // namespace
