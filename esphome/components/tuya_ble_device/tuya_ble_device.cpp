#include "tuya_ble_device.h"
#include "esphome/core/helpers.h"
#include "esphome/core/application.h"

#include <ctime>
#include <algorithm>

#ifdef USE_ESP32
#include "esp_random.h"
#endif

namespace esphome {
namespace tuya_ble_device {

static const char *const TAG = "tuya_ble_device";

void TuyaBLEDevice::setup() {
  this->derive_keys_();
  ESP_LOGI(TAG, "TuyaBLEDevice setup complete");
  ESP_LOGI(TAG, "  UUID: %s", this->uuid_.c_str());
  ESP_LOGI(TAG, "  Device ID: %s", this->device_id_.c_str());
  ESP_LOGI(TAG, "  Waiting for BLE connection...");
}

void TuyaBLEDevice::loop() {
  uint32_t now = millis();

  // Write any pending packets
  if (!this->pending_packets_.empty() && this->write_handle_ != 0) {
    auto packet = std::move(this->pending_packets_.front());
    this->pending_packets_.erase(this->pending_packets_.begin());

    auto status = esp_ble_gattc_write_char(
        this->parent()->get_gattc_if(), this->parent()->get_conn_id(),
        this->write_handle_, packet.size(), packet.data(),
        ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);

    if (status != ESP_OK) {
      ESP_LOGW(TAG, "Error writing to BLE characteristic: %d", status);
    }
  }

  // Check for pairing timeout - if we're connected but not paired for too long,
  // disconnect and let ble_client retry the connection from scratch
  if (this->ble_connected_ && !this->is_paired_ && this->pairing_start_time_ != 0) {
    if (now - this->pairing_start_time_ > PAIRING_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Pairing handshake timed out after %us (device_info_received=%s, pair_request_sent=%s)",
               PAIRING_TIMEOUT_MS / 1000,
               this->device_info_received_ ? "true" : "false",
               this->pair_request_sent_ ? "true" : "false");
      this->pairing_fail_count_++;
      ESP_LOGW(TAG, "Pairing failure count: %u, disconnecting to retry...", this->pairing_fail_count_);
      this->pairing_start_time_ = 0;
      // Disconnect - ble_client will automatically attempt to reconnect
      this->parent()->set_enabled(false);
      this->parent()->set_enabled(true);
    }
  }
}

void TuyaBLEDevice::dump_config() {
  ESP_LOGCONFIG(TAG, "Tuya BLE Device:");
  ESP_LOGCONFIG(TAG, "  UUID: %s", this->uuid_.c_str());
  ESP_LOGCONFIG(TAG, "  Device ID: %s", this->device_id_.c_str());
  ESP_LOGCONFIG(TAG, "  Local Key: %s...", this->local_key_str_.substr(0, 4).c_str());
}

void TuyaBLEDevice::derive_keys_() {
  // local_key = first 6 chars of the provided key
  size_t key_len = std::min((size_t) 6, this->local_key_str_.size());
  memcpy(this->local_key_, this->local_key_str_.c_str(), key_len);

  // login_key = MD5(local_key)
  this->compute_md5_(this->local_key_, 6, this->login_key_);

  ESP_LOGD(TAG, "Keys derived from local_key");
}

void TuyaBLEDevice::compute_md5_(const uint8_t *input, size_t len, uint8_t *output) {
  mbedtls_md5_context ctx;
  mbedtls_md5_init(&ctx);
  mbedtls_md5_starts(&ctx);
  mbedtls_md5_update(&ctx, input, len);
  mbedtls_md5_finish(&ctx, output);
  mbedtls_md5_free(&ctx);
}

void TuyaBLEDevice::get_key_for_security_flag_(uint8_t flag, uint8_t *key_out) {
  switch (flag) {
    case 1:
      memcpy(key_out, this->auth_key_, 16);
      break;
    case 4:
      memcpy(key_out, this->login_key_, 16);
      break;
    case 5:
      memcpy(key_out, this->session_key_, 16);
      break;
    default:
      ESP_LOGW(TAG, "Unknown security flag: %d", flag);
      memcpy(key_out, this->login_key_, 16);
      break;
  }
}

void TuyaBLEDevice::aes_cbc_encrypt_(const uint8_t *key, const uint8_t *iv,
                                      const uint8_t *input, size_t input_len,
                                      uint8_t *output) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_enc(&ctx, key, 128);
  // mbedtls modifies iv in-place, so copy it
  uint8_t iv_copy[16];
  memcpy(iv_copy, iv, 16);
  mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, input_len, iv_copy, input, output);
  mbedtls_aes_free(&ctx);
}

void TuyaBLEDevice::aes_cbc_decrypt_(const uint8_t *key, const uint8_t *iv,
                                      const uint8_t *input, size_t input_len,
                                      uint8_t *output) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_dec(&ctx, key, 128);
  uint8_t iv_copy[16];
  memcpy(iv_copy, iv, 16);
  mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, input_len, iv_copy, input, output);
  mbedtls_aes_free(&ctx);
}

uint16_t TuyaBLEDevice::calc_crc16_(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i] & 0xFF;
    for (int j = 0; j < 8; j++) {
      uint16_t tmp = crc & 1;
      crc >>= 1;
      if (tmp != 0)
        crc ^= 0xA001;
    }
  }
  return crc;
}

std::vector<uint8_t> TuyaBLEDevice::pack_varint_(uint32_t value) {
  std::vector<uint8_t> result;
  do {
    uint8_t curr = value & 0x7F;
    value >>= 7;
    if (value != 0)
      curr |= 0x80;
    result.push_back(curr);
  } while (value != 0);
  return result;
}

bool TuyaBLEDevice::unpack_varint_(const uint8_t *data, size_t data_len,
                                    size_t &pos, uint32_t &result) {
  result = 0;
  uint32_t offset = 0;
  while (offset < 5) {
    if (pos >= data_len)
      return false;
    uint8_t curr = data[pos];
    result |= (uint32_t)(curr & 0x7F) << (offset * 7);
    pos++;
    offset++;
    if ((curr & 0x80) == 0)
      return true;
  }
  return false;  // Too many bytes
}

std::vector<std::vector<uint8_t>> TuyaBLEDevice::build_packets_(
    uint32_t seq_num, uint16_t code, const uint8_t *data, size_t data_len,
    uint32_t response_to) {
  // Determine key and security flag
  // Only DEVICE_INFO uses login_key (security_flag 0x04)
  // Everything else (including PAIR) uses session_key (0x05) after derivation
  uint8_t key[16];
  uint8_t security_flag;
  if (code == FUN_SENDER_DEVICE_INFO || !this->device_info_received_) {
    memcpy(key, this->login_key_, 16);
    security_flag = 0x04;
  } else {
    memcpy(key, this->session_key_, 16);
    security_flag = 0x05;
  }

  // Build command structure:
  // seq_num(4B) | response_to(4B) | code(2B) | data_len(2B) | data | CRC16(2B) | padding
  size_t raw_len = 4 + 4 + 2 + 2 + data_len + 2;
  // Pad to 16-byte boundary
  size_t padded_len = ((raw_len + 15) / 16) * 16;

  std::vector<uint8_t> raw(padded_len, 0);
  // seq_num (big-endian)
  raw[0] = (seq_num >> 24) & 0xFF;
  raw[1] = (seq_num >> 16) & 0xFF;
  raw[2] = (seq_num >> 8) & 0xFF;
  raw[3] = seq_num & 0xFF;
  // response_to
  raw[4] = (response_to >> 24) & 0xFF;
  raw[5] = (response_to >> 16) & 0xFF;
  raw[6] = (response_to >> 8) & 0xFF;
  raw[7] = response_to & 0xFF;
  // code
  raw[8] = (code >> 8) & 0xFF;
  raw[9] = code & 0xFF;
  // data_len
  raw[10] = (data_len >> 8) & 0xFF;
  raw[11] = data_len & 0xFF;
  // data
  if (data_len > 0)
    memcpy(&raw[12], data, data_len);
  // CRC16 over everything before CRC
  uint16_t crc = calc_crc16_(raw.data(), 12 + data_len);
  raw[12 + data_len] = (crc >> 8) & 0xFF;
  raw[12 + data_len + 1] = crc & 0xFF;

  // Generate random IV
  uint8_t iv[16];
#ifdef USE_ESP32
  esp_fill_random(iv, 16);
#else
  for (int i = 0; i < 16; i++)
    iv[i] = random(256);
#endif

  // Encrypt
  std::vector<uint8_t> encrypted(padded_len);
  this->aes_cbc_encrypt_(key, iv, raw.data(), padded_len, encrypted.data());

  // Build full encrypted payload: security_flag(1B) + iv(16B) + encrypted
  std::vector<uint8_t> payload;
  payload.push_back(security_flag);
  payload.insert(payload.end(), iv, iv + 16);
  payload.insert(payload.end(), encrypted.begin(), encrypted.end());

  // Fragment into MTU-sized BLE packets
  std::vector<std::vector<uint8_t>> packets;
  uint32_t packet_num = 0;
  size_t pos = 0;
  size_t total_len = payload.size();

  while (pos < total_len) {
    std::vector<uint8_t> packet;
    auto pn_bytes = pack_varint_(packet_num);
    packet.insert(packet.end(), pn_bytes.begin(), pn_bytes.end());

    if (packet_num == 0) {
      auto len_bytes = pack_varint_((uint32_t) total_len);
      packet.insert(packet.end(), len_bytes.begin(), len_bytes.end());
      packet.push_back(this->protocol_version_ << 4);
    }

    size_t remaining_space = GATT_MTU - packet.size();
    size_t chunk = std::min(remaining_space, total_len - pos);
    packet.insert(packet.end(), payload.begin() + pos, payload.begin() + pos + chunk);

    packets.push_back(std::move(packet));
    pos += chunk;
    packet_num++;
  }

  return packets;
}

void TuyaBLEDevice::send_command_(uint16_t code, const std::vector<uint8_t> &data) {
  uint32_t seq = this->current_seq_num_++;
  auto packets = this->build_packets_(seq, code, data.data(), data.size());
  this->write_packets_(packets);
}

void TuyaBLEDevice::send_response_(uint16_t code, const std::vector<uint8_t> &data,
                                    uint32_t response_to) {
  uint32_t seq = this->current_seq_num_++;
  auto packets = this->build_packets_(seq, code, data.data(), data.size(), response_to);
  this->write_packets_(packets);
}

void TuyaBLEDevice::write_packets_(const std::vector<std::vector<uint8_t>> &packets) {
  for (const auto &packet : packets) {
    this->pending_packets_.push_back(packet);
  }
}

void TuyaBLEDevice::gattc_event_handler(esp_gattc_cb_event_t event,
                                          esp_gatt_if_t gattc_if,
                                          esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status == ESP_GATT_OK) {
        ESP_LOGI(TAG, "BLE connected, starting service discovery...");
        this->ble_connected_ = true;
        this->set_paired_(false);
        this->device_info_received_ = false;
        this->pair_request_sent_ = false;
        this->current_seq_num_ = 1;
        this->pairing_start_time_ = millis();
        this->clean_input_();
      } else {
        ESP_LOGW(TAG, "BLE connect failed, status=%d - will retry automatically", param->open.status);
        this->ble_connected_ = false;
      }
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      bool was_paired = this->is_paired_;
      ESP_LOGW(TAG, "BLE disconnected (was_paired=%s) - ble_client will attempt reconnection",
               was_paired ? "true" : "false");
      this->ble_connected_ = false;
      this->set_paired_(false);
      this->device_info_received_ = false;
      this->pair_request_sent_ = false;
      this->notify_handle_ = 0;
      this->write_handle_ = 0;
      this->pairing_start_time_ = 0;
      this->pending_packets_.clear();
      this->clean_input_();
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      ESP_LOGI(TAG, "Service discovery complete, looking for Tuya BLE characteristics...");

      // Try all known Tuya BLE service UUIDs to find the characteristics.
      // Different Tuya devices expose the same characteristics under different service UUIDs.
      uint16_t found_notify_handle = 0;
      uint16_t found_write_handle = 0;

      for (size_t i = 0; i < NUM_SERVICE_UUIDS; i++) {
        auto *notify_chr = this->parent()->get_characteristic(SERVICE_UUIDS[i], CHAR_NOTIFY);
        if (notify_chr != nullptr) {
          auto *write_chr = this->parent()->get_characteristic(SERVICE_UUIDS[i], CHAR_WRITE);
          if (write_chr != nullptr) {
            ESP_LOGI(TAG, "Found Tuya characteristics under service UUID index %u", i);
            found_notify_handle = notify_chr->handle;
            found_write_handle = write_chr->handle;
            break;
          }
        }
      }

      if (found_notify_handle == 0 || found_write_handle == 0) {
        ESP_LOGE(TAG, "Tuya BLE characteristics not found under any known service UUID!");
        ESP_LOGE(TAG, "  Tried: 00001910-..., 0000fd50-..., 0000a201-...");
        ESP_LOGE(TAG, "  Notify char: 00002b10-..., Write char: 00002b11-...");
        ESP_LOGE(TAG, "  Is this a Tuya BLE device? Check the MAC address.");
        break;
      }

      this->notify_handle_ = found_notify_handle;
      this->write_handle_ = found_write_handle;
      ESP_LOGD(TAG, "Notify handle=0x%04X, Write handle=0x%04X",
               this->notify_handle_, this->write_handle_);

      // Register for notifications
      ESP_LOGI(TAG, "Registering for BLE notifications...");
      auto status = esp_ble_gattc_register_for_notify(
          gattc_if, this->parent()->get_remote_bda(), this->notify_handle_);
      if (status != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register for notifications: %d", status);
      }
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      if (param->reg_for_notify.status == ESP_GATT_OK) {
        ESP_LOGI(TAG, "Notification registration successful, beginning pairing handshake...");
        this->start_pairing_();
      } else {
        ESP_LOGE(TAG, "Notification registration failed: %d", param->reg_for_notify.status);
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      if (param->notify.handle == this->notify_handle_) {
        this->handle_notification_(param->notify.value, param->notify.value_len);
      }
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
      if (param->write.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "Write failed, status=%d", param->write.status);
      }
      break;
    }
    default:
      break;
  }
}

void TuyaBLEDevice::start_pairing_() {
  // Step 1: Send DEVICE_INFO request (empty payload)
  ESP_LOGI(TAG, "Step 1/3: Sending DEVICE_INFO request to device...");
  std::vector<uint8_t> empty;
  this->send_command_(FUN_SENDER_DEVICE_INFO, empty);
}

std::vector<uint8_t> TuyaBLEDevice::build_pairing_request_() {
  // Pairing request: uuid(variable) + local_key(6B) + device_id(variable) padded to 44 bytes
  std::vector<uint8_t> result;
  result.reserve(44);

  // UUID
  for (char c : this->uuid_)
    result.push_back((uint8_t) c);
  // Local key (first 6 chars)
  for (int i = 0; i < 6; i++)
    result.push_back(this->local_key_[i]);
  // Device ID
  for (char c : this->device_id_)
    result.push_back((uint8_t) c);
  // Pad to 44 bytes
  while (result.size() < 44)
    result.push_back(0);

  return result;
}

void TuyaBLEDevice::handle_notification_(const uint8_t *data, size_t len) {
  if (len == 0)
    return;

  ESP_LOGV(TAG, "Notification received, %d bytes", len);

  size_t pos = 0;
  uint32_t packet_num;
  if (!unpack_varint_(data, len, pos, packet_num)) {
    ESP_LOGW(TAG, "Failed to unpack packet number");
    this->clean_input_();
    return;
  }

  if (packet_num < this->input_expected_packet_num_) {
    ESP_LOGW(TAG, "Unexpected packet number %u, expected %u", packet_num,
             this->input_expected_packet_num_);
    this->clean_input_();
  }

  if (packet_num == this->input_expected_packet_num_) {
    if (packet_num == 0) {
      this->input_buffer_.clear();
      if (!unpack_varint_(data, len, pos, this->input_expected_length_)) {
        ESP_LOGW(TAG, "Failed to unpack total length");
        this->clean_input_();
        return;
      }
      pos++;  // Skip protocol version nibble byte
    }
    this->input_buffer_.insert(this->input_buffer_.end(), data + pos, data + len);
    this->input_expected_packet_num_++;
  } else {
    ESP_LOGW(TAG, "Missing packet %u, received %u", this->input_expected_packet_num_, packet_num);
    this->clean_input_();
    return;
  }

  if (this->input_buffer_.size() > this->input_expected_length_) {
    ESP_LOGW(TAG, "Buffer overflow: %u > %u", this->input_buffer_.size(),
             this->input_expected_length_);
    this->clean_input_();
    return;
  }

  if (this->input_buffer_.size() == this->input_expected_length_) {
    this->parse_input_();
  }
}

void TuyaBLEDevice::clean_input_() {
  this->input_buffer_.clear();
  this->input_expected_packet_num_ = 0;
  this->input_expected_length_ = 0;
}

void TuyaBLEDevice::parse_input_() {
  if (this->input_buffer_.size() < 17) {
    ESP_LOGW(TAG, "Input too short for decryption");
    this->clean_input_();
    return;
  }

  uint8_t security_flag = this->input_buffer_[0];
  const uint8_t *iv = &this->input_buffer_[1];
  const uint8_t *encrypted = &this->input_buffer_[17];
  size_t encrypted_len = this->input_buffer_.size() - 17;

  // Ensure encrypted length is multiple of 16
  if (encrypted_len % 16 != 0) {
    ESP_LOGW(TAG, "Encrypted data length not aligned: %u", encrypted_len);
    this->clean_input_();
    return;
  }

  uint8_t key[16];
  this->get_key_for_security_flag_(security_flag, key);

  std::vector<uint8_t> raw(encrypted_len);
  this->aes_cbc_decrypt_(key, iv, encrypted, encrypted_len, raw.data());

  this->clean_input_();

  if (raw.size() < 12) {
    ESP_LOGW(TAG, "Decrypted data too short");
    return;
  }

  // Parse header
  uint32_t seq_num = ((uint32_t) raw[0] << 24) | ((uint32_t) raw[1] << 16) |
                     ((uint32_t) raw[2] << 8) | raw[3];
  uint32_t response_to = ((uint32_t) raw[4] << 24) | ((uint32_t) raw[5] << 16) |
                          ((uint32_t) raw[6] << 8) | raw[7];
  uint16_t code = ((uint16_t) raw[8] << 8) | raw[9];
  uint16_t data_len = ((uint16_t) raw[10] << 8) | raw[11];

  size_t data_end_pos = 12 + data_len;
  if (data_end_pos > raw.size()) {
    ESP_LOGW(TAG, "Data length exceeds decrypted buffer");
    return;
  }

  // Validate CRC if there's room
  if (raw.size() > data_end_pos + 1) {
    uint16_t calc_crc = calc_crc16_(raw.data(), data_end_pos);
    uint16_t data_crc = ((uint16_t) raw[data_end_pos] << 8) | raw[data_end_pos + 1];
    if (calc_crc != data_crc) {
      ESP_LOGW(TAG, "CRC mismatch: calc=0x%04X data=0x%04X", calc_crc, data_crc);
      return;
    }
  }

  ESP_LOGD(TAG, "Received cmd=0x%04X seq=%u resp_to=%u len=%u", code, seq_num, response_to,
           data_len);

  this->handle_command_or_response_(seq_num, response_to, code, &raw[12], data_len);
}

void TuyaBLEDevice::handle_command_or_response_(uint32_t seq_num, uint32_t response_to,
                                                  uint16_t code, const uint8_t *data,
                                                  size_t len) {
  switch (code) {
    case FUN_SENDER_DEVICE_INFO: {
      if (len < 46) {
        ESP_LOGE(TAG, "DEVICE_INFO response too short (%u bytes, need 46) - is local_key correct?", len);
        return;
      }

      ESP_LOGI(TAG, "Step 2/3: DEVICE_INFO received - device v%d.%d, protocol v%d.%d, hardware v%d.%d",
               data[0], data[1], data[2], data[3], data[12], data[13]);

      this->protocol_version_ = data[2];

      // Derive session key: MD5(local_key + srand)
      uint8_t key_material[12];
      memcpy(key_material, this->local_key_, 6);
      memcpy(key_material + 6, &data[6], 6);  // srand
      this->compute_md5_(key_material, 12, this->session_key_);
      ESP_LOGD(TAG, "Session key derived from local_key + device srand");

      // Store auth key
      memcpy(this->auth_key_, &data[14], 32);

      this->device_info_received_ = true;

      // Now send pairing request
      if (!this->pair_request_sent_) {
        ESP_LOGI(TAG, "Step 3/3: Sending PAIR request (uuid=%s, device_id=%s)...",
                 this->uuid_.c_str(), this->device_id_.c_str());
        auto pair_data = this->build_pairing_request_();
        this->send_command_(FUN_SENDER_PAIR, pair_data);
        this->pair_request_sent_ = true;
      }
      break;
    }
    case FUN_SENDER_PAIR: {
      if (len < 1) {
        ESP_LOGE(TAG, "PAIR response too short - device rejected pairing");
        return;
      }
      uint8_t result = data[0];
      if (result == 2) {
        ESP_LOGI(TAG, "Device reports already paired, treating as success");
        result = 0;
      }
      bool paired = (result == 0);
      this->set_paired_(paired);
      if (this->is_paired_) {
        this->pairing_start_time_ = 0;  // Clear timeout
        this->pairing_fail_count_ = 0;
        ESP_LOGI(TAG, "Successfully paired with device! Connection is now fully established.");
        // Request initial device status to populate all datapoints
        ESP_LOGI(TAG, "Requesting initial device status...");
        std::vector<uint8_t> empty;
        this->send_command_(FUN_SENDER_DEVICE_STATUS, empty);
      } else {
        ESP_LOGE(TAG, "Pairing FAILED with result code: %d", data[0]);
        ESP_LOGE(TAG, "  Check that uuid, device_id, and local_key are correct");
        ESP_LOGE(TAG, "  uuid=%s, device_id=%s", this->uuid_.c_str(), this->device_id_.c_str());
      }
      break;
    }
    case FUN_RECEIVE_DP: {
      this->parse_datapoints_v3_(data, len, 0);
      std::vector<uint8_t> empty;
      this->send_response_(code, empty, seq_num);
      break;
    }
    case FUN_RECEIVE_SIGN_DP: {
      if (len < 3) return;
      // uint16_t dp_seq_num = ((uint16_t)data[0] << 8) | data[1];
      // uint8_t flags = data[2];
      this->parse_datapoints_v3_(data, len, 3);
      // Send response with dp_seq_num, flags, result
      std::vector<uint8_t> resp(4);
      resp[0] = data[0];
      resp[1] = data[1];
      resp[2] = data[2];
      resp[3] = 0;  // result = success
      this->send_response_(code, resp, seq_num);
      break;
    }
    case FUN_RECEIVE_TIME_DP: {
      size_t pos = this->parse_timestamp_(data, len, 0);
      if (pos > 0)
        this->parse_datapoints_v3_(data, len, pos);
      std::vector<uint8_t> empty;
      this->send_response_(code, empty, seq_num);
      break;
    }
    case FUN_RECEIVE_SIGN_TIME_DP: {
      if (len < 3) return;
      size_t pos = this->parse_timestamp_(data, len, 3);
      if (pos > 0)
        this->parse_datapoints_v3_(data, len, pos);
      std::vector<uint8_t> resp(4);
      resp[0] = data[0];
      resp[1] = data[1];
      resp[2] = data[2];
      resp[3] = 0;
      this->send_response_(code, resp, seq_num);
      break;
    }
    case FUN_RECEIVE_TIME1_REQ: {
      this->send_time1_response_(seq_num);
      break;
    }
    case FUN_RECEIVE_TIME2_REQ: {
      this->send_time2_response_(seq_num);
      break;
    }
    default:
      ESP_LOGD(TAG, "Unhandled command: 0x%04X", code);
      break;
  }
}

void TuyaBLEDevice::parse_datapoints_v3_(const uint8_t *data, size_t len, size_t start_pos) {
  size_t pos = start_pos;
  while (len - pos >= 4) {
    uint8_t dp_id = data[pos++];
    uint8_t dp_type_raw = data[pos++];
    uint8_t dp_len = data[pos++];

    if (dp_type_raw > DT_BITMAP) {
      ESP_LOGW(TAG, "Invalid DP type: %d", dp_type_raw);
      return;
    }
    if (pos + dp_len > len) {
      ESP_LOGW(TAG, "DP data exceeds buffer");
      return;
    }

    TuyaBLEDataPointType dp_type = static_cast<TuyaBLEDataPointType>(dp_type_raw);
    TuyaBLEDatapoint dp;
    dp.id = dp_id;
    dp.type = dp_type;

    switch (dp_type) {
      case DT_BOOL:
        dp.value_bool = (dp_len > 0 && data[pos] != 0);
        dp.value_int = dp.value_bool ? 1 : 0;
        break;
      case DT_VALUE: {
        // Build unsigned value from big-endian bytes, then sign-extend
        uint32_t uval = 0;
        for (size_t i = 0; i < dp_len && i < 4; i++) {
          uval = (uval << 8) | data[pos + i];
        }
        // Sign extend based on actual byte width
        if (dp_len > 0 && dp_len < 4 && (data[pos] & 0x80)) {
          // Fill upper bits with 1s for sign extension
          uint32_t mask = 0xFFFFFFFF << (dp_len * 8);
          uval |= mask;
        }
        dp.value_int = static_cast<int32_t>(uval);
        dp.value_bool = (dp.value_int != 0);
        break;
      }
      case DT_ENUM: {
        int32_t val = 0;
        for (size_t i = 0; i < dp_len && i < 4; i++) {
          val = (val << 8) | data[pos + i];
        }
        dp.value_int = val;
        dp.value_bool = (val != 0);
        break;
      }
      case DT_STRING:
        dp.value_string = std::string((const char *) &data[pos], dp_len);
        break;
      case DT_RAW:
      case DT_BITMAP:
        dp.value_raw.assign(&data[pos], &data[pos + dp_len]);
        break;
    }

    ESP_LOGD(TAG, "DP update: id=%u type=%u value_int=%d", dp_id, dp_type_raw, dp.value_int);

    // Notify listeners
    auto range = this->dp_listeners_.equal_range(dp_id);
    for (auto it = range.first; it != range.second; ++it) {
      it->second(dp);
    }

    pos += dp_len;
  }
}

size_t TuyaBLEDevice::parse_timestamp_(const uint8_t *data, size_t len, size_t start_pos) {
  if (start_pos >= len)
    return 0;
  uint8_t time_type = data[start_pos];
  size_t pos = start_pos + 1;
  switch (time_type) {
    case 0:
      // 13-byte ASCII millisecond timestamp
      pos += 13;
      break;
    case 1:
      // 4-byte big-endian timestamp
      pos += 4;
      break;
    default:
      ESP_LOGW(TAG, "Unknown timestamp type: %d", time_type);
      return 0;
  }
  if (pos > len)
    return 0;
  return pos;
}

static int16_t get_tz_offset_() {
  // Compute UTC offset by comparing localtime and gmtime
  time_t now;
  time(&now);
  struct tm local_tm, gm_tm;
  localtime_r(&now, &local_tm);
  gmtime_r(&now, &gm_tm);
  // Difference in seconds
  int32_t diff = (local_tm.tm_hour - gm_tm.tm_hour) * 3600 +
                 (local_tm.tm_min - gm_tm.tm_min) * 60 +
                 (local_tm.tm_sec - gm_tm.tm_sec);
  // Handle day boundary
  int day_diff = local_tm.tm_mday - gm_tm.tm_mday;
  if (day_diff > 1) day_diff = -1;   // Month boundary: local is 1st, gm is 28-31st
  if (day_diff < -1) day_diff = 1;   // Month boundary: local is 28-31st, gm is 1st
  diff += day_diff * 86400;
  return (int16_t)(diff / 36);
}

void TuyaBLEDevice::send_time1_response_(uint32_t seq_num) {
  // Millisecond timestamp as string + timezone
  time_t now;
  time(&now);
  uint64_t ms = (uint64_t) now * 1000;
  char buf[14];
  snprintf(buf, sizeof(buf), "%013llu", (unsigned long long) ms);

  int16_t tz_offset = get_tz_offset_();

  std::vector<uint8_t> resp;
  for (int i = 0; i < 13; i++)
    resp.push_back((uint8_t) buf[i]);
  resp.push_back((tz_offset >> 8) & 0xFF);
  resp.push_back(tz_offset & 0xFF);

  this->send_response_(FUN_RECEIVE_TIME1_REQ, resp, seq_num);
}

void TuyaBLEDevice::send_time2_response_(uint32_t seq_num) {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  int16_t tz_offset = get_tz_offset_();

  std::vector<uint8_t> resp(9);
  resp[0] = timeinfo.tm_year % 100;  // Year (2-digit)
  resp[1] = timeinfo.tm_mon + 1;     // Month (1-12)
  resp[2] = timeinfo.tm_mday;        // Day
  resp[3] = timeinfo.tm_hour;        // Hour
  resp[4] = timeinfo.tm_min;         // Minute
  resp[5] = timeinfo.tm_sec;         // Second
  // Python tm_wday: Monday=0..Sunday=6; C tm_wday: Sunday=0..Saturday=6
  // Convert C convention to Python convention for Tuya protocol
  resp[6] = (timeinfo.tm_wday + 6) % 7;  // Day of week (0=Monday)
  resp[7] = (tz_offset >> 8) & 0xFF;
  resp[8] = tz_offset & 0xFF;

  this->send_response_(FUN_RECEIVE_TIME2_REQ, resp, seq_num);
}

void TuyaBLEDevice::register_listener(uint8_t dp_id, const DPListener &listener) {
  this->dp_listeners_.insert({dp_id, listener});
}

void TuyaBLEDevice::register_connection_listener(const ConnectionStateListener &listener) {
  this->connection_listeners_.push_back(listener);
}

void TuyaBLEDevice::set_paired_(bool paired) {
  bool changed = (this->is_paired_ != paired);
  this->is_paired_ = paired;
  if (changed) {
    ESP_LOGI(TAG, "Connection state changed: paired=%s", paired ? "true" : "false");
    this->fire_connection_listeners_(paired);
  }
}

void TuyaBLEDevice::fire_connection_listeners_(bool connected) {
  for (const auto &listener : this->connection_listeners_) {
    listener(connected);
  }
}

std::vector<uint8_t> TuyaBLEDevice::encode_dp_value_(const TuyaBLEDatapoint &dp) {
  std::vector<uint8_t> result;
  switch (dp.type) {
    case DT_BOOL:
      result.push_back(dp.value_bool ? 1 : 0);
      break;
    case DT_VALUE:
      result.push_back((dp.value_int >> 24) & 0xFF);
      result.push_back((dp.value_int >> 16) & 0xFF);
      result.push_back((dp.value_int >> 8) & 0xFF);
      result.push_back(dp.value_int & 0xFF);
      break;
    case DT_ENUM:
      if (dp.value_int > 0xFFFF) {
        result.push_back((dp.value_int >> 24) & 0xFF);
        result.push_back((dp.value_int >> 16) & 0xFF);
        result.push_back((dp.value_int >> 8) & 0xFF);
        result.push_back(dp.value_int & 0xFF);
      } else if (dp.value_int > 0xFF) {
        result.push_back((dp.value_int >> 8) & 0xFF);
        result.push_back(dp.value_int & 0xFF);
      } else {
        result.push_back(dp.value_int & 0xFF);
      }
      break;
    case DT_STRING:
      result.assign(dp.value_string.begin(), dp.value_string.end());
      break;
    case DT_RAW:
    case DT_BITMAP:
      result = dp.value_raw;
      break;
  }
  return result;
}

void TuyaBLEDevice::send_datapoint(const TuyaBLEDatapoint &dp) {
  if (!this->is_paired_) {
    ESP_LOGW(TAG, "Cannot send DP %u: device not paired (ble_connected=%s, device_info=%s, pair_sent=%s)",
             dp.id,
             this->ble_connected_ ? "true" : "false",
             this->device_info_received_ ? "true" : "false",
             this->pair_request_sent_ ? "true" : "false");
    return;
  }

  auto value = encode_dp_value_(dp);

  // Build DP payload: dp_id(1B) + type(1B) + length(1B) + value
  std::vector<uint8_t> data;
  data.push_back(dp.id);
  data.push_back(static_cast<uint8_t>(dp.type));
  data.push_back(value.size());
  data.insert(data.end(), value.begin(), value.end());

  ESP_LOGD(TAG, "Sending DP: id=%u type=%u value_int=%d", dp.id, dp.type, dp.value_int);
  this->send_command_(FUN_SENDER_DPS, data);
}

}  // namespace tuya_ble_device
}  // namespace esphome
