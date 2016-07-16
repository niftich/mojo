// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/application_table.h"

#include <utility>

#include "lib/ftl/memory/make_unique.h"

namespace mojo {

ApplicationTable::ApplicationTable() {}

ApplicationTable::~ApplicationTable() {}

ApplicationInstance* ApplicationTable::GetOrStartApplication(
    ApplicationManager* manager,
    std::string name) {
  auto result = map_.emplace(std::move(name), nullptr);
  auto it = result.first;
  if (result.second) {
    auto application = ftl::MakeUnique<ApplicationInstance>();
    if (!application->Start(manager, it->first)) {
      map_.erase(it);
      return nullptr;
    }
    it->second = std::move(application);
  }
  return it->second.get();
}

void ApplicationTable::StopApplication(const std::string& name) {
  map_.erase(name);
}

}  // namespace mojo
