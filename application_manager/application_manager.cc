// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_manager.h"

#include <stdio.h>
#include <stdlib.h>

#include "ftl/logging.h"
#include "ftl/memory/make_unique.h"
#include "mojo/application_manager/application_instance.h"
#include "mojo/application_manager/shell_impl.h"

namespace mojo {

ApplicationManager::ApplicationManager() {}

ApplicationManager::~ApplicationManager() {}

bool ApplicationManager::StartInitialApplication(std::string name) {
  FTL_DCHECK(table_.is_empty());
  return GetOrStartApplication(std::move(name));
}

void ApplicationManager::ConnectToApplication(
    const std::string& application_name,
    const std::string& requestor_name,
    InterfaceRequest<ServiceProvider> services) {
  Application* application = GetOrStartApplication(std::move(application_name));
  if (!application)
    return;
  application->AcceptConnection(requestor_name, requestor_name,
                                std::move(services));
}

Application* ApplicationManager::GetOrStartApplication(std::string name) {
  ApplicationInstance* instance = table_.GetOrStartApplication(name);
  if (!instance) {
    fprintf(stderr, "error: Failed to start application %s", name.c_str());
    return nullptr;
  }
  if (!instance->is_initialized()) {
    instance->Initialize(ftl::MakeUnique<ShellImpl>(name, this), nullptr, name);
    instance->set_connection_error_handler([this, instance]() {
      table_.StopApplication(instance->name());
    });
  }
  return instance->application();
}

}  // namespace mojo
