// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_SHARING_NEARBY_PLATFORM_V2_BLE_PERIPHERAL_H_
#define CHROME_SERVICES_SHARING_NEARBY_PLATFORM_V2_BLE_PERIPHERAL_H_

#include <map>
#include <string>

#include "device/bluetooth/public/cpp/bluetooth_uuid.h"
#include "device/bluetooth/public/mojom/adapter.mojom.h"
#include "third_party/nearby/src/cpp/platform_v2/api/ble.h"

namespace location {
namespace nearby {
namespace chrome {

// Concrete BlePeripheral implementation.
class BlePeripheral : public api::BlePeripheral {
 public:
  BlePeripheral(bluetooth::mojom::DeviceInfoPtr device_info,
                const std::map<std::string, device::BluetoothUUID>&
                    service_id_to_fast_advertisement_service_uuid_map);
  ~BlePeripheral() override;

  BlePeripheral(const BlePeripheral&) = delete;
  BlePeripheral& operator=(const BlePeripheral&) = delete;
  BlePeripheral(BlePeripheral&&);
  BlePeripheral& operator=(BlePeripheral&&);

  // api::BlePeripheral:
  std::string GetName() const override;
  ByteArray GetAdvertisementBytes(const std::string& service_id) const override;

  void UpdateDeviceInfo(bluetooth::mojom::DeviceInfoPtr device_info);
  void UpdateIdToUuidMap(const std::map<std::string, device::BluetoothUUID>&
                             service_id_to_fast_advertisement_service_uuid_map);

 private:
  bluetooth::mojom::DeviceInfoPtr device_info_;
  std::map<std::string, device::BluetoothUUID>
      service_id_to_fast_advertisement_service_uuid_map_;
};

}  // namespace chrome
}  // namespace nearby
}  // namespace location

#endif  // CHROME_SERVICES_SHARING_NEARBY_PLATFORM_V2_BLE_PERIPHERAL_H_