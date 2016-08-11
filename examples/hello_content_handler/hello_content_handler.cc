// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/system/main.h>
#include <stdio.h>

#include <utility>

#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/wait.h"
#include "mojo/services/content_handler/interfaces/content_handler.mojom.h"

namespace {

class HelloApplication : public mojo::Application {
 public:
  HelloApplication(mojo::InterfaceRequest<mojo::Application> request)
      : binding_(this, request.Pass()) {}

  void Initialize(mojo::InterfaceHandle<mojo::Shell> shell,
                  mojo::Array<mojo::String> args,
                  const mojo::String& url) override {
    printf("Initializing HelloApplication url=%s\n", url.get().c_str());
    shell_ = mojo::ShellPtr::Create(std::move(shell));
  }

  void RequestQuit() override {}

  void AcceptConnection(
      const mojo::String& requestor_url,
      const mojo::String& url,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override {
    printf("Connected to HelloApplication requestor_url=%s url=%s\n",
           requestor_url.get().c_str(), url.get().c_str());
  }

  mojo::StrongBinding<mojo::Application> binding_;
  mojo::ShellPtr shell_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(HelloApplication);
};

class ContentHandlerImpl : public mojo::ContentHandler {
 public:
  explicit ContentHandlerImpl(
      mojo::InterfaceRequest<mojo::ContentHandler> request)
      : binding_(this, request.Pass()) {}
  ~ContentHandlerImpl() override {}

 private:
  void StartApplication(mojo::InterfaceRequest<mojo::Application> application,
                        mojo::URLResponsePtr response) override {
    new HelloApplication(application.Pass());
  }

  mojo::StrongBinding<mojo::ContentHandler> binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ContentHandlerImpl);
};

class ContentHandlerApp : public mojo::ApplicationImplBase {
 public:
  ContentHandlerApp() {}
  ~ContentHandlerApp() override {}

  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<mojo::ContentHandler>(
        [](const mojo::ConnectionContext& connection_context,
           mojo::InterfaceRequest<mojo::ContentHandler> request) {
          new ContentHandlerImpl(request.Pass());
        });
    return true;
  }

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(ContentHandlerApp);
};

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  ContentHandlerApp content_handler_app;
  return mojo::RunApplication(application_request, &content_handler_app);
}
