// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_APPLICATION_MANAGER_APPLICATION_MANAGER_H_
#define MOJO_APPLICATION_MANAGER_APPLICATION_MANAGER_H_

#include <string>

#include "mojo/application_manager/application_table.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/network/url_response.mojom.h"

namespace mojo {

class ApplicationManager {
 public:
  ApplicationManager();
  ~ApplicationManager();

  bool StartInitialApplication(std::string name);

  void ConnectToApplication(const std::string& application_name,
                            const std::string& requestor_name,
                            InterfaceRequest<ServiceProvider> services);

  void StartApplicationUsingContentHandler(
      const std::string& content_handler_name,
      URLResponsePtr response,
      InterfaceRequest<Application> application_request);

 private:
  ApplicationInstance* GetOrStartApplicationInstance(std::string name);

  ApplicationTable table_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ApplicationManager);
};

}  // namespace mojo

#endif  // MOJO_APPLICATION_MANAGER_APPLICATION_MANAGER_H_
