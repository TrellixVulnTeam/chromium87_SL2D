// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_PAYMENTS_PAYMENT_REQUEST_TEST_CONTROLLER_H_
#define CHROME_TEST_PAYMENTS_PAYMENT_REQUEST_TEST_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "build/build_config.h"

#if !defined(OS_ANDROID)
namespace sync_preferences {
class TestingPrefServiceSyncable;
}
#endif

namespace content {
class WebContents;
}

namespace payments {

class ContentPaymentRequestDelegate;

struct AppDescription {
  std::string label;
  std::string sublabel;
  std::string total;
};

// Observe states or actions taken by the PaymentRequest in tests, supporting
// both Android and desktop.
class PaymentRequestTestObserver {
 public:
  virtual void OnCanMakePaymentCalled() {}
  virtual void OnCanMakePaymentReturned() {}
  virtual void OnHasEnrolledInstrumentCalled() {}
  virtual void OnHasEnrolledInstrumentReturned() {}
  virtual void OnAppListReady() {}
  virtual void OnNotSupportedError() {}
  virtual void OnConnectionTerminated() {}
  virtual void OnAbortCalled() {}
  virtual void OnCompleteCalled() {}
  virtual void OnMinimalUIReady() {}

 protected:
  virtual ~PaymentRequestTestObserver() = default;
};

// A class to control creation and behaviour of PaymentRequests in a
// cross-platform way for testing both Android and desktop.
class PaymentRequestTestController {
 public:
  PaymentRequestTestController();
  ~PaymentRequestTestController();

  // To be called from an override of BrowserTestBase::SetUpOnMainThread().
  void SetUpOnMainThread();

  void SetObserver(PaymentRequestTestObserver* observer);

  // Sets values that will change the behaviour of PaymentRequests created in
  // the future.
  void SetOffTheRecord(bool is_off_the_record);
  void SetValidSsl(bool valid_ssl);
  void SetCanMakePaymentEnabledPref(bool can_make_payment_enabled);
  void SetTwaPackageName(const std::string& twa_package_name);
  void SetHasAuthenticator(bool has_authenticator);
  void SetTwaPaymentApp(const std::string& method_name,
                        const std::string& response);

  // Gets the WebContents of the Payment Handler for testing purpose, or null if
  // nonexistent. To guarantee a non-null return, this function should be called
  // only if: 1) PaymentRequest UI is opening. 2) ScrollToExpandPaymentHandler
  // feature is enabled (on Android). 3) PaymentHandler is opening.
  content::WebContents* GetPaymentHandlerWebContents();

#if defined(OS_ANDROID)
  // Clicks the security icon on the Expandable Payment Handler toolbar for
  // testing purpose. Return whether it's succeeded.
  bool ClickPaymentHandlerSecurityIcon();
#endif

  // Clicks the close button on the Payment Handler toolbar for testing purpose.
  // Return whether it's succeeded.
  bool ClickPaymentHandlerCloseButton();

  // Confirms payment in a browser payment sheet, be it either PAYMENT_REQUEST
  // or SECURE_PAYMENT_CONFIRMATION type. Returns true if the dialog was
  // available.
  bool ConfirmPayment();

  // Confirms payment in minimal UI. Returns true on success or if the minimal
  // UI is not implemented on the current platform.
  bool ConfirmMinimalUI();

  // Dismisses payment in minimal UI. Returns true on success or if the minimal
  // UI is not implemented on the current platform.
  bool DismissMinimalUI();

  // Returns true when running on Android M or L.
  bool IsAndroidMarshmallowOrLollipop();

  // Sets the list of apps available for the current payment request.
  void set_app_descriptions(
      const std::vector<AppDescription>& app_descriptions) {
    app_descriptions_ = app_descriptions;
  }

  // Returns the list of apps available for the current payment request.
  const std::vector<AppDescription>& app_descriptions() const {
    return app_descriptions_;
  }

 private:
  // Observers that forward through to the PaymentRequestTestObserver.
  void OnCanMakePaymentCalled();
  void OnCanMakePaymentReturned();
  void OnHasEnrolledInstrumentCalled();
  void OnHasEnrolledInstrumentReturned();
  void OnAppListReady();
  void OnNotSupportedError();
  void OnConnectionTerminated();
  void OnAbortCalled();
  void OnCompleteCalled();
  void OnMinimalUIReady();

  PaymentRequestTestObserver* observer_ = nullptr;

  bool is_off_the_record_ = false;
  bool valid_ssl_ = true;
  bool can_make_payment_pref_ = true;
  std::string twa_package_name_;
  bool has_authenticator_ = false;
  std::string twa_payment_app_method_name_;
  std::string twa_payment_app_response_;
  std::vector<AppDescription> app_descriptions_;

#if !defined(OS_ANDROID)
  void UpdateDelegateFactory();

  std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> prefs_;

  class ObserverConverter;
  std::unique_ptr<ObserverConverter> observer_converter_;

  base::WeakPtr<ContentPaymentRequestDelegate> delegate_;
#endif
};

}  // namespace payments

#endif  // CHROME_TEST_PAYMENTS_PAYMENT_REQUEST_TEST_CONTROLLER_H_