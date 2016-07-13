// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_APPLICATION_MANAGER_SHELL_IMPL_H_
#define MOJO_APPLICATION_MANAGER_SHELL_IMPL_H_

#include "mojo/application_manager/application_connector_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {
class ApplicationManager;

class ShellImpl : public Shell {
 public:
  ShellImpl(std::string application_name, ApplicationManager* manager);
  ~ShellImpl();

  const std::string& name() const { return connector_.name(); }

  void Bind(InterfaceRequest<Shell> request);

  void ConnectToApplication(
      const mojo::String& app_name,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override;

  void CreateApplicationConnector(
      mojo::InterfaceRequest<mojo::ApplicationConnector> request) override;

 private:
  Binding<Shell> binding_;
  ApplicationConnectorImpl connector_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ShellImpl);
};

}  // namespace mojo

#endif  // MOJO_APPLICATION_MANAGER_SHELL_IMPL_H_
