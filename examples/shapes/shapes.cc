// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <string>
#include <utility>

#include <magenta/syscalls.h>

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/framebuffer/interfaces/framebuffer.mojom.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace {

class ShapesApp : public mojo::ApplicationImplBase {
 public:
  ShapesApp() {}
  ~ShapesApp() override {}

  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:framebuffer",
                           mojo::GetProxy(&provider_));
    CreateFramebuffer();
  }

  void CreateFramebuffer() { provider_->Create(CallCreateSurface(this)); }

  void CreateSurface(mojo::InterfaceHandle<mojo::Framebuffer> frame_buffer,
                     mojo::FramebufferInfoPtr info) {
    if (!frame_buffer) {
      fprintf(stderr, "Failed to create frame buffer\n");
      return;
    }
    frame_buffer_.Bind(std::move(frame_buffer));
    info_ = std::move(info);

    uintptr_t buffer = 0;
    size_t row_bytes = info_->row_bytes;
    size_t size = row_bytes * info_->size->height;

    mx_status_t status =
        mx_process_vm_map(0, info_->vmo.get().value(), 0, size, &buffer,
                          MX_VM_FLAG_PERM_READ | MX_VM_FLAG_PERM_WRITE);

    if (status < 0) {
      fprintf(stderr, "Cannot map frame buffer %d\n", status);
      mojo::RunLoop::current()->Quit();
      return;
    }

    // TODO(abarth): Enablet this check once info_->format is correct.
    // if (info_->format != mojo::FramebufferFormat::RGB_565) {
    //   fprintf(stderr, "Frame buffer format not supported %d\n",
    //   info_->format);
    //   mojo::RunLoop::current()->Quit();
    //   return;
    // }

    SkImageInfo image_info =
        SkImageInfo::Make(info_->size->width, info_->size->height,
                          kRGB_565_SkColorType, kPremul_SkAlphaType);

    surface_ = SkSurface::MakeRasterDirect(
        image_info, reinterpret_cast<void*>(buffer), row_bytes);

    mojo::RunLoop::current()->PostDelayedTask([this]() { Draw(); }, 0);
  }

  void Draw() {
    x_ += 10.0f;
    if (x_ > info_->size->width)
      x_ = 0.0;

    SkCanvas* canvas = surface_->getCanvas();
    canvas->clear(SK_ColorBLUE);
    SkPaint paint;
    paint.setColor(0xFFFF00FF);
    paint.setAntiAlias(true);
    canvas->drawCircle(x_, y_, 200.0f, paint);
    canvas->flush();

    frame_buffer_->Flush([this]() { Draw(); });
  }

 private:
  class CallCreateSurface {
   public:
    explicit CallCreateSurface(ShapesApp* app) : app_(app) {}

    void Run(mojo::InterfaceHandle<mojo::Framebuffer> frame_buffer,
             mojo::FramebufferInfoPtr info) const {
      app_->CreateSurface(std::move(frame_buffer), std::move(info));
    }

   private:
    ShapesApp* app_;
  };

  mojo::FramebufferProviderPtr provider_;
  mojo::FramebufferPtr frame_buffer_;
  mojo::FramebufferInfoPtr info_;
  sk_sp<SkSurface> surface_;
  float x_ = 400.0f;
  float y_ = 300.0f;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ShapesApp);
};

}  // namespace

MojoResult MojoMain(MojoHandle request) {
  ShapesApp app;
  return mojo::RunApplication(request, &app);
}
