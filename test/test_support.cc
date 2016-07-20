// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/test_support/test_support.h"

namespace mojo {
namespace test {

void LogPerfResult(const char* test_name,
                   const char* sub_test_name,
                   double value,
                   const char* units) {
  // TODO(jamesr): implement.
}

FILE* OpenSourceRootRelativeFile(const std::string& relative_path) {
  // TODO(jamesr): implement.
  return nullptr;
}

std::vector<std::string> EnumerateSourceRootRelativeDirectory(
    const std::string& relative_path) {
  // TODO(jamesr): implement.
  return std::vector<std::string>();
}

}  // namespace test
}  // namespace mojo
