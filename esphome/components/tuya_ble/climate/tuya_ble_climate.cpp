#include "tuya_ble_climate.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_climate {

static const char *const TAG = "tuya_ble.climate";

void TuyaBLEClimate::setup() {
  this->device_->register_listener(this->switch_dp_,
                                    [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                      this->on_dp_update_(dp);
                                    });
  this->device_->register_listener(this->current_temp_dp_,
                                    [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                      this->on_dp_update_(dp);
                                    });
  this->device_->register_listener(this->target_temp_dp_,
                                    [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                      this->on_dp_update_(dp);
                                    });
}

void TuyaBLEClimate::dump_config() {
  LOG_CLIMATE("", "Tuya BLE Climate", this);
  ESP_LOGCONFIG(TAG, "  Switch DP: %u", this->switch_dp_);
  ESP_LOGCONFIG(TAG, "  Current Temp DP: %u (coefficient: %.1f)", this->current_temp_dp_, this->temp_coefficient_);
  ESP_LOGCONFIG(TAG, "  Target Temp DP: %u (step: %.1f)", this->target_temp_dp_, this->temp_step_);
  ESP_LOGCONFIG(TAG, "  Range: %.1f-%.1f°C", this->temp_min_, this->temp_max_);
}

climate::ClimateTraits TuyaBLEClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(this->temp_min_);
  traits.set_visual_max_temperature(this->temp_max_);
  traits.set_visual_temperature_step(this->temp_step_);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});
  return traits;
}

void TuyaBLEClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    auto mode = call.get_mode().value();
    tuya_ble_device::TuyaBLEDatapoint dp;
    dp.id = this->switch_dp_;
    dp.type = tuya_ble_device::DT_BOOL;
    dp.value_bool = (mode != climate::CLIMATE_MODE_OFF);
    this->device_->send_datapoint(dp);

    this->mode = mode;
  }

  if (call.get_target_temperature().has_value()) {
    float target = call.get_target_temperature().value();
    tuya_ble_device::TuyaBLEDatapoint dp;
    dp.id = this->target_temp_dp_;
    dp.type = tuya_ble_device::DT_VALUE;
    dp.value_int = (int32_t) (target * this->temp_coefficient_);
    this->device_->send_datapoint(dp);

    this->target_temperature = target;
  }

  this->publish_state();
}

void TuyaBLEClimate::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  if (dp.id == this->switch_dp_) {
    this->mode = dp.value_bool ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_OFF;
  } else if (dp.id == this->current_temp_dp_) {
    this->current_temperature = (float) dp.value_int / this->temp_coefficient_;
  } else if (dp.id == this->target_temp_dp_) {
    this->target_temperature = (float) dp.value_int / this->temp_coefficient_;
  }
  this->publish_state();
}

}  // namespace tuya_ble_climate
}  // namespace esphome
