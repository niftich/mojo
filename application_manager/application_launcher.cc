// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_launcher.h"

#include <magenta/processargs.h>
#include <magenta/types.h>
#include <mxio/util.h>

namespace mojo {
namespace {

constexpr char kMojoAppDir[] = "/boot/apps/";
constexpr char kMojoScheme[] = "mojo:";
constexpr size_t kMojoSchemeLength = sizeof(kMojoScheme) - 1;

size_t CloneStdStreams(mx_handle_t* handles, uint32_t* ids) {
  size_t index = 0;
  for (int fd = 0; fd < 3; fd++) {
    mx_status_t result = mxio_clone_fd(fd, fd, handles + index, ids + index);
    if (result <= 0)
      continue;
    index += result;
  }
  return index;
}

std::string GetPathFromApplicationName(const std::string& name) {
  if (name.find(kMojoScheme) == 0)
    return kMojoAppDir + name.substr(kMojoSchemeLength);
  return std::string();
}

}  // namespace

mtl::UniqueHandle LaunchApplication(
    const std::string& name,
    mojo::InterfaceRequest<mojo::Application> application_request) {
  std::string path = GetPathFromApplicationName(name);
  if (path.empty())
    return mtl::UniqueHandle();

  // We need room for:
  //
  //  * stdin/stdout/stderror
  //  * The shell handle
  //  * The process handle
  mx_handle_t child_handles[5 * MXIO_MAX_HANDLES];
  uint32_t ids[5 * MXIO_MAX_HANDLES];

  // TODO(abarth): Remove stdin, stdout, and stderr.
  size_t index = CloneStdStreams(child_handles, ids);
  mojo::Handle initial_handle = application_request.PassMessagePipe().release();
  child_handles[index] = static_cast<mx_handle_t>(initial_handle.value());
  ids[index] = MX_HND_TYPE_MOJO_SHELL;
  ++index;

  const char* path_arg = path.c_str();
  // TODO(abarth): Remove const_cast once MG-185 is fixed.
  char** argv = const_cast<char**>(&path_arg);
  return mtl::UniqueHandle(mxio_start_process_etc(
      path_arg, 1, argv, index, child_handles, ids));
}

}  // namespace mojo
