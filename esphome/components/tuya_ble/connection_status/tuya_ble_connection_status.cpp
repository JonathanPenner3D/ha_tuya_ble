#include "tuya_ble_connection_status.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_ble_connection_status {

static const char *const TAG = "tuya_ble.connection_status";

void TuyaBLEConnectionStatus::setup() {
  // Publish initial state
  this->publish_state(this->device_->is_paired());

  // Register for connection state changes
  this->device_->register_connection_listener([this](bool connected) {
    ESP_LOGI(TAG, "Connection status changed: %s", connected ? "CONNECTED" : "DISCONNECTED");
    this->publish_state(connected);
  });
}

void TuyaBLEConnectionStatus::dump_config() {
  LOG_BINARY_SENSOR("", "Tuya BLE Connection Status", this);
}

}  // namespace tuya_ble_connection_status
}  // namespace esphome
