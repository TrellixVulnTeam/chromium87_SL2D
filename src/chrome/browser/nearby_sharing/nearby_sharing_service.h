// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NEARBY_SHARING_NEARBY_SHARING_SERVICE_H_
#define CHROME_BROWSER_NEARBY_SHARING_NEARBY_SHARING_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "chrome/browser/nearby_sharing/nearby_share_settings.h"
#include "chrome/browser/nearby_sharing/share_target_discovered_callback.h"
#include "chrome/browser/nearby_sharing/transfer_update_callback.h"
#include "components/keyed_service/core/keyed_service.h"

class NearbyNotificationDelegate;
class NearbyShareContactManager;
class NearbyShareCertificateManager;
class NearbyShareHttpNotifier;
class NearbyShareLocalDeviceDataManager;

// This service implements Nearby Sharing on top of the Nearby Connections mojo.
// Currently only single profile will be allowed to be bound at a time and only
// after the user has enabled Nearby Sharing in prefs.
class NearbySharingService : public KeyedService {
 public:
  enum class StatusCodes {
    // The operation failed, without any more information.
    kError,
    // The operation was successful.
    kOk,
    // The operation failed since it was called in an invalid order.
    kOutOfOrderApiCall,
    // Tried to stop something that was already stopped.
    kStatusAlreadyStopped,
    // Tried to register an opposite foreground surface in the midst of a
    // transfer or connection.
    // (Tried to register Send Surface when receiving a file or tried to
    // register Receive Surface when
    // sending a file.)
    kTransferAlreadyInProgress,
  };

  enum class ReceiveSurfaceState {
    // Default, invalid state.
    kUnknown,
    // Background receive surface advertises only to contacts.
    kBackground,
    // Foreground receive surface advertises to everyone.
    kForeground,
  };

  enum class SendSurfaceState {
    // Default, invalid state.
    kUnknown,
    // Background send surface only listens to transfer update.
    kBackground,
    // Foreground send surface both scans and listens to transfer update.
    kForeground,
  };

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnHighVisibilityChanged(bool in_high_visibility) = 0;

    // Called during the |KeyedService| shutdown, but before everything has been
    // cleaned up. It is safe to remove any observers on this event.
    virtual void OnShutdown() = 0;
  };

  using StatusCodesCallback =
      base::OnceCallback<void(StatusCodes status_codes)>;

  ~NearbySharingService() override = default;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
  virtual bool HasObserver(Observer* observer) = 0;

  // Registers a send surface for handling payload transfer status and device
  // discovery.
  virtual StatusCodes RegisterSendSurface(
      TransferUpdateCallback* transfer_callback,
      ShareTargetDiscoveredCallback* discovery_callback,
      SendSurfaceState state) = 0;

  // Unregisters the current send surface.
  virtual StatusCodes UnregisterSendSurface(
      TransferUpdateCallback* transfer_callback,
      ShareTargetDiscoveredCallback* discovery_callback) = 0;

  // Registers a receiver surface for handling payload transfer status.
  virtual StatusCodes RegisterReceiveSurface(
      TransferUpdateCallback* transfer_callback,
      ReceiveSurfaceState state) = 0;

  // Unregisters the current receive surface.
  virtual StatusCodes UnregisterReceiveSurface(
      TransferUpdateCallback* transfer_callback) = 0;

  // Returns true if a foreground receive surface is registered.
  virtual bool IsInHighVisibility() = 0;

  // Sends |attachments| to the remote |share_target|.
  virtual StatusCodes SendAttachments(
      const ShareTarget& share_target,
      std::vector<std::unique_ptr<Attachment>> attachments) = 0;

  // Accepts incoming share from the remote |share_target|.
  virtual void Accept(const ShareTarget& share_target,
                      StatusCodesCallback status_codes_callback) = 0;

  // Rejects incoming share from the remote |share_target|.
  virtual void Reject(const ShareTarget& share_target,
                      StatusCodesCallback status_codes_callback) = 0;

  // Cancels outoing shares to the remote |share_target|.
  virtual void Cancel(const ShareTarget& share_target,
                      StatusCodesCallback status_codes_callback) = 0;

  // Opens attachments from the remote |share_target|.
  virtual void Open(const ShareTarget& share_target,
                    StatusCodesCallback status_codes_callback) = 0;

  // Gets a delegate to handle events for |notification_id| or nullptr.
  virtual NearbyNotificationDelegate* GetNotificationDelegate(
      const std::string& notification_id) = 0;

  virtual NearbyShareSettings* GetSettings() = 0;
  virtual NearbyShareHttpNotifier* GetHttpNotifier() = 0;
  virtual NearbyShareLocalDeviceDataManager* GetLocalDeviceDataManager() = 0;
  virtual NearbyShareContactManager* GetContactManager() = 0;
  virtual NearbyShareCertificateManager* GetCertificateManager() = 0;
};

#endif  // CHROME_BROWSER_NEARBY_SHARING_NEARBY_SHARING_SERVICE_H_