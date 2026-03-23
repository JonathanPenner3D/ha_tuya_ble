#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/tuya_ble_device/tuya_ble_device.h"

namespace esphome {
namespace tuya_ble_cover {

class TuyaBLECover : public cover::Cover, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_tuya_ble_device(tuya_ble_device::TuyaBLEDevice *device) { this->device_ = device; }
  void set_state_dp(uint8_t dp) { this->state_dp_ = dp; }
  void set_position_dp(uint8_t dp) { this->position_dp_ = dp; }
  void set_position_set_dp(uint8_t dp) { this->position_set_dp_ = dp; }
  void set_tilt_dp(uint8_t dp) { this->tilt_dp_ = dp; }

  cover::CoverTraits get_traits() override;

 protected:
  void control(const cover::CoverCall &call) override;
  void on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp);

  tuya_ble_device::TuyaBLEDevice *device_{nullptr};
  optional<uint8_t> state_dp_{};
  optional<uint8_t> position_dp_{};
  optional<uint8_t> position_set_dp_{};
  optional<uint8_t> tilt_dp_{};
};

}  // namespace tuya_ble_cover
}  // namespace esphome
