#include "tuya_ble_number.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_number {

static const char *const TAG = "tuya_ble.number";

void TuyaBLENumber::setup() {
  this->device_->register_listener(
      this->dp_id_, [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
        this->on_dp_update_(dp);
      });
}

void TuyaBLENumber::dump_config() {
  LOG_NUMBER("", "Tuya BLE Number", this);
  ESP_LOGCONFIG(TAG, "  DP: %u", this->dp_id_);
  ESP_LOGCONFIG(TAG, "  Coefficient: %.2f", this->coefficient_);
}

void TuyaBLENumber::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  // Remember the type for when we send back
  this->last_seen_type_ = dp.type;

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
      ESP_LOGW(TAG, "Unsupported DP type %d for number", dp.type);
      return;
  }
  this->publish_state(value);
}

void TuyaBLENumber::control(float value) {
  tuya_ble_device::TuyaBLEDatapoint dp;
  dp.id = this->dp_id_;

  // Use override type if set, otherwise use auto-detected type
  if (this->dp_type_override_.has_value()) {
    dp.type = static_cast<tuya_ble_device::TuyaBLEDataPointType>(this->dp_type_override_.value());
  } else {
    dp.type = this->last_seen_type_;
  }

  dp.value_int = static_cast<int32_t>(value * this->coefficient_);
  dp.value_bool = (dp.value_int != 0);

  this->device_->send_datapoint(dp);
  this->publish_state(value);
}

}  // namespace tuya_ble_number
}  // namespace esphome
