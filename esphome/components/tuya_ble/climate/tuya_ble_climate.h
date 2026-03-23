#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/tuya_ble_device/tuya_ble_device.h"

namespace esphome {
namespace tuya_ble_climate {

class TuyaBLEClimate : public climate::Climate, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_tuya_ble_device(tuya_ble_device::TuyaBLEDevice *device) { this->device_ = device; }
  void set_switch_dp(uint8_t dp) { this->switch_dp_ = dp; }
  void set_current_temp_dp(uint8_t dp) { this->current_temp_dp_ = dp; }
  void set_target_temp_dp(uint8_t dp) { this->target_temp_dp_ = dp; }
  void set_temp_coefficient(float coeff) { this->temp_coefficient_ = coeff; }
  void set_temp_step(float step) { this->temp_step_ = step; }
  void set_temp_min(float min) { this->temp_min_ = min; }
  void set_temp_max(float max) { this->temp_max_ = max; }

  climate::ClimateTraits traits() override;

 protected:
  void control(const climate::ClimateCall &call) override;
  void on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp);

  tuya_ble_device::TuyaBLEDevice *device_{nullptr};
  uint8_t switch_dp_{0};
  uint8_t current_temp_dp_{0};
  uint8_t target_temp_dp_{0};
  float temp_coefficient_{1.0f};
  float temp_step_{1.0f};
  float temp_min_{5.0f};
  float temp_max_{30.0f};
};

}  // namespace tuya_ble_climate
}  // namespace esphome
