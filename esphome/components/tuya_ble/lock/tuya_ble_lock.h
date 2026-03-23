#pragma once

#include "esphome/core/component.h"
#include "esphome/components/lock/lock.h"
#include "esphome/components/tuya_ble_device/tuya_ble_device.h"

namespace esphome {
namespace tuya_ble_lock {

class TuyaBLELock : public lock::Lock, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_tuya_ble_device(tuya_ble_device::TuyaBLEDevice *device) { this->device_ = device; }
  void set_lock_state_dp(uint8_t dp) { this->lock_state_dp_ = dp; }
  void set_lock_control_dp(uint8_t dp) { this->lock_control_dp_ = dp; }
  void set_invert_state(bool invert) { this->invert_state_ = invert; }

 protected:
  void control(const lock::LockCall &call) override;
  void on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp);

  tuya_ble_device::TuyaBLEDevice *device_{nullptr};
  uint8_t lock_state_dp_{0};
  uint8_t lock_control_dp_{0};
  bool invert_state_{true};
};

}  // namespace tuya_ble_lock
}  // namespace esphome
