// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_APPLICATION_MANAGER_APPLICATION_CONNECTOR_IMPL_H_
#define MOJO_APPLICATION_MANAGER_APPLICATION_CONNECTOR_IMPL_H_

#include "ftl/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/interfaces/application/application_connector.mojom.h"

namespace mojo {
class ApplicationManager;

class ApplicationConnectorImpl : public ApplicationConnector {
 public:
  // The ApplicationManager must remain alive for the lifetime of this object.
  ApplicationConnectorImpl(std::string application_name,
                           ApplicationManager* manager);
  ~ApplicationConnectorImpl();

  const std::string& name() const { return name_; }

  void ConnectToApplication(
      const String& app_name,
      InterfaceRequest<ServiceProvider> services) override;

  void Duplicate(InterfaceRequest<ApplicationConnector> request) override;

 private:
  BindingSet<ApplicationConnector> bindings_;
  const std::string name_;
  ApplicationManager* const manager_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ApplicationConnectorImpl);
};

}  // namespace mojo

#endif  // MOJO_APPLICATION_MANAGER_APPLICATION_CONNECTOR_IMPL_H_
