#include "tuya_ble_light.h"
#include "esphome/core/log.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace esphome {
namespace tuya_ble_light {

static const char *const TAG = "tuya_ble.light";

void TuyaBLELight::setup() {
  this->device_->register_listener(this->switch_dp_,
                                    [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                      this->on_dp_update_(dp);
                                    });
  if (this->brightness_dp_.has_value()) {
    this->device_->register_listener(this->brightness_dp_.value(),
                                      [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                        this->on_dp_update_(dp);
                                      });
  }
  if (this->color_temp_dp_.has_value()) {
    this->device_->register_listener(this->color_temp_dp_.value(),
                                      [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                        this->on_dp_update_(dp);
                                      });
  }
  if (this->color_dp_.has_value()) {
    this->device_->register_listener(this->color_dp_.value(),
                                      [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                        this->on_dp_update_(dp);
                                      });
  }
  if (this->work_mode_dp_.has_value()) {
    this->device_->register_listener(this->work_mode_dp_.value(),
                                      [this](const tuya_ble_device::TuyaBLEDatapoint &dp) {
                                        this->on_dp_update_(dp);
                                      });
  }
}

void TuyaBLELight::dump_config() {
  ESP_LOGCONFIG(TAG, "Tuya BLE Light:");
  ESP_LOGCONFIG(TAG, "  Switch DP: %u", this->switch_dp_);
  if (this->brightness_dp_.has_value())
    ESP_LOGCONFIG(TAG, "  Brightness DP: %u (max: %d)", this->brightness_dp_.value(), this->brightness_max_);
  if (this->color_temp_dp_.has_value())
    ESP_LOGCONFIG(TAG, "  Color Temp DP: %u (%d-%d mireds)", this->color_temp_dp_.value(),
                  this->color_temp_min_mireds_, this->color_temp_max_mireds_);
  if (this->color_dp_.has_value())
    ESP_LOGCONFIG(TAG, "  Color DP: %u", this->color_dp_.value());
  if (this->work_mode_dp_.has_value())
    ESP_LOGCONFIG(TAG, "  Work Mode DP: %u", this->work_mode_dp_.value());
}

light::LightTraits TuyaBLELight::get_traits() {
  auto traits = light::LightTraits();
  if (this->color_dp_.has_value() && this->color_temp_dp_.has_value()) {
    traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
  } else if (this->color_dp_.has_value()) {
    traits.set_supported_color_modes({light::ColorMode::RGB});
  } else if (this->brightness_dp_.has_value() && this->color_temp_dp_.has_value()) {
    traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  } else if (this->brightness_dp_.has_value()) {
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  } else {
    traits.set_supported_color_modes({light::ColorMode::ON_OFF});
  }
  if (this->color_temp_dp_.has_value()) {
    traits.set_min_mireds(this->color_temp_min_mireds_);
    traits.set_max_mireds(this->color_temp_max_mireds_);
  }
  return traits;
}

void TuyaBLELight::write_state(light::LightState *state) {
  this->state_ = state;

  float brightness;
  state->current_values_as_brightness(&brightness);

  bool is_on;
  state->current_values_as_binary(&is_on);

  // Send on/off
  {
    tuya_ble_device::TuyaBLEDatapoint dp;
    dp.id = this->switch_dp_;
    dp.type = tuya_ble_device::DT_BOOL;
    dp.value_bool = is_on;
    this->device_->send_datapoint(dp);
  }

  if (!is_on)
    return;

  auto color_mode = state->current_values.get_color_mode();

  // RGB mode
  if (color_mode == light::ColorMode::RGB && this->color_dp_.has_value()) {
    // Set work mode to "colour"
    if (this->work_mode_dp_.has_value()) {
      tuya_ble_device::TuyaBLEDatapoint dp;
      dp.id = this->work_mode_dp_.value();
      dp.type = tuya_ble_device::DT_STRING;
      dp.value_string = "colour";
      this->device_->send_datapoint(dp);
    }

    float red, green, blue;
    state->current_values_as_rgb(&red, &green, &blue);

    // Convert RGB to HSV
    float h, s, v;
    float max_c = std::max({red, green, blue});
    float min_c = std::min({red, green, blue});
    float delta = max_c - min_c;
    v = max_c;
    s = (max_c > 0) ? (delta / max_c) : 0;
    if (delta == 0) {
      h = 0;
    } else if (max_c == red) {
      h = 60.0f * fmod((green - blue) / delta, 6.0f);
    } else if (max_c == green) {
      h = 60.0f * ((blue - red) / delta + 2.0f);
    } else {
      h = 60.0f * ((red - green) / delta + 4.0f);
    }
    if (h < 0) h += 360.0f;

    // Encode as 12-char hex: HHHHSSSSVVVV (scaled to brightness_max_)
    int h_int = (int) h;
    int s_int = (int) (s * this->brightness_max_);
    int v_int = (int) (v * this->brightness_max_);

    char buf[16];
    snprintf(buf, sizeof(buf), "%04x%04x%04x", h_int, s_int, v_int);

    tuya_ble_device::TuyaBLEDatapoint dp;
    dp.id = this->color_dp_.value();
    dp.type = tuya_ble_device::DT_STRING;
    dp.value_string = std::string(buf);
    this->device_->send_datapoint(dp);
    return;
  }

  // Color temperature mode
  if (color_mode == light::ColorMode::COLOR_TEMPERATURE && this->color_temp_dp_.has_value()) {
    // Set work mode to "white"
    if (this->work_mode_dp_.has_value()) {
      tuya_ble_device::TuyaBLEDatapoint dp;
      dp.id = this->work_mode_dp_.value();
      dp.type = tuya_ble_device::DT_STRING;
      dp.value_string = "white";
      this->device_->send_datapoint(dp);
    }

    float color_temp = state->current_values.get_color_temperature();
    // Map mireds to device range (0 to brightness_max_)
    int raw = (int) ((color_temp - this->color_temp_min_mireds_) /
                      (float) (this->color_temp_max_mireds_ - this->color_temp_min_mireds_) *
                      this->brightness_max_);
    if (raw < 0) raw = 0;
    if (raw > this->brightness_max_) raw = this->brightness_max_;

    tuya_ble_device::TuyaBLEDatapoint dp;
    dp.id = this->color_temp_dp_.value();
    dp.type = tuya_ble_device::DT_VALUE;
    dp.value_int = raw;
    this->device_->send_datapoint(dp);
  }

  // Brightness (for white/color_temp modes)
  if (this->brightness_dp_.has_value()) {
    int raw = (int) (brightness * this->brightness_max_);
    if (raw < 1 && is_on) raw = 1;
    if (raw > this->brightness_max_) raw = this->brightness_max_;

    tuya_ble_device::TuyaBLEDatapoint dp;
    dp.id = this->brightness_dp_.value();
    dp.type = tuya_ble_device::DT_VALUE;
    dp.value_int = raw;
    this->device_->send_datapoint(dp);
  }
}

void TuyaBLELight::on_dp_update_(const tuya_ble_device::TuyaBLEDatapoint &dp) {
  if (dp.id == this->switch_dp_) {
    this->device_on_ = dp.value_bool;
  } else if (this->brightness_dp_.has_value() && dp.id == this->brightness_dp_.value()) {
    this->device_brightness_ = dp.value_int;
  } else if (this->color_temp_dp_.has_value() && dp.id == this->color_temp_dp_.value()) {
    this->device_color_temp_ = dp.value_int;
  } else if (this->color_dp_.has_value() && dp.id == this->color_dp_.value()) {
    this->parse_color_data_(dp.value_string);
  } else if (this->work_mode_dp_.has_value() && dp.id == this->work_mode_dp_.value()) {
    this->device_work_mode_ = dp.value_string;
  }

  if (this->state_ == nullptr)
    return;

  auto call = this->state_->make_call();
  call.set_state(this->device_on_);

  if (this->device_on_) {
    bool is_color = (this->device_work_mode_ == "colour" && this->color_dp_.has_value());

    if (is_color) {
      float r, g, b;
      // Convert HSV to RGB
      float h = this->device_hue_;
      float s = this->device_saturation_;
      float v = this->device_value_;
      int hi = (int) (h / 60.0f) % 6;
      float f = h / 60.0f - hi;
      float p = v * (1.0f - s);
      float q = v * (1.0f - f * s);
      float t = v * (1.0f - (1.0f - f) * s);
      switch (hi) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
      }
      call.set_color_mode(light::ColorMode::RGB);
      call.set_red(r);
      call.set_green(g);
      call.set_blue(b);
    } else {
      if (this->brightness_dp_.has_value()) {
        float bri = (float) this->device_brightness_ / this->brightness_max_;
        if (bri < 0) bri = 0;
        if (bri > 1) bri = 1;
        call.set_brightness(bri);
      }
      if (this->color_temp_dp_.has_value()) {
        float ct = this->color_temp_min_mireds_ +
                   (float) this->device_color_temp_ / this->brightness_max_ *
                       (this->color_temp_max_mireds_ - this->color_temp_min_mireds_);
        call.set_color_temperature(ct);
        call.set_color_mode(light::ColorMode::COLOR_TEMPERATURE);
      }
    }
  }

  call.perform();
}

void TuyaBLELight::parse_color_data_(const std::string &data) {
  if (data.length() >= 12) {
    // HSV format: HHHHSSSSVVVV
    char h_str[5] = {0}, s_str[5] = {0}, v_str[5] = {0};
    memcpy(h_str, data.c_str(), 4);
    memcpy(s_str, data.c_str() + 4, 4);
    memcpy(v_str, data.c_str() + 8, 4);
    int h = (int) strtol(h_str, nullptr, 16);
    int s = (int) strtol(s_str, nullptr, 16);
    int v = (int) strtol(v_str, nullptr, 16);
    this->device_hue_ = (float) h;
    this->device_saturation_ = (float) s / this->brightness_max_;
    this->device_value_ = (float) v / this->brightness_max_;
  }
}

}  // namespace tuya_ble_light
}  // namespace esphome
