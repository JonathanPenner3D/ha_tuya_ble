#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/tuya_ble_device/tuya_ble_device.h"

#include <map>
#include <string>

namespace esphome {
namespace tuya_ble_text_sensor {

class TuyaBLETextSensor : public text_sensor::TextSensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_tuya_ble_device(tuya_ble_device::TuyaBLEDevice *device) { this->device_ = device; }
  void set_dp_id(uint8_t dp_id) { this->dp_id_ = dp_id; }
  void add_enum_value(int key, const std::string &value) { this->enum_values_[key] = value; }

 protected:
  void on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp);

  tuya_ble_device::TuyaBLEDevice *device_{nullptr};
  uint8_t dp_id_{0};
  std::map<int, std::string> enum_values_;
};

}  // namespace tuya_ble_text_sensor
}  // namespace esphome
