#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/tuya_ble_device/tuya_ble_device.h"

namespace esphome {
namespace tuya_ble_sensor {

class TuyaBLESensor : public sensor::Sensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_tuya_ble_device(tuya_ble_device::TuyaBLEDevice *device) { this->device_ = device; }
  void set_dp_id(uint8_t dp_id) { this->dp_id_ = dp_id; }
  void set_dp_type(int dp_type) { this->dp_type_override_ = dp_type; }
  void set_coefficient(float coefficient) { this->coefficient_ = coefficient; }

 protected:
  void on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp);

  tuya_ble_device::TuyaBLEDevice *device_{nullptr};
  uint8_t dp_id_{0};
  optional<int> dp_type_override_{};
  float coefficient_{1.0f};
};

}  // namespace tuya_ble_sensor
}  // namespace esphome
