// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/macros.h"
#include "third_party/skia/include/core/SkBitmapDevice.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace {

class ShapesApp : public mojo::ApplicationImplBase {
 public:
  ShapesApp() {}
  ~ShapesApp() override {}

  void OnInitialize() override {
    printf("Shapes OnInitialize.\n");
    SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600);
    sk_sp<SkBitmapDevice> device(SkBitmapDevice::Create(info));
    sk_sp<SkCanvas> canvas = sk_make_sp<SkCanvas>(device.get());
    canvas->clear(SK_ColorBLUE);
    SkPaint paint;
    paint.setColor(SK_ColorGREEN);
    paint.setAntiAlias(true);
    canvas->drawCircle(400.0f, 300.0f, 200.0f, paint);
    // TODO(abarth): Actually show these shapes once we have the screen wired
    // up in some fashion.
  }

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(ShapesApp);
};

}  // namespace

MojoResult MojoMain(MojoHandle request) {
  ShapesApp app;
  return mojo::RunApplication(request, &app);
}
