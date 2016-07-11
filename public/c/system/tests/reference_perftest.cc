// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This provides a reference for comparison of performance versus overhead in
// perftests.

#include "mojo/public/c/system/tests/perftest_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// A no-op test so we can compare performance.
TEST(ReferencePerftest, NoOp) {
  mojo::test::IterateAndReportPerf("Reference_NoOp", nullptr, []() {});
}

}  // namespace
