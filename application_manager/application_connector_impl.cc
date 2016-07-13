// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_connector_impl.h"

#include <utility>

#include "mojo/application_manager/application_manager.h"

namespace mojo {

ApplicationConnectorImpl::ApplicationConnectorImpl(std::string application_name,
                                                   ApplicationManager* manager)
  : name_(std::move(application_name)), manager_(manager) {}

ApplicationConnectorImpl::~ApplicationConnectorImpl() {}

void ApplicationConnectorImpl::ConnectToApplication(
      const String& app_name,
      InterfaceRequest<ServiceProvider> services) {
  manager_->ConnectToApplication(app_name, name_, std::move(services));
}

void ApplicationConnectorImpl::Duplicate(
    InterfaceRequest<ApplicationConnector> application_connector_request) {
  bindings_.AddBinding(this, std::move(application_connector_request));
}

}  // namespace mojo
