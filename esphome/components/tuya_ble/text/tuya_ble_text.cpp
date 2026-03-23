#include "tuya_ble_text.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_text {

static const char *const TAG = "tuya_ble.text";

void TuyaBLEText::setup() {
  this->device_->register_listener(
      this->dp_id_, [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
        this->on_dp_update_(dp);
      });
}

void TuyaBLEText::dump_config() {
  LOG_TEXT("", "Tuya BLE Text", this);
  ESP_LOGCONFIG(TAG, "  DP: %u", this->dp_id_);
}

void TuyaBLEText::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  std::string value;
  switch (dp.type) {
    case tuya_ble_device::DT_STRING:
      value = dp.value_string;
      break;
    case tuya_ble_device::DT_VALUE:
    case tuya_ble_device::DT_ENUM:
      value = std::to_string(dp.value_int);
      break;
    case tuya_ble_device::DT_BOOL:
      value = dp.value_bool ? "true" : "false";
      break;
    default:
      // For raw/bitmap, show hex
      value = "";
      for (uint8_t b : dp.value_raw) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", b);
        value += buf;
      }
      break;
  }
  this->publish_state(value);
}

void TuyaBLEText::control(const std::string &value) {
  tuya_ble_device::TuyaBLEDatapoint dp;
  dp.id = this->dp_id_;
  dp.type = tuya_ble_device::DT_STRING;
  dp.value_string = value;

  this->device_->send_datapoint(dp);
  this->publish_state(value);
}

}  // namespace tuya_ble_text
}  // namespace esphome
