#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/tuya_ble_device/tuya_ble_device.h"

namespace esphome {
namespace tuya_ble_binary_sensor {

class TuyaBLEBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_tuya_ble_device(tuya_ble_device::TuyaBLEDevice *device) { this->device_ = device; }
  void set_dp_id(uint8_t dp_id) { this->dp_id_ = dp_id; }

 protected:
  void on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp);

  tuya_ble_device::TuyaBLEDevice *device_{nullptr};
  uint8_t dp_id_{0};
};

}  // namespace tuya_ble_binary_sensor
}  // namespace esphome
