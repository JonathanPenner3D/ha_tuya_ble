#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble/ble_uuid.h"
#include "esphome/core/log.h"

#include <mbedtls/aes.h>
#include <mbedtls/md5.h>

#include <vector>
#include <map>
#include <functional>
#include <cstring>

namespace esphome {
namespace tuya_ble_device {

static const uint8_t GATT_MTU = 20;

// GATT characteristics — using esp32_ble::ESPBTUUID
static const esp32_ble::ESPBTUUID SERVICE_UUID =
    esp32_ble::ESPBTUUID::from_raw("0000fd50-0000-1000-8000-00805f9b34fb");
static const esp32_ble::ESPBTUUID CHAR_NOTIFY =
    esp32_ble::ESPBTUUID::from_raw("00002b10-0000-1000-8000-00805f9b34fb");
static const esp32_ble::ESPBTUUID CHAR_WRITE =
    esp32_ble::ESPBTUUID::from_raw("00002b11-0000-1000-8000-00805f9b34fb");

// Command codes
enum TuyaBLECode : uint16_t {
  FUN_SENDER_DEVICE_INFO = 0x0000,
  FUN_SENDER_PAIR = 0x0001,
  FUN_SENDER_DPS = 0x0002,
  FUN_SENDER_DEVICE_STATUS = 0x0003,
  FUN_RECEIVE_DP = 0x8001,
  FUN_RECEIVE_TIME_DP = 0x8003,
  FUN_RECEIVE_SIGN_DP = 0x8004,
  FUN_RECEIVE_SIGN_TIME_DP = 0x8005,
  FUN_RECEIVE_TIME1_REQ = 0x8011,
  FUN_RECEIVE_TIME2_REQ = 0x8012,
};

// Datapoint types
enum TuyaBLEDataPointType : uint8_t {
  DT_RAW = 0,
  DT_BOOL = 1,
  DT_VALUE = 2,
  DT_STRING = 3,
  DT_ENUM = 4,
  DT_BITMAP = 5,
};

// A single datapoint value
struct TuyaBLEDatapoint {
  uint8_t id;
  TuyaBLEDataPointType type;
  union {
    bool value_bool;
    int32_t value_int;
  };
  std::string value_string;
  std::vector<uint8_t> value_raw;
};

// Callback type for datapoint updates
using DPListener = std::function<void(const TuyaBLEDatapoint &)>;

class TuyaBLEDevice : public Component, public ble_client::BLEClientNode {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_BLUETOOTH; }

  // Configuration
  void set_local_key(const std::string &key) { this->local_key_str_ = key; }
  void set_uuid(const std::string &uuid) { this->uuid_ = uuid; }
  void set_device_id(const std::string &device_id) { this->device_id_ = device_id; }

  // BLE client callbacks
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                            esp_ble_gattc_cb_param_t *param) override;

  // Entity registration
  void register_listener(uint8_t dp_id, const DPListener &listener);

  // Send a datapoint value to the device
  void send_datapoint(const TuyaBLEDatapoint &dp);

  // Check if connected and paired
  bool is_paired() const { return this->is_paired_; }

 protected:
  // Key management
  void derive_keys_();
  void compute_md5_(const uint8_t *input, size_t len, uint8_t *output);
  void get_key_for_security_flag_(uint8_t flag, uint8_t *key_out);

  // Encryption
  void aes_cbc_encrypt_(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *input, size_t input_len,
                         uint8_t *output);
  void aes_cbc_decrypt_(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *input, size_t input_len,
                         uint8_t *output);

  // CRC
  static uint16_t calc_crc16_(const uint8_t *data, size_t len);

  // Variable-length integer encoding
  static std::vector<uint8_t> pack_varint_(uint32_t value);
  static bool unpack_varint_(const uint8_t *data, size_t data_len, size_t &pos, uint32_t &result);

  // Packet building & sending
  std::vector<std::vector<uint8_t>> build_packets_(uint32_t seq_num, uint16_t code,
                                                     const uint8_t *data, size_t data_len,
                                                     uint32_t response_to = 0);
  void send_command_(uint16_t code, const std::vector<uint8_t> &data);
  void send_response_(uint16_t code, const std::vector<uint8_t> &data, uint32_t response_to);
  void write_packets_(const std::vector<std::vector<uint8_t>> &packets);

  // Incoming data handling
  void handle_notification_(const uint8_t *data, size_t len);
  void parse_input_();
  void clean_input_();
  void handle_command_or_response_(uint32_t seq_num, uint32_t response_to,
                                    uint16_t code, const uint8_t *data, size_t len);
  void parse_datapoints_v3_(const uint8_t *data, size_t len, size_t start_pos);
  size_t parse_timestamp_(const uint8_t *data, size_t len, size_t start_pos);

  // Connection state machine
  void start_pairing_();
  std::vector<uint8_t> build_pairing_request_();

  // Time response
  void send_time1_response_(uint32_t seq_num);
  void send_time2_response_(uint32_t seq_num);

  // DP encoding
  static std::vector<uint8_t> encode_dp_value_(const TuyaBLEDatapoint &dp);

  // Configuration
  std::string local_key_str_;
  std::string uuid_;
  std::string device_id_;

  // Derived keys
  uint8_t local_key_[6]{};
  uint8_t login_key_[16]{};
  uint8_t session_key_[16]{};
  uint8_t auth_key_[32]{};

  // Connection state
  bool is_paired_{false};
  uint8_t protocol_version_{3};
  uint32_t current_seq_num_{1};
  bool device_info_received_{false};
  bool pair_request_sent_{false};

  // BLE handles
  uint16_t notify_handle_{0};
  uint16_t write_handle_{0};

  // Input buffer for reassembling fragmented packets
  std::vector<uint8_t> input_buffer_;
  uint32_t input_expected_packet_num_{0};
  uint32_t input_expected_length_{0};

  // Packet queue for sending
  std::vector<std::vector<uint8_t>> pending_packets_;

  // DP listeners
  std::multimap<uint8_t, DPListener> dp_listeners_;

  // Retry/reconnect tracking
  uint32_t last_connect_attempt_{0};
};

}  // namespace tuya_ble_device
}  // namespace esphome
