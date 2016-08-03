// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has perf tests for async waits using |mojo::RunLoop| (i.e., the
// "standalone" |Environment|).

#include <stdio.h>

#include "mojo/public/c/environment/tests/async_waiter_perftest_helpers.h"
#include "mojo/public/cpp/environment/environment.h"
#include "mojo/public/cpp/test_support/test_support.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "third_party/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

constexpr MojoTimeTicks kPerftestTimeMicroseconds = 3 * 1000000;

TEST(RunLoopAsyncWaitPerftest, SingleThreaded) {
  for (uint32_t num_handles = 1u; num_handles <= 10000u; num_handles *= 10u) {
    RunLoop run_loop;

    MojoTimeTicks start_time = 0;
    MojoTimeTicks end_time = 0;
    uint64_t raw_result = test::DoAsyncWaiterPerfTest(
        Environment::GetDefaultAsyncWaiter(), num_handles,
        [&start_time, &end_time]() {
          RunLoop::current()->PostDelayedTask(
              []() { RunLoop::current()->Quit(); }, kPerftestTimeMicroseconds);
          start_time = MojoGetTimeTicksNow();
          RunLoop::current()->Run();
          end_time = MojoGetTimeTicksNow();
        });

    double result = static_cast<double>(raw_result) /
                    (static_cast<double>(end_time - start_time) / 1000000.0);
    char sub_test_name[100] = {};
    sprintf(sub_test_name, "%u", static_cast<unsigned>(num_handles));
    test::LogPerfResult("RunLoopAsyncWaitPerftest.SingleThreaded",
                        sub_test_name, result, "callbacks/second");
  }
}

}  // namespace
}  // namespace mojo
