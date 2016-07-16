// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_APPLICATION_MANAGER_APPLICATION_LAUNCHER_H_
#define MOJO_APPLICATION_MANAGER_APPLICATION_LAUNCHER_H_

#include <magenta/types.h>
#include <string>
#include <utility>

#include "lib/ftl/macros.h"
#include "lib/mtl/unique_handle.h"
#include "mojo/public/interfaces/application/application.mojom.h"

namespace mojo {
class ApplicationManager;

// Starts the application with the given name.
//
// If the name stats with "mojo:", this function will look for the application
// in /boot/apps. Otherwise, no application is launched.
//
// If the name resolves to a native executable, this function will create a
// process and load the executable. If the name resolves to a file with mojo
// magic (i.e., #!mojo), this function will ask the appropriate content handler
// to start the application.
//
// The first field of the return value indicates whether this function was able
// deliver the application_request to something that might be able to provide
// an application.
//
// The second field of the return value is a handle to the process created by
// this function, if the name resolved to a native executable.
std::pair<bool, mtl::UniqueHandle> LaunchApplication(
    ApplicationManager* manager,
    const std::string& name,
    mojo::InterfaceRequest<mojo::Application> application_request);

}  // namespace mojo

#endif  // MOJO_APPLICATION_MANAGER_APPLICATION_LAUNCHER_H_
