// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_launcher.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <launchpad/launchpad.h>
#include <magenta/processargs.h>
#include <magenta/types.h>
#include <mxio/util.h>

#include "lib/ftl/files/unique_fd.h"
#include "lib/ftl/logging.h"
#include "lib/mtl/data_pipe/files.h"
#include "lib/mtl/tasks/message_loop.h"
#include "mojo/application_manager/application_manager.h"
#include "mojo/public/cpp/system/data_pipe.h"

namespace mojo {
namespace {

constexpr char kMojoAppDir[] = "/boot/apps/";
constexpr char kMojoScheme[] = "mojo:";
constexpr size_t kMojoSchemeLength = sizeof(kMojoScheme) - 1;

constexpr char kMojoMagic[] = "#!mojo ";
constexpr size_t kMojoMagicLength = sizeof(kMojoMagic) - 1;
constexpr size_t kMaxShebangLength = 2048;

void Ignored(bool success) {
  // There's not much we can do with this success value. If we didn't succeed in
  // filling the data pipe with content, that could just mean the content
  // handler wasn't interested in the data and closed its end of the pipe before
  // reading all the data.
}

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
  ftl::UniqueFD fd(open(path.c_str(), O_RDONLY));
  if (!fd.is_valid())
    return request;
  std::string shebang(kMaxShebangLength, '\0');
  ssize_t count = read(fd.get(), &shebang[0], shebang.length());
  if (count == -1)
    return request;
  if (shebang.find(kMojoMagic) != 0)
    return request;
  size_t newline = shebang.find('\n', kMojoMagicLength);
  if (newline == std::string::npos)
    return request;
  if (lseek(fd.get(), 0, SEEK_SET) == -1)
    return request;
  std::string handler =
      shebang.substr(kMojoMagicLength, newline - kMojoMagicLength);
  URLResponsePtr response = URLResponse::New();
  response->status_code = 200;
  mojo::DataPipe data_pipe;
  response->body = std::move(data_pipe.consumer_handle);
  mtl::CopyFromFileDescriptor(
      std::move(fd), std::move(data_pipe.producer_handle),
      // TODO(abarth): Move file tasks to a background thread.
      mtl::MessageLoop::GetCurrent()->task_runner(), Ignored);
  manager->StartApplicationUsingContentHandler(handler, std::move(response),
                                               std::move(request));
  return nullptr;
}

// TODO(abarth): We should use the fd we opened in LaunchWithContentHandler
// rather than using the path again.
mtl::UniqueHandle LaunchWithProcess(
    const std::string& path,
    mojo::InterfaceRequest<mojo::Application> request) {
  // We need room for:
  //
  //  * stdin/stdout/stderr
  //  * The mxio root (for framebuffer)
  //  * The shell handle
  //  * The process handle
  mx_handle_t child_handles[6 * MXIO_MAX_HANDLES];
  uint32_t ids[6 * MXIO_MAX_HANDLES];

  // TODO(abarth): Remove stdin, stdout, and stderr.
  size_t index = CloneStdStreams(child_handles, ids);

  // The framebuffer app is a special snowflake because it needs access to the
  // device tree to talk to the virtual console.
  // TODO(abarth): Find a more structured way to define which apps should have
  // elevated privileges.
  if (path == "/boot/apps/framebuffer") {
    mx_status_t status = mxio_clone_root(&child_handles[index], &ids[index]);
    if (status < 0) {
      fprintf(stderr, "Failed to clone mxio root: %d", status);
    } else {
      index += status;
    }
  }

  mojo::Handle initial_handle = request.PassMessagePipe().release();
  child_handles[index] = static_cast<mx_handle_t>(initial_handle.value());
  ids[index] = MX_HND_TYPE_APPLICATION_REQUEST;
  ++index;

  const char* path_arg = path.c_str();
  // TODO(abarth): Remove const_cast once MG-185 is fixed.
  char** argv = const_cast<char**>(&path_arg);
  return mtl::UniqueHandle(
      launchpad_launch_basic(path_arg, 1, argv, index, child_handles, ids));
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
