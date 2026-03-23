#include "tuya_ble_lock.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_lock {

static const char *const TAG = "tuya_ble.lock";

void TuyaBLELock::setup() {
  this->device_->register_listener(this->lock_state_dp_,
                                    [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                      this->on_dp_update_(dp);
                                    });
}

void TuyaBLELock::dump_config() {
  ESP_LOGCONFIG(TAG, "Tuya BLE Lock:");
  ESP_LOGCONFIG(TAG, "  Lock State DP: %u", this->lock_state_dp_);
  ESP_LOGCONFIG(TAG, "  Lock Control DP: %u", this->lock_control_dp_);
  ESP_LOGCONFIG(TAG, "  Invert State: %s", this->invert_state_ ? "true" : "false");
}

void TuyaBLELock::control(const lock::LockCall &call) {
  auto state = call.get_state();
  tuya_ble_device::TuyaBLEDatapoint dp;
  dp.id = this->lock_control_dp_;
  dp.type = tuya_ble_device::DT_BOOL;

  if (state == lock::LOCK_STATE_LOCKED) {
    dp.value_bool = true;
    this->device_->send_datapoint(dp);
    this->publish_state(lock::LOCK_STATE_LOCKED);
  } else if (state == lock::LOCK_STATE_UNLOCKED) {
    dp.value_bool = false;
    this->device_->send_datapoint(dp);
    this->publish_state(lock::LOCK_STATE_UNLOCKED);
  }
}

void TuyaBLELock::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  if (dp.id == this->lock_state_dp_) {
    bool raw_state = dp.value_bool;
    // HA integration inverts lock_motor_state: locked = !motor_state
    bool is_locked = this->invert_state_ ? !raw_state : raw_state;
    this->publish_state(is_locked ? lock::LOCK_STATE_LOCKED : lock::LOCK_STATE_UNLOCKED);
  }
}

}  // namespace tuya_ble_lock
}  // namespace esphome
