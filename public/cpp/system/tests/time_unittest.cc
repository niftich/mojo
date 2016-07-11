// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C++ wrappers in mojo/public/cpp/system/time.h.

#include "mojo/public/cpp/system/time.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

TEST(TimeTest, GetTimeTicksNow) {
  const MojoTimeTicks start = GetTimeTicksNow();
  EXPECT_NE(static_cast<MojoTimeTicks>(0), start);
}

}  // namespace
}  // namespace mojo
