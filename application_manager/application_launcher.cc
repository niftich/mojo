// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_launcher.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <magenta/processargs.h>
#include <magenta/types.h>
#include <mxio/util.h>

#include "mojo/application_manager/application_manager.h"

namespace mojo {
namespace {

constexpr char kMojoAppDir[] = "/boot/apps/";
constexpr char kMojoScheme[] = "mojo:";
constexpr size_t kMojoSchemeLength = sizeof(kMojoScheme) - 1;

constexpr char kMojoMagic[] = "#!mojo ";
constexpr size_t kMojoMagicLength = sizeof(kMojoMagic) - 1;
constexpr size_t kMaxShebangLength = 2048;

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

mojo::InterfaceRequest<mojo::Application> LaunchWithContentHandler(
    ApplicationManager* manager,
    const std::string& path,
    mojo::InterfaceRequest<mojo::Application> request) {
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1)
    return request;
  std::string shebang(kMaxShebangLength, '\0');
  ssize_t count = read(fd, &shebang[0], shebang.length());
  close(fd);
  if (count == -1)
    return request;
  if (shebang.find(kMojoMagic) != 0)
    return request;
  size_t newline = shebang.find('\n', kMojoMagicLength);
  if (newline == std::string::npos)
    return request;
  std::string handler = shebang.substr(kMojoMagicLength,
                                       newline - kMojoMagicLength);
  URLResponsePtr response = URLResponse::New();
  response->status_code = 200;
  // TODO(abarth): Fill in the data pipe.
  manager->StartApplicationUsingContentHandler(
      handler, std::move(response), std::move(request));
  return nullptr;
}

// TODO(abarth): We should use the fd we opened in LaunchWithContentHandler
// rather than using the path again.
mtl::UniqueHandle LaunchWithProcess(
    const std::string& path,
    mojo::InterfaceRequest<mojo::Application> request) {
  // We need room for:
  //
  //  * stdin/stdout/stderror
  //  * The shell handle
  //  * The process handle
  mx_handle_t child_handles[5 * MXIO_MAX_HANDLES];
  uint32_t ids[5 * MXIO_MAX_HANDLES];

  // TODO(abarth): Remove stdin, stdout, and stderr.
  size_t index = CloneStdStreams(child_handles, ids);
  mojo::Handle initial_handle = request.PassMessagePipe().release();
  child_handles[index] = static_cast<mx_handle_t>(initial_handle.value());
  ids[index] = MX_HND_TYPE_APPLICATION_REQUEST;
  ++index;

  const char* path_arg = path.c_str();
  // TODO(abarth): Remove const_cast once MG-185 is fixed.
  char** argv = const_cast<char**>(&path_arg);
  return mtl::UniqueHandle(mxio_start_process_etc(
      path_arg, 1, argv, index, child_handles, ids));
}

}  // namespace

std::pair<bool, mtl::UniqueHandle> LaunchApplication(
    ApplicationManager* manager,
    const std::string& name,
    mojo::InterfaceRequest<mojo::Application> request) {
  std::string path = GetPathFromApplicationName(name);
  if (path.empty())
    return std::make_pair(false, mtl::UniqueHandle());

  request = LaunchWithContentHandler(manager, path, std::move(request));
  if (!request.is_pending()) {
    // LaunchWithContentHandler has consumed the interface request, which
    // means we succeeded in kicking off the load process.
    return std::make_pair(true, mtl::UniqueHandle());
  }

  mtl::UniqueHandle handle = LaunchWithProcess(path, std::move(request));
  bool success = handle.is_valid();
  return std::make_pair(success, std::move(handle));
}

}  // namespace mojo
