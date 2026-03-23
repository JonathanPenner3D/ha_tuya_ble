#include "tuya_ble_binary_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_binary_sensor {

static const char *const TAG = "tuya_ble.binary_sensor";

void TuyaBLEBinarySensor::setup() {
  this->device_->register_listener(
      this->dp_id_, [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
        this->on_dp_update_(dp);
      });
}

void TuyaBLEBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "Tuya BLE Binary Sensor", this);
  ESP_LOGCONFIG(TAG, "  DP: %u", this->dp_id_);
}

void TuyaBLEBinarySensor::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  bool state;
  switch (dp.type) {
    case tuya_ble_device::DT_BOOL:
      state = dp.value_bool;
      break;
    case tuya_ble_device::DT_VALUE:
    case tuya_ble_device::DT_ENUM:
      state = (dp.value_int != 0);
      break;
    default:
      ESP_LOGW(TAG, "Unsupported DP type %d for binary sensor", dp.type);
      return;
  }
  this->publish_state(state);
}

}  // namespace tuya_ble_binary_sensor
}  // namespace esphome
