#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/tuya_ble_device/tuya_ble_device.h"

namespace esphome {
namespace tuya_ble_connection_status {

class TuyaBLEConnectionStatus : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_tuya_ble_device(tuya_ble_device::TuyaBLEDevice *device) { this->device_ = device; }

 protected:
  tuya_ble_device::TuyaBLEDevice *device_{nullptr};
};

}  // namespace tuya_ble_connection_status
}  // namespace esphome
