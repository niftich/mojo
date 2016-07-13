// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_APPLICATION_MANAGER_APPLICATION_INSTANCE_H_
#define MOJO_APPLICATION_MANAGER_APPLICATION_INSTANCE_H_

#include <memory>
#include <string>
#include <vector>

#include "mtl/unique_handle.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/application_manager/shell_impl.h"

namespace mojo {

class ApplicationInstance {
 public:
  ApplicationInstance();
  ~ApplicationInstance();

  // Starts the application with the given name. Returns whether the process
  // was created successfully. Does not send the initialization message.
  //
  // To stop the process, destruct this object.
  bool Start(const std::string& name);

  // Sends the initialize message to the application.
  //
  // Creates a message pipe for the shell and binds the given shell to that
  // message pipe.
  //
  // The given shell must not be bound. The application must have already been
  // started but must not have been initialized yet.
  void Initialize(std::unique_ptr<ShellImpl> shell,
                  mojo::Array<mojo::String> args,
                  const mojo::String& name);

  mx_handle_t process() const { return process_.get(); }
  mojo::Application* application() const { return application_.get(); }

  bool is_initialized() const { return !!shell_; }
  const std::string& name() const { return shell_->name(); }

  void set_connection_error_handler(const Closure& error_handler) {
    application_.set_connection_error_handler(error_handler);
  }

 private:
  mtl::UniqueHandle process_;
  mojo::ApplicationPtr application_;
  std::unique_ptr<ShellImpl> shell_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ApplicationInstance);
};

}  // namespace mojo

#endif  // MOJO_APPLICATION_MANAGER_APPLICATION_INSTANCE_H_
