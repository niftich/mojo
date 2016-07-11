// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_TESTS_PERFTEST_UTILS_H_
#define MOJO_PUBLIC_C_SYSTEM_TESTS_PERFTEST_UTILS_H_

#include <functional>

#include "mojo/public/c/system/time.h"

namespace mojo {
namespace test {

// How long to run each perf test for (in microseconds).
constexpr MojoTimeTicks kPerftestTimeMicroseconds = 3 * 1000000;

// Iterates the given function for |kPerftestTimeMicroseconds| and reports the
// number of iterations executed per second.
void IterateAndReportPerf(const char* test_name,
                          const char* sub_test_name,
                          std::function<void()> single_iteration);

// Sleeps for the given amount of time (in microseconds).
void Sleep(MojoTimeTicks microseconds);

}  // namespace test
}  // namespace mojo

#endif  // MOJO_PUBLIC_C_SYSTEM_TESTS_PERFTEST_UTILS_H_
