// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/user_creation_screen_handler.h"

#include "base/values.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/user_creation_screen.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/login/localized_values_builder.h"
#include "ui/chromeos/devicetype_utils.h"

namespace chromeos {

constexpr StaticOobeScreenId UserCreationView::kScreenId;

UserCreationScreenHandler::UserCreationScreenHandler(
    JSCallsContainer* js_calls_container)
    : BaseScreenHandler(kScreenId, js_calls_container) {
  set_user_acted_method_path("login.UserCreationScreen.userActed");
}

UserCreationScreenHandler::~UserCreationScreenHandler() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void UserCreationScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->AddF("userCreationTitle", IDS_OOBE_USER_CREATION_TITLE,
                ui::GetChromeOSDeviceName());
  builder->Add("userCreationSubtitle", IDS_OOBE_USER_CREATION_SUBTITLE);
  builder->AddF("userCreationAddPersonTitle",
                IDS_OOBE_USER_CREATION_ADD_PERSON_TITLE,
                ui::GetChromeOSDeviceName());
  builder->Add("userCreationAddPersonSubtitle",
               IDS_OOBE_USER_CREATION_ADD_PERSON_SUBTITLE);
  builder->Add("createForSelfLabel", IDS_OOBE_USER_CREATION_SELF_BUTTON_LABEL);
  builder->Add("createForSelfDescription",
               IDS_OOBE_USER_CREATION_SELF_BUTTON_DESCRIPTION);
  builder->Add("createForChildLabel",
               IDS_OOBE_USER_CREATION_CHILD_BUTTON_LABEL);
  builder->Add("createForChildDescription",
               IDS_OOBE_USER_CREATION_CHILD_BUTTON_DESCRIPTION);
  builder->AddF("childSignInTitle", IDS_OOBE_USER_CREATION_CHILD_SIGNIN_TITLE,
                ui::GetChromeOSDeviceName());
  builder->Add("childSignInSubtitle",
               IDS_OOBE_USER_CREATION_CHILD_SIGNIN_SUBTITLE);
  builder->Add("createAccountForChildLabel",
               IDS_OOBE_USER_CREATION_CHILD_ACCOUNT_CREATION_BUTTON_LABEL);
  builder->Add("signInForChildLabel",
               IDS_OOBE_USER_CREATION_CHILD_SIGN_IN_BUTTON_LABEL);
}

void UserCreationScreenHandler::Initialize() {}

void UserCreationScreenHandler::Show() {
  ShowScreen(kScreenId);
}

void UserCreationScreenHandler::Bind(UserCreationScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen_);
}

void UserCreationScreenHandler::Unbind() {
  screen_ = nullptr;
  BaseScreenHandler::SetBaseScreen(nullptr);
}

void UserCreationScreenHandler::SetIsBackButtonVisible(bool value) {
  CallJS("login.UserCreationScreen.setIsBackButtonVisible", value);
}

}  // namespace chromeos