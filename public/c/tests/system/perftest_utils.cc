// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/tests/system/perftest_utils.h"

#include <assert.h>
#include <mojo/macros.h>
#include <mojo/system/time.h>
#include <stddef.h>
#include <time.h>

#include "mojo/public/cpp/test_support/test_support.h"

namespace mojo {
namespace test {

// Iterates the given function for |kPerftestTimeMicroseconds| and reports the
// number of iterations executed per second.
void IterateAndReportPerf(const char* test_name,
                          const char* sub_test_name,
                          std::function<void()> single_iteration) {
  // TODO(vtl): These should be specifiable using command-line flags.
  static constexpr size_t kGranularity = 100u;

  const MojoTimeTicks start_time = MojoGetTimeTicksNow();
  MojoTimeTicks end_time;
  size_t iterations = 0u;
  do {
    for (size_t i = 0u; i < kGranularity; i++)
      single_iteration();
    iterations += kGranularity;

    end_time = MojoGetTimeTicksNow();
  } while (end_time - start_time < kPerftestTimeMicroseconds);

  LogPerfResult(test_name, sub_test_name,
                1000000.0 * iterations / (end_time - start_time),
                "iterations/second");
}

void Sleep(MojoTimeTicks microseconds) {
  struct timespec req = {
      static_cast<time_t>(microseconds / 1000000),       // Seconds.
      static_cast<long>(microseconds % 1000000) * 1000L  // Nanoseconds.
  };
  int rv = nanosleep(&req, nullptr);
  MOJO_ALLOW_UNUSED_LOCAL(rv);
  assert(rv == 0);
}

}  // namespace test
}  // namespace mojo
