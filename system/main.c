// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <magenta/processargs.h>
#include <runtime/process.h>

#include "mojo/public/c/system/main.h"

int main(int argc, char** argv) {
  return MojoMain(mxr_process_get_handle(MX_HND_TYPE_MOJO_SHELL));
}
