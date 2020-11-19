// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/testing_browser_process_platform_part.h"

TestingBrowserProcessPlatformPart::TestingBrowserProcessPlatformPart() {
}

TestingBrowserProcessPlatformPart::~TestingBrowserProcessPlatformPart() {
}
#if defined(OS_MAC)
void TestingBrowserProcessPlatformPart::SetLocationPermissionManager(
    std::unique_ptr<GeolocationSystemPermissionManager>
        location_permission_manager) {
  location_permission_manager_ = std::move(location_permission_manager);
}
#endif