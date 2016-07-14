// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_APPLICATION_MANAGER_APPLICATION_TABLE_H_
#define MOJO_APPLICATION_MANAGER_APPLICATION_TABLE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "mojo/application_manager/application_instance.h"

namespace mojo {
class ApplicationManager;

class ApplicationTable {
 public:
  ApplicationTable();
  ~ApplicationTable();

  ApplicationInstance* GetOrStartApplication(ApplicationManager* manager,
                                             std::string name);
  void StopApplication(const std::string& name);

  bool is_empty() const { return map_.empty(); }

 private:
  using AppMap = std::unordered_map<std::string,
                                    std::unique_ptr<ApplicationInstance>>;

  AppMap map_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ApplicationTable);
};

}  // namespace mojo

#endif  // MOJO_APPLICATION_MANAGER_APPLICATION_TABLE_H_
