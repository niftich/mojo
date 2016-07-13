// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>

#include "mojo/application_manager/application_manager.h"
#include "mojo/public/cpp/utility/run_loop.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "error: Missing path to initial application\n");
    return 1;
  }

  const char* initial_app = argv[1];
  mojo::ApplicationManager manager;

  mojo::RunLoop loop;

  loop.PostDelayedTask([&]() {
    if (!manager.StartInitialApplication(initial_app))
      exit(1);
  }, 0);

  loop.Run();
  return 0;
}
