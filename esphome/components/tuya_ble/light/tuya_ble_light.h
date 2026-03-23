#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/tuya_ble_device/tuya_ble_device.h"

namespace esphome {
namespace tuya_ble_light {

class TuyaBLELight : public light::LightOutput, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_tuya_ble_device(tuya_ble_device::TuyaBLEDevice *device) { this->device_ = device; }
  void set_switch_dp(uint8_t dp) { this->switch_dp_ = dp; }
  void set_brightness_dp(uint8_t dp) { this->brightness_dp_ = dp; }
  void set_brightness_max(int max) { this->brightness_max_ = max; }
  void set_color_temp_dp(uint8_t dp) { this->color_temp_dp_ = dp; }
  void set_color_temp_min_mireds(int min) { this->color_temp_min_mireds_ = min; }
  void set_color_temp_max_mireds(int max) { this->color_temp_max_mireds_ = max; }
  void set_color_dp(uint8_t dp) { this->color_dp_ = dp; }
  void set_work_mode_dp(uint8_t dp) { this->work_mode_dp_ = dp; }

  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;

 protected:
  void on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp);
  void parse_color_data_(const std::string &data);

  tuya_ble_device::TuyaBLEDevice *device_{nullptr};
  light::LightState *state_{nullptr};

  uint8_t switch_dp_{0};
  optional<uint8_t> brightness_dp_{};
  int brightness_max_{1000};
  optional<uint8_t> color_temp_dp_{};
  int color_temp_min_mireds_{153};
  int color_temp_max_mireds_{500};
  optional<uint8_t> color_dp_{};
  optional<uint8_t> work_mode_dp_{};

  // Cached state from device
  bool device_on_{false};
  int device_brightness_{0};
  int device_color_temp_{0};
  float device_hue_{0};
  float device_saturation_{0};
  float device_value_{0};
  std::string device_work_mode_{"white"};
};

}  // namespace tuya_ble_light
}  // namespace esphome
