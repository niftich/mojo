// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/test_support/test_support.h"

#include <stdio.h>

namespace mojo {
namespace test {

void LogPerfResult(const char* test_name,
                   const char* sub_test_name,
                   double value,
                   const char* units) {
  if (sub_test_name)
    printf("%s %s %.3f %s\n", test_name, sub_test_name, value, units);
  else
    printf("%s %.3f %s\n", test_name, value, units);
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
