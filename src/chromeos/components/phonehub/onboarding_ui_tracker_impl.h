// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PHONEHUB_ONBOARDING_UI_TRACKER_IMPL_H_
#define CHROMEOS_COMPONENTS_PHONEHUB_ONBOARDING_UI_TRACKER_IMPL_H_

#include "base/callback.h"
#include "chromeos/components/phonehub/feature_status_provider.h"
#include "chromeos/components/phonehub/onboarding_ui_tracker.h"
#include "chromeos/services/multidevice_setup/public/cpp/multidevice_setup_client.h"

class PrefRegistrySimple;
class PrefService;

namespace chromeos {
namespace phonehub {

// OnboardingUiTracker implementation that uses the
// |kHasDismissedUiAfterCompletingOnboarding| pref to determine whether the
// Onboarding UI should be shown. This class invokes
// |show_multidevice_setup_dialog_callback| when the user proceeds with the
// onboarding flow if Better Together is disabled. If Better Together is
// enabled, but PhoneHub is disabled, this class enables the PhoneHub feature
// via the MultiDeviceSetupClient instead.
class OnboardingUiTrackerImpl : public OnboardingUiTracker,
                                public FeatureStatusProvider::Observer {
 public:
  static void RegisterPrefs(PrefRegistrySimple* registry);

  OnboardingUiTrackerImpl(
      PrefService* pref_service,
      FeatureStatusProvider* feature_status_provider,
      multidevice_setup::MultiDeviceSetupClient* multidevice_setup_client,
      const base::RepeatingClosure& show_multidevice_setup_dialog_callback);
  ~OnboardingUiTrackerImpl() override;

  // OnboardingUiTracker:
  bool ShouldShowOnboardingUi() const override;
  void DismissSetupUi() override;
  void HandleGetStarted() override;

 private:
  // FeatureStatusProvider::Observer:
  void OnFeatureStatusChanged() override;

  bool ComputeShouldShowOnboardingUi();
  void UpdateShouldShowOnboardingUi();
  PrefService* pref_service_;
  FeatureStatusProvider* feature_status_provider_;
  multidevice_setup::MultiDeviceSetupClient* multidevice_setup_client_;
  bool should_show_onboarding_ui_;
  base::RepeatingClosure show_multidevice_setup_dialog_callback_;
};

}  // namespace phonehub
}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_PHONEHUB_ONBOARDING_UI_TRACKER_IMPL_H_