// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_instance.h"

#include "lib/ftl/logging.h"
#include "mojo/application_manager/application_launcher.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {
namespace {

// This function duplicates code in mojo/public/cpp/application/connect.h but
// we can't use that header here because the mojo/public/cpp/application target
// assumes that we want to implement mojo.Application.
// TODO(abarth): Find a way to share this code by moving connect.h to a less
// opinionated target.
template <typename Interface>
inline void ConnectToService(
    ServiceProvider* service_provider,
    InterfaceRequest<Interface> interface_request,
    const std::string& interface_name = Interface::Name_) {
  service_provider->ConnectToService(interface_name,
                                     interface_request.PassMessagePipe());
}

}  // namespace

ApplicationInstance::ApplicationInstance() {}

ApplicationInstance::~ApplicationInstance() {}

bool ApplicationInstance::Start(ApplicationManager* manager,
                                const std::string& name) {
  FTL_DCHECK(!application_);
  FTL_DCHECK(!process_.is_valid());
  FTL_DCHECK(!shell_);
  auto result = mojo::LaunchApplication(manager, name, GetProxy(&application_));
  process_ = std::move(result.second);
  return result.first;
}

void ApplicationInstance::Initialize(std::unique_ptr<ShellImpl> shell,
                                     mojo::Array<mojo::String> args,
                                     const mojo::String& name) {
  FTL_DCHECK(application_);
  FTL_DCHECK(!shell_);
  shell_ = std::move(shell);
  InterfaceHandle<Shell> shell_handle;
  shell_->Bind(GetProxy(&shell_handle));
  application_->Initialize(std::move(shell_handle), std::move(args), name);
}

mojo::ContentHandler* ApplicationInstance::GetOrCreateContentHandler() {
  FTL_DCHECK(application_);
  if (!content_handler_) {
    mojo::ServiceProviderPtr service_provider;
    // We use an empty requestor_url because this request is on behalf of the
    // application manager itself.
    std::string requestor_url;
    application_->AcceptConnection(requestor_url, name(),
                                   mojo::GetProxy(&service_provider));
    ConnectToService(service_provider.get(), mojo::GetProxy(&content_handler_));
  }
  return content_handler_.get();
}

}  // namespace mojo
