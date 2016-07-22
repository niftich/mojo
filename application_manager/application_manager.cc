// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_manager.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib/ftl/logging.h"
#include "mojo/application_manager/application_instance.h"
#include "mojo/application_manager/shell_impl.h"

namespace mojo {

ApplicationManager::ApplicationManager() {}

ApplicationManager::~ApplicationManager() {}

bool ApplicationManager::StartInitialApplication(std::string name) {
  FTL_DCHECK(table_.is_empty());
  return GetOrStartApplicationInstance(std::move(name));
}

void ApplicationManager::ConnectToApplication(
    const std::string& application_name,
    const std::string& requestor_name,
    InterfaceRequest<ServiceProvider> services) {
  ApplicationInstance* instance =
      GetOrStartApplicationInstance(std::move(application_name));
  if (!instance)
    return;
  instance->application()->AcceptConnection(requestor_name, requestor_name,
                                            std::move(services));
}

void ApplicationManager::StartApplicationUsingContentHandler(
    const std::string& content_handler_name,
    URLResponsePtr response,
    InterfaceRequest<Application> application_request) {
  ApplicationInstance* instance =
      GetOrStartApplicationInstance(content_handler_name);
  if (!instance)
    return;
  ContentHandler* content_handler = instance->GetOrCreateContentHandler();
  content_handler->StartApplication(std::move(application_request),
                                    std::move(response));
}

ApplicationInstance* ApplicationManager::GetOrStartApplicationInstance(
    std::string name) {
  ApplicationInstance* instance = table_.GetOrStartApplication(this, name);
  if (!instance) {
    fprintf(stderr, "error: Failed to start application %s", name.c_str());
    return nullptr;
  }
  if (!instance->is_initialized()) {
    instance->Initialize(std::make_unique<ShellImpl>(name, this), nullptr,
                         name);
    instance->set_connection_error_handler(
        [this, instance]() { table_.StopApplication(instance->name()); });
  }
  return instance;
}

}  // namespace mojo
