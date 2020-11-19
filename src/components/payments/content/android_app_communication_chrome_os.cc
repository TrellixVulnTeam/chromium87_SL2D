// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/android_app_communication.h"

#include <utility>

#include "components/arc/mojom/payment_app.mojom.h"
#include "components/arc/pay/arc_payment_app_bridge.h"
#include "components/payments/core/android_app_description.h"
#include "components/payments/core/chrome_os_error_strings.h"
#include "components/payments/core/method_strings.h"
#include "components/payments/core/native_error_strings.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace payments {
namespace {

static constexpr char kEmptyDictionaryJson[] = "{}";

void OnIsImplemented(
    const std::string& twa_package_name,
    AndroidAppCommunication::GetAppDescriptionsCallback callback,
    arc::mojom::IsPaymentImplementedResultPtr response) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!twa_package_name.empty());

  if (response.is_null()) {
    std::move(callback).Run(errors::kEmptyResponse, /*app_descriptions=*/{});
    return;
  }

  if (response->is_error()) {
    std::move(callback).Run(response->get_error(), /*app_descriptions=*/{});
    return;
  }

  if (!response->is_valid()) {
    std::move(callback).Run(errors::kInvalidResponse, /*app_descriptions=*/{});
    return;
  }

  if (response->get_valid()->activity_names.empty()) {
    // If a TWA does not implement PAY intent in any of its activities, then
    // |activity_names| is empty, which is not an error.
    std::move(callback).Run(/*error_message=*/base::nullopt,
                            /*app_descriptions=*/{});
    return;
  }

  if (response->get_valid()->activity_names.size() != 1U) {
    std::move(callback).Run(errors::kMoreThanOneActivity,
                            /*app_descriptions=*/{});
    return;
  }

  auto activity = std::make_unique<AndroidActivityDescription>();
  activity->name = response->get_valid()->activity_names.front();

  // The only available payment method identifier in a Chrome OS TWA at this
  // time.
  activity->default_payment_method = methods::kGooglePlayBilling;

  auto app = std::make_unique<AndroidAppDescription>();
  app->package = twa_package_name;
  app->activities.emplace_back(std::move(activity));
  app->service_names = response->get_valid()->service_names;

  std::vector<std::unique_ptr<AndroidAppDescription>> app_descriptions;
  app_descriptions.emplace_back(std::move(app));

  std::move(callback).Run(/*error_message=*/base::nullopt,
                          std::move(app_descriptions));
}

void OnIsReadyToPay(AndroidAppCommunication::IsReadyToPayCallback callback,
                    arc::mojom::IsReadyToPayResultPtr response) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (response.is_null()) {
    std::move(callback).Run(errors::kEmptyResponse, /*is_ready_to_pay=*/false);
    return;
  }

  if (response->is_error()) {
    std::move(callback).Run(response->get_error(), /*is_ready_to_pay=*/false);
    return;
  }

  if (!response->is_response()) {
    std::move(callback).Run(errors::kInvalidResponse,
                            /*is_ready_to_pay=*/false);
    return;
  }

  std::move(callback).Run(/*error_message=*/base::nullopt,
                          response->get_response());
}

void OnPaymentAppResponse(
    AndroidAppCommunication::InvokePaymentAppCallback callback,
    arc::mojom::InvokePaymentAppResultPtr response) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (response.is_null()) {
    std::move(callback).Run(errors::kEmptyResponse,
                            /*is_activity_result_ok=*/false,
                            /*payment_method_identifier=*/"",
                            /*stringified_details=*/kEmptyDictionaryJson);
    return;
  }

  if (response->is_error()) {
    std::move(callback).Run(response->get_error(),
                            /*is_activity_result_ok=*/false,
                            /*payment_method_identifier=*/"",
                            /*stringified_details=*/kEmptyDictionaryJson);
    return;
  }

  if (!response->is_valid()) {
    std::move(callback).Run(errors::kInvalidResponse,
                            /*is_activity_result_ok=*/false,
                            /*payment_method_identifier=*/"",
                            /*stringified_details=*/kEmptyDictionaryJson);
    return;
  }

  // Chrome OS TWA currently supports only methods::kGooglePlayBilling payment
  // method identifier.
  std::move(callback).Run(
      /*error_message=*/base::nullopt,
      response->get_valid()->is_activity_result_ok,
      /*payment_method_identifier=*/methods::kGooglePlayBilling,
      response->get_valid()->stringified_details);
}

arc::mojom::PaymentParametersPtr CreatePaymentParameters(
    const std::string& package_name,
    const std::string& activity_or_service_name,
    const std::map<std::string, std::set<std::string>>& stringified_method_data,
    const GURL& top_level_origin,
    const GURL& payment_request_origin,
    const std::string& payment_request_id,
    base::Optional<std::string>* error_message) {
  // Chrome OS TWA supports only kGooglePlayBilling payment method identifier
  // at this time.
  auto supported_method_iterator =
      stringified_method_data.find(methods::kGooglePlayBilling);
  if (supported_method_iterator == stringified_method_data.end())
    return nullptr;

  // Chrome OS TWA supports only one set of payment method specific data.
  if (supported_method_iterator->second.size() > 1) {
    *error_message = errors::kMoreThanOneMethodData;
    return nullptr;
  }

  auto parameters = arc::mojom::PaymentParameters::New();
  parameters->stringified_method_data =
      supported_method_iterator->second.empty()
          ? kEmptyDictionaryJson
          : *supported_method_iterator->second.begin();
  parameters->package_name = package_name;
  parameters->activity_or_service_name = activity_or_service_name;
  parameters->top_level_origin = top_level_origin.spec();
  parameters->payment_request_origin = payment_request_origin.spec();
  parameters->payment_request_id = payment_request_id;

  return parameters;
}

std::vector<std::unique_ptr<AndroidAppDescription>> CreateAppForTesting(
    const std::string& package_name,
    const std::string& method_name) {
  auto activity = std::make_unique<AndroidActivityDescription>();
  activity->name = package_name + ".Activity";
  activity->default_payment_method = method_name;
  auto app = std::make_unique<AndroidAppDescription>();
  app->package = package_name;
  app->activities.emplace_back(std::move(activity));
  std::vector<std::unique_ptr<AndroidAppDescription>> app_descriptions;
  app_descriptions.emplace_back(std::move(app));
  return app_descriptions;
}

// Invokes the TWA Android app in Android subsystem on Chrome OS.
class AndroidAppCommunicationChromeOS : public AndroidAppCommunication {
 public:
  explicit AndroidAppCommunicationChromeOS(content::BrowserContext* context)
      : AndroidAppCommunication(context),
        get_app_service_(base::BindRepeating(
            &arc::ArcPaymentAppBridge::GetForBrowserContext)) {}

  ~AndroidAppCommunicationChromeOS() override = default;

  // Disallow copy and assign.
  AndroidAppCommunicationChromeOS(
      const AndroidAppCommunicationChromeOS& other) = delete;
  AndroidAppCommunicationChromeOS& operator=(
      const AndroidAppCommunicationChromeOS& other) = delete;

  // AndroidAppCommunication implementation:
  void GetAppDescriptions(const std::string& twa_package_name,
                          GetAppDescriptionsCallback callback) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (twa_package_name.empty()) {
      // Chrome OS supports Android app payment only through a TWA. An empty
      // |twa_package_name| indicates that Chrome was not launched from a TWA,
      // so there're no payment apps available.
      std::move(callback).Run(/*error_message=*/base::nullopt,
                              /*app_descriptions=*/{});
      return;
    }

    if (!package_name_for_testing_.empty()) {
      std::move(callback).Run(
          /*error_message=*/base::nullopt,
          CreateAppForTesting(package_name_for_testing_, method_for_testing_));
      return;
    }

    auto* payment_app_service = get_app_service_.Run(context());
    if (!payment_app_service) {
      std::move(callback).Run(errors::kUnableToInvokeAndroidPaymentApps,
                              /*app_descriptions=*/{});
      return;
    }

    payment_app_service->IsPaymentImplemented(
        twa_package_name, base::BindOnce(&OnIsImplemented, twa_package_name,
                                         std::move(callback)));
  }

  // AndroidAppCommunication implementation.
  void IsReadyToPay(const std::string& package_name,
                    const std::string& service_name,
                    const std::map<std::string, std::set<std::string>>&
                        stringified_method_data,
                    const GURL& top_level_origin,
                    const GURL& payment_request_origin,
                    const std::string& payment_request_id,
                    IsReadyToPayCallback callback) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    auto* payment_app_service = get_app_service_.Run(context());
    if (!payment_app_service) {
      std::move(callback).Run(errors::kUnableToInvokeAndroidPaymentApps,
                              /*is_ready_to_pay=*/false);
      return;
    }

    base::Optional<std::string> error_message;
    auto parameters = CreatePaymentParameters(
        package_name, service_name, stringified_method_data, top_level_origin,
        payment_request_origin, payment_request_id, &error_message);
    if (!parameters) {
      std::move(callback).Run(error_message, /*is_ready_to_pay=*/false);
      return;
    }

    payment_app_service->IsReadyToPay(
        std::move(parameters),
        base::BindOnce(&OnIsReadyToPay, std::move(callback)));
  }

  // AndroidAppCommunication implementation.
  void InvokePaymentApp(const std::string& package_name,
                        const std::string& activity_name,
                        const std::map<std::string, std::set<std::string>>&
                            stringified_method_data,
                        const GURL& top_level_origin,
                        const GURL& payment_request_origin,
                        const std::string& payment_request_id,
                        InvokePaymentAppCallback callback) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    base::Optional<std::string> error_message;
    if (package_name_for_testing_ == package_name) {
      std::move(callback).Run(error_message,
                              /*is_activity_result_ok=*/true,
                              method_for_testing_, response_for_testing_);
      return;
    }

    auto* payment_app_service = get_app_service_.Run(context());
    if (!payment_app_service) {
      std::move(callback).Run(errors::kUnableToInvokeAndroidPaymentApps,
                              /*is_activity_result_ok=*/false,
                              /*payment_method_identifier=*/"",
                              /*stringified_details=*/kEmptyDictionaryJson);
      return;
    }

    auto parameters = CreatePaymentParameters(
        package_name, activity_name, stringified_method_data, top_level_origin,
        payment_request_origin, payment_request_id, &error_message);
    if (!parameters) {
      std::move(callback).Run(error_message,
                              /*is_activity_result_ok=*/false,
                              /*payment_method_identifier=*/"",
                              /*stringified_details=*/kEmptyDictionaryJson);
      return;
    }

    payment_app_service->InvokePaymentApp(
        std::move(parameters),
        base::BindOnce(&OnPaymentAppResponse, std::move(callback)));
  }

  // AndroidAppCommunication implementation.
  void SetForTesting() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    get_app_service_ = base::BindRepeating(
        &arc::ArcPaymentAppBridge::GetForBrowserContextForTesting);
  }

  // AndroidAppCommunication implementation.
  void SetAppForTesting(const std::string& package_name,
                        const std::string& method,
                        const std::string& response) override {
    package_name_for_testing_ = package_name;
    method_for_testing_ = method;
    response_for_testing_ = response;
  }

 private:
  std::string package_name_for_testing_;
  std::string method_for_testing_;
  std::string response_for_testing_;

  base::RepeatingCallback<arc::ArcPaymentAppBridge*(content::BrowserContext*)>
      get_app_service_;
};

}  // namespace

// Declared in cross-platform header file. See:
// //components/payments/content/android_app_communication.h
// static
std::unique_ptr<AndroidAppCommunication> AndroidAppCommunication::Create(
    content::BrowserContext* context) {
  return std::make_unique<AndroidAppCommunicationChromeOS>(context);
}

}  // namespace payments