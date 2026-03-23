#include "tuya_ble_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_sensor {

static const char *const TAG = "tuya_ble.sensor";

void TuyaBLESensor::setup() {
  this->device_->register_listener(
      this->dp_id_, [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
        this->on_dp_update_(dp);
      });
}

void TuyaBLESensor::dump_config() {
  LOG_SENSOR("", "Tuya BLE Sensor", this);
  ESP_LOGCONFIG(TAG, "  DP: %u", this->dp_id_);
  ESP_LOGCONFIG(TAG, "  Coefficient: %.2f", this->coefficient_);
}

void TuyaBLESensor::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  float value;
  switch (dp.type) {
    case tuya_ble_device::DT_BOOL:
      value = dp.value_bool ? 1.0f : 0.0f;
      break;
    case tuya_ble_device::DT_VALUE:
    case tuya_ble_device::DT_ENUM:
      value = static_cast<float>(dp.value_int) / this->coefficient_;
      break;
    default:
      ESP_LOGW(TAG, "Unsupported DP type %d for sensor", dp.type);
      return;
  }
  this->publish_state(value);
}

}  // namespace tuya_ble_sensor
}  // namespace esphome
