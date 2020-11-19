// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_TEST_KIOSK_TEST_HELPERS_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_TEST_KIOSK_TEST_HELPERS_H_

#include "base/run_loop.h"
#include "base/scoped_observer.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager_base.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager_observer.h"
#include "chrome/browser/chromeos/login/app_mode/kiosk_launch_controller.h"
#include "chrome/browser/chromeos/settings/scoped_cros_settings_test_helper.h"

namespace chromeos {

class FakeOwnerSettingsService;

// Common classes that can be used for kiosk mode testing.
// Waits for kiosk session to be initialized.
class KioskSessionInitializedWaiter : public KioskAppManagerObserver {
 public:
  KioskSessionInitializedWaiter();
  ~KioskSessionInitializedWaiter() override;
  KioskSessionInitializedWaiter(const KioskSessionInitializedWaiter&) = delete;
  KioskSessionInitializedWaiter& operator=(
      const KioskSessionInitializedWaiter&) = delete;

  void Wait();

 private:
  // KioskAppManagerObserver:
  void OnKioskSessionInitialized() override;

  ScopedObserver<KioskAppManagerBase, KioskAppManagerObserver> scoped_observer_{
      this};
  base::RunLoop run_loop_;
};

// Used to replace OwnerSettingsService.
class ScopedDeviceSettings {
 public:
  ScopedDeviceSettings();
  ~ScopedDeviceSettings();

  FakeOwnerSettingsService* owner_settings_service() {
    return owner_settings_service_.get();
  }

 private:
  ScopedCrosSettingsTestHelper settings_helper_;
  std::unique_ptr<FakeOwnerSettingsService> owner_settings_service_;
};

class ScopedCanConfigureNetwork {
 public:
  ScopedCanConfigureNetwork(bool can_configure, bool needs_owner_auth);
  ScopedCanConfigureNetwork(const ScopedCanConfigureNetwork&) = delete;
  ScopedCanConfigureNetwork& operator=(const ScopedCanConfigureNetwork&) =
      delete;
  ~ScopedCanConfigureNetwork();

  bool CanConfigureNetwork() const { return can_configure_; }

  bool NeedsOwnerAuthToConfigureNetwork() const { return needs_owner_auth_; }

 private:
  const bool can_configure_;
  const bool needs_owner_auth_;
  KioskLaunchController::ReturnBoolCallback can_configure_network_callback_;
  KioskLaunchController::ReturnBoolCallback needs_owner_auth_callback_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_TEST_KIOSK_TEST_HELPERS_H_