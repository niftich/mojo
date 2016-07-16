// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/shell_impl.h"

#include <utility>

namespace mojo {

ShellImpl::ShellImpl(std::string application_name, ApplicationManager* manager)
    : binding_(this), connector_(std::move(application_name), manager) {}

ShellImpl::~ShellImpl() {}

void ShellImpl::Bind(InterfaceRequest<Shell> request) {
  binding_.Bind(std::move(request));
}

void ShellImpl::ConnectToApplication(
    const mojo::String& app_name,
    mojo::InterfaceRequest<mojo::ServiceProvider> services) {
  connector_.ConnectToApplication(app_name, std::move(services));
}

void ShellImpl::CreateApplicationConnector(
    mojo::InterfaceRequest<mojo::ApplicationConnector> request) {
  connector_.Duplicate(std::move(request));
}

}  // namespace mojo
