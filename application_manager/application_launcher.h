// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_APPLICATION_MANAGER_APPLICATION_LAUNCHER_H_
#define MOJO_APPLICATION_MANAGER_APPLICATION_LAUNCHER_H_

#include <magenta/types.h>
#include <string>

#include "mtl/unique_handle.h"
#include "mojo/public/interfaces/application/application.mojom.h"

namespace mojo {

// Starts the application with the given name.
//
// If the name stats with "mojo:", this function will look for the application
// in /boot/apps. Otherwise, no application is launched.
mtl::UniqueHandle LaunchApplication(
    const std::string& name,
    mojo::InterfaceRequest<mojo::Application> application_request);

}  // namespace mojo

#endif  // MOJO_APPLICATION_MANAGER_APPLICATION_LAUNCHER_H_
