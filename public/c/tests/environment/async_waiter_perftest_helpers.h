// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_TESTS_ENVIRONMENT_ASYNC_WAITER_PERFTEST_HELPERS_H_
#define MOJO_PUBLIC_C_TESTS_ENVIRONMENT_ASYNC_WAITER_PERFTEST_HELPERS_H_

#include <mojo/environment/async_waiter.h>
#include <stdint.h>

#include <functional>

namespace mojo {
namespace test {

// Basic single-threaded perf test helper for |MojoAsyncWaiter|. This will keep
// |num_handles| active async waits and activate them one at a time (with each
// callback triggering the next). This sets up the test (setting up the initial
// set of async waits), calls |run_loop_function|, and once that is done cleans
// up. It returns the number of async waits completed. |run_loop_function|
// should typically run the message/run loop and quit after some fixed amount of
// time (it should probably measure the actual amount of time spent running, and
// the return value should probably be normalized with respect to that).
uint64_t DoAsyncWaiterPerfTest(const MojoAsyncWaiter* async_waiter,
                               uint32_t num_handles,
                               std::function<void()> run_loop_function);

}  // namespace test
}  // namespace mojo

#endif  // MOJO_PUBLIC_C_TESTS_ENVIRONMENT_ASYNC_WAITER_PERFTEST_HELPERS_H_
