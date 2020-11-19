// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/fingerprint_setup_screen.h"

#include "base/metrics/histogram_functions.h"
#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_utils.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager_util.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/webui/chromeos/login/fingerprint_setup_screen_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/device_service.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {
namespace {

constexpr char kUserActionSetupDone[] = "setup-done";
constexpr char kUserActionSetupSkipped[] = "setup-skipped";
constexpr char kUserActionDoItLater[] = "do-it-later";
constexpr char kUserActionAddAnotherFinger[] = "add-another-finger";
constexpr char kUserActionShowSensorLocation[] = "show-sensor-location";

struct FingerprintSetupUserAction {
  const char* name_;
  FingerprintSetupScreen::UserAction uma_name_;
};

const FingerprintSetupUserAction actions[] = {
    {kUserActionSetupDone, FingerprintSetupScreen::UserAction::kSetupDone},
    {kUserActionSetupSkipped,
     FingerprintSetupScreen::UserAction::kSetupSkipped},
    {kUserActionDoItLater, FingerprintSetupScreen::UserAction::kDoItLater},
    {kUserActionAddAnotherFinger,
     FingerprintSetupScreen::UserAction::kAddAnotherFinger},
    {kUserActionShowSensorLocation,
     FingerprintSetupScreen::UserAction::kShowSensorLocation},
};

void RecordFingerprintSetupUserAction(
    FingerprintSetupScreen::UserAction value) {
  base::UmaHistogramEnumeration("OOBE.FingerprintSetupScreen.UserActions",
                                value);
}

bool IsFingerprintUserAction(const std::string& action_id) {
  for (const auto& el : actions) {
    if (action_id == el.name_)
      return true;
  }
  return false;
}

void RecordUserAction(const std::string& action_id) {
  for (const auto& el : actions) {
    if (action_id == el.name_) {
      RecordFingerprintSetupUserAction(el.uma_name_);
      return;
    }
  }
  NOTREACHED() << "Unexpected action id: " << action_id;
}

// The max number of fingerprints that can be stored.
constexpr int kMaxAllowedFingerprints = 3;

// Determines what the newly added fingerprint's name should be.
std::string GetDefaultFingerprintName(int enrolled_finger_count) {
  DCHECK(enrolled_finger_count < kMaxAllowedFingerprints);
  switch (enrolled_finger_count) {
    case 0:
      return l10n_util::GetStringUTF8(
          IDS_OOBE_FINGERPINT_SETUP_SCREEN_NEW_FINGERPRINT_DEFAULT_NAME_1);
    case 1:
      return l10n_util::GetStringUTF8(
          IDS_OOBE_FINGERPINT_SETUP_SCREEN_NEW_FINGERPRINT_DEFAULT_NAME_2);
    case 2:
      return l10n_util::GetStringUTF8(
          IDS_OOBE_FINGERPINT_SETUP_SCREEN_NEW_FINGERPRINT_DEFAULT_NAME_3);
    default:
      NOTREACHED();
  }
  return std::string();
}

}  // namespace

// static
std::string FingerprintSetupScreen::GetResultString(Result result) {
  switch (result) {
    case Result::DONE:
      return "Done";
    case Result::SKIPPED:
      return "Skipped";
    case Result::DO_IT_LATER:
      return "DoItLater";
    case Result::NOT_APPLICABLE:
      return BaseScreen::kNotApplicable;
  }
}

FingerprintSetupScreen* FingerprintSetupScreen::Get(ScreenManager* manager) {
  return static_cast<FingerprintSetupScreen*>(
      manager->GetScreen(FingerprintSetupScreenView::kScreenId));
}

FingerprintSetupScreen::FingerprintSetupScreen(
    FingerprintSetupScreenView* view,
    const ScreenExitCallback& exit_callback)
    : BaseScreen(FingerprintSetupScreenView::kScreenId,
                 OobeScreenPriority::DEFAULT),
      view_(view),
      exit_callback_(exit_callback) {
  content::GetDeviceService().BindFingerprint(
      fp_service_.BindNewPipeAndPassReceiver());
  fp_service_->AddFingerprintObserver(receiver_.BindNewPipeAndPassRemote());
  DCHECK(view_);
  view_->Bind(this);
}

FingerprintSetupScreen::~FingerprintSetupScreen() {
  view_->Bind(nullptr);
}

bool FingerprintSetupScreen::MaybeSkip(WizardContext* context) {
  if (!chromeos::quick_unlock::IsFingerprintEnabled(
          ProfileManager::GetActiveUserProfile()) ||
      chrome_user_manager_util::IsPublicSessionOrEphemeralLogin()) {
    exit_callback_.Run(Result::NOT_APPLICABLE);
    return true;
  }
  return false;
}

void FingerprintSetupScreen::ShowImpl() {
  StartAddingFinger();
  view_->Show();
}

void FingerprintSetupScreen::HideImpl() {
  // Clean up existing fingerprint enroll session.
  if (enroll_session_started_) {
    fp_service_->CancelCurrentEnrollSession(
        base::BindOnce(&FingerprintSetupScreen::OnCancelCurrentEnrollSession,
                       weak_ptr_factory_.GetWeakPtr()));
  }
  view_->Hide();
}

void FingerprintSetupScreen::OnUserAction(const std::string& action_id) {
  if (!IsFingerprintUserAction(action_id)) {
    BaseScreen::OnUserAction(action_id);
    return;
  }
  RecordUserAction(action_id);
  if (action_id == kUserActionSetupDone) {
    exit_callback_.Run(Result::DONE);
  } else if (action_id == kUserActionSetupSkipped) {
    exit_callback_.Run(Result::SKIPPED);
  } else if (action_id == kUserActionDoItLater) {
    exit_callback_.Run(Result::DO_IT_LATER);
  } else if (action_id == kUserActionAddAnotherFinger) {
    StartAddingFinger();
  }
}

void FingerprintSetupScreen::OnRestarted() {
  VLOG(1) << "Fingerprint session restarted.";
}

void FingerprintSetupScreen::OnEnrollScanDone(
    device::mojom::ScanResult scan_result,
    bool enroll_session_complete,
    int percent_complete) {
  VLOG(1) << "Receive fingerprint enroll scan result. scan_result="
          << scan_result
          << ", enroll_session_complete=" << enroll_session_complete
          << ", percent_complete=" << percent_complete;
  view_->OnEnrollScanDone(scan_result, enroll_session_complete,
                          percent_complete);

  if (!enroll_session_complete)
    return;

  enroll_session_started_ = false;
  ++enrolled_finger_count_;
  view_->EnableAddAnotherFinger(
      /* enable = */ enrolled_finger_count_ < kMaxAllowedFingerprints);

  // Update the number of registered fingers, it's fine to override because
  // this is the first time user log in and have no finger registered.
  ProfileManager::GetActiveUserProfile()->GetPrefs()->SetInteger(
      prefs::kQuickUnlockFingerprintRecord, enrolled_finger_count_);
}

void FingerprintSetupScreen::OnAuthScanDone(
    device::mojom::ScanResult scan_result,
    const base::flat_map<std::string, std::vector<std::string>>& matches) {}

void FingerprintSetupScreen::OnSessionFailed() {
  // TODO(xiaoyinh): Add more user visible information when available.
  LOG(ERROR) << "Fingerprint session failed.";
}

void FingerprintSetupScreen::StartAddingFinger() {
  DCHECK(enrolled_finger_count_ < kMaxAllowedFingerprints);

  enroll_session_started_ = true;
  fp_service_->StartEnrollSession(
      ProfileHelper::Get()->GetUserIdHashFromProfile(
          ProfileManager::GetActiveUserProfile()),
      GetDefaultFingerprintName(enrolled_finger_count_));
}

void FingerprintSetupScreen::OnCancelCurrentEnrollSession(bool success) {
  if (!success)
    LOG(ERROR) << "Failed to cancel current fingerprint enroll session.";
}

}  // namespace chromeos