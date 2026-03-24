#include "tuya_ble_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_text_sensor {

static const char *const TAG = "tuya_ble.text_sensor";

void TuyaBLETextSensor::setup() {
  this->device_->register_listener(
      this->dp_id_, [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
        this->on_dp_update_(dp);
      });
}

void TuyaBLETextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Tuya BLE Text Sensor", this);
  ESP_LOGCONFIG(TAG, "  DP: %u", this->dp_id_);
  if (!this->enum_values_.empty()) {
    ESP_LOGCONFIG(TAG, "  Enum values:");
    for (const auto &pair : this->enum_values_) {
      ESP_LOGCONFIG(TAG, "    %d: %s", pair.first, pair.second.c_str());
    }
  }
}

void TuyaBLETextSensor::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  std::string value;
  switch (dp.type) {
    case tuya_ble_device::DT_STRING:
      value = dp.value_string;
      break;
    case tuya_ble_device::DT_VALUE:
    case tuya_ble_device::DT_ENUM: {
      auto it = this->enum_values_.find(dp.value_int);
      if (it != this->enum_values_.end()) {
        value = it->second;
      } else {
        value = std::to_string(dp.value_int);
      }
      break;
    }
    case tuya_ble_device::DT_BOOL:
      value = dp.value_bool ? "true" : "false";
      break;
    default:
      value = "";
      for (uint8_t b : dp.value_raw) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", b);
        value += buf;
      }
      break;
  }
  ESP_LOGD(TAG, "DP %u: publishing '%s' (raw_int=%d, type=%u)",
           this->dp_id_, value.c_str(), dp.value_int, dp.type);
  this->publish_state(value);
}

}  // namespace tuya_ble_text_sensor
}  // namespace esphome
