// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_instance.h"

#include "lib/ftl/logging.h"
#include "mojo/application_manager/application_launcher.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {

ApplicationInstance::ApplicationInstance() {}

ApplicationInstance::~ApplicationInstance() {}

bool ApplicationInstance::Start(const std::string& name) {
  FTL_DCHECK(!application_);
  FTL_DCHECK(!process_.is_valid());
  FTL_DCHECK(!shell_);
  process_ = mojo::LaunchApplication(name, mojo::GetProxy(&application_));
  return process_.is_valid();
}

void ApplicationInstance::Initialize(std::unique_ptr<ShellImpl> shell,
                                     mojo::Array<mojo::String> args,
                                     const mojo::String& name) {
  FTL_DCHECK(application_);
  FTL_DCHECK(process_.is_valid());
  FTL_DCHECK(!shell_);
  shell_ = std::move(shell);
  InterfaceHandle<Shell> shell_handle;
  shell_->Bind(GetProxy(&shell_handle));
  application_->Initialize(std::move(shell_handle), std::move(args), name);
}

}  // namespace mojo
