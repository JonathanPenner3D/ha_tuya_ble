#include "tuya_ble_cover.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_cover {

static const char *const TAG = "tuya_ble.cover";

void TuyaBLECover::setup() {
  // Register for DP updates
  if (this->state_dp_.has_value()) {
    this->device_->register_listener(this->state_dp_.value(),
                                      [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                        this->on_dp_update_(dp);
                                      });
  }
  if (this->position_dp_.has_value()) {
    this->device_->register_listener(this->position_dp_.value(),
                                      [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                        this->on_dp_update_(dp);
                                      });
  }
  if (this->tilt_dp_.has_value()) {
    this->device_->register_listener(this->tilt_dp_.value(),
                                      [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                        this->on_dp_update_(dp);
                                      });
  }
}

void TuyaBLECover::dump_config() {
  LOG_COVER("", "Tuya BLE Cover", this);
  if (this->state_dp_.has_value())
    ESP_LOGCONFIG(TAG, "  State DP: %u", this->state_dp_.value());
  if (this->position_dp_.has_value())
    ESP_LOGCONFIG(TAG, "  Position DP: %u", this->position_dp_.value());
  if (this->position_set_dp_.has_value())
    ESP_LOGCONFIG(TAG, "  Position Set DP: %u", this->position_set_dp_.value());
  if (this->tilt_dp_.has_value())
    ESP_LOGCONFIG(TAG, "  Tilt DP: %u", this->tilt_dp_.value());
}

cover::CoverTraits TuyaBLECover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_supports_stop(this->state_dp_.has_value());
  traits.set_supports_position(this->position_set_dp_.has_value());
  traits.set_supports_tilt(this->tilt_dp_.has_value());
  return traits;
}

void TuyaBLECover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    if (this->state_dp_.has_value()) {
      tuya_ble_device::TuyaBLEDatapoint dp;
      dp.id = this->state_dp_.value();
      dp.type = tuya_ble_device::DT_ENUM;
      dp.value_int = 1;  // STOP
      this->device_->send_datapoint(dp);
    }
    return;
  }

  if (call.get_position().has_value()) {
    float pos = call.get_position().value();
    if (pos == cover::COVER_OPEN) {
      // Open command via state DP
      if (this->state_dp_.has_value()) {
        tuya_ble_device::TuyaBLEDatapoint dp;
        dp.id = this->state_dp_.value();
        dp.type = tuya_ble_device::DT_ENUM;
        dp.value_int = 0;  // OPEN
        this->device_->send_datapoint(dp);
      }
    } else if (pos == cover::COVER_CLOSED) {
      // Close command via state DP
      if (this->state_dp_.has_value()) {
        tuya_ble_device::TuyaBLEDatapoint dp;
        dp.id = this->state_dp_.value();
        dp.type = tuya_ble_device::DT_ENUM;
        dp.value_int = 2;  // CLOSE
        this->device_->send_datapoint(dp);
      }
    } else if (this->position_set_dp_.has_value()) {
      // Set specific position (inverted: send 100 - desired)
      int raw_pos = 100 - (int)(pos * 100);
      tuya_ble_device::TuyaBLEDatapoint dp;
      dp.id = this->position_set_dp_.value();
      dp.type = tuya_ble_device::DT_VALUE;
      dp.value_int = raw_pos;
      this->device_->send_datapoint(dp);
    }
  }

  if (call.get_tilt().has_value() && this->tilt_dp_.has_value()) {
    float tilt = call.get_tilt().value();
    // Convert 0-100% to 1-10 range
    int raw_tilt = (int)(tilt * 9.0f / 100.0f) + 1;
    if (raw_tilt < 1) raw_tilt = 1;
    if (raw_tilt > 10) raw_tilt = 10;
    tuya_ble_device::TuyaBLEDatapoint dp;
    dp.id = this->tilt_dp_.value();
    dp.type = tuya_ble_device::DT_VALUE;
    dp.value_int = raw_tilt;
    this->device_->send_datapoint(dp);
  }
}

void TuyaBLECover::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  if (this->position_dp_.has_value() && dp.id == this->position_dp_.value()) {
    // Position is inverted: displayed = 100 - raw
    float pos = (100.0f - dp.value_int) / 100.0f;
    if (pos < 0) pos = 0;
    if (pos > 1) pos = 1;
    this->position = pos;
    this->publish_state();
  }

  if (this->state_dp_.has_value() && dp.id == this->state_dp_.value()) {
    // State: 0=OPEN, 1=STOP, 2=CLOSE
    switch (dp.value_int) {
      case 0:
        this->current_operation = cover::COVER_OPERATION_OPENING;
        break;
      case 1:
        this->current_operation = cover::COVER_OPERATION_IDLE;
        break;
      case 2:
        this->current_operation = cover::COVER_OPERATION_CLOSING;
        break;
    }
    this->publish_state();
  }

  if (this->tilt_dp_.has_value() && dp.id == this->tilt_dp_.value()) {
    // Tilt: 1-10 range → 0-100%
    float tilt = (dp.value_int - 1) / 9.0f;
    if (tilt < 0) tilt = 0;
    if (tilt > 1) tilt = 1;
    this->tilt = tilt;
    this->publish_state();
  }
}

}  // namespace tuya_ble_cover
}  // namespace esphome
