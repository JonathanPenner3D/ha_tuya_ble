# Tuya BLE Datapoint (DP) Reference Guide

This guide lists known DP assignments for Tuya BLE devices, extracted from the
Home Assistant `tuya_ble_jp` integration. Use it to configure the ESPHome
component without needing to look up DPs on the Tuya IoT Platform.

> **Tip:** DP assignments can vary between product revisions. If a mapping below
> doesn't work for your device, check your actual DPs via the Tuya IoT Platform
> (iot.tuya.com) under Cloud > Development > Devices > Functions (DP).

---

## Blind / Curtain Controller (category `cl`)

Covers use **sensible defaults** (state=1, position_set=2, position=3), so you
typically don't need to specify DP numbers at all.

### Core DPs

| DP | Name | Type | Values |
|----|------|------|--------|
| 1 | State | enum | 0=Open, 1=Stop, 2=Close |
| 2 | Position Set | int | 0-100 (inverted: 0=open, 100=closed) |
| 3 | Position (current) | int | 0-100 (same inverted scale) |
| 4 | Opening Mode | int | Device-specific |
| 7 | Work State | enum | 0=Standby, 1=Success, 2=Learning |
| 13 | Battery | int | 0-100% |
| 101 | Tilt (venetian blinds) | int | 1-10 |
| 102 | Upper Limit | int | Device-specific |
| 105 | Motor Speed | int | 1-40 |
| 107 | Factory Reset | int | Device-specific |

### Known Products

| Product ID | Device |
|------------|--------|
| 4pbr8eig, vlwf3ud6, mnet9kgf | Blind Controller |
| kcy0x4pi | Curtain Controller |
| dy4dh1q0 | AOK AM24 Venetian Blind Motor (has tilt DP 101) |

### Minimal ESPHome Config

```yaml
cover:
  - platform: tuya_ble
    tuya_ble_device_id: my_device
    name: "Blinds"
    # Defaults: state_dp=1, position_set_dp=2, position_dp=3
    # tilt_dp: 101  # Only for venetian blinds

sensor:
  - platform: tuya_ble
    tuya_ble_device_id: my_device
    sensors:
      - dp: 13
        name: "Battery"
        unit_of_measurement: "%"
        device_class: battery

number:
  - platform: tuya_ble
    tuya_ble_device_id: my_device
    numbers:
      - dp: 105
        name: "Motor Speed"
        min_value: 1
        max_value: 40
```

---

## Fingerbot (category `szjqr`)

DP assignments differ between CubeTouch, Fingerbot Plus, and Fingerbot models.

### CubeTouch 1s / CubeTouch II

Products: `3yqdo5yt`, `xhf790if`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | Switch | bool | switch | On/Off |
| 2 | Mode | enum | select | push, switch, program |
| 3 | Hold Time | int | number | 0-10 seconds |
| 4 | Reverse Positions | bool | switch | |
| 5 | Up Position | int | number | 0-100% |
| 6 | Down Position | int | number | 0-100% |
| 7 | Battery Charging | enum | sensor | |
| 8 | Battery | int | sensor | 0-100% |

### Fingerbot Plus

Products: `blliqpsj`, `ndvkgsrm`, `yiihr7zh`, `neq16kgd`, `6jcvqwh0`, `riecov42`, `h8kdwywx`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 2 | Switch | bool | switch | On/Off |
| 8 | Mode | enum | select | push, switch, program |
| 9 | Down Position | int | number | 51-100% |
| 10 | Hold Time | int | number | 0-10 seconds |
| 11 | Reverse Positions | bool | switch | |
| 12 | Battery | int | sensor | 0-100% |
| 15 | Up Position | int | number | 0-50% |
| 17 | Manual Control | bool | switch | |
| 121 | Program Repeats / Idle Pos | int | number | Multi-field DP |

### Fingerbot (Standard)

Products: `ltak7e1p`, `y6kttvd6`, `yrnk7mnn`, `nvr2rocq`, `bnt7wajf`, `rvdceqjh`, `5xhbk964`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 2 | Switch | bool | switch | On/Off |
| 8 | Mode | enum | select | push, switch, program |
| 9 | Down Position | int | number | 51-100% |
| 10 | Hold Time | int | number | 0-10s (coeff 10, step 0.1) |
| 11 | Reverse Positions | bool | switch | |
| 12 | Battery | int | sensor | 0-100% |
| 15 | Up Position | int | number | 0-50% |

### Nedis SmartLife Finger Robot

Product: `yn4x5fa7`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | Switch | bool | switch | On/Off |
| 2 | Mode | enum | select | push, switch, program |
| 3 | Hold Time | int | number | 0.3-10s (coeff 10) |
| 4 | Up Position | int | number | 0-30% |
| 5 | Down Position | int | number | 0-30% |
| 6 | Reverse Positions | bool | switch | |

### Example Config (Fingerbot Plus)

```yaml
switch:
  - platform: tuya_ble
    tuya_ble_device_id: my_fingerbot
    switches:
      - dp: 2
        name: "Switch"

sensor:
  - platform: tuya_ble
    tuya_ble_device_id: my_fingerbot
    sensors:
      - dp: 12
        name: "Battery"
        unit_of_measurement: "%"
        device_class: battery

number:
  - platform: tuya_ble
    tuya_ble_device_id: my_fingerbot
    numbers:
      - dp: 10
        name: "Hold Time"
        min_value: 0
        max_value: 10
        step: 1
        unit_of_measurement: "s"
      - dp: 15
        name: "Up Position"
        min_value: 0
        max_value: 50
        step: 1
        unit_of_measurement: "%"
      - dp: 9
        name: "Down Position"
        min_value: 51
        max_value: 100
        step: 1
        unit_of_measurement: "%"
```

---

## Fingerbot Plus — alternate category (`kg`)

Products: `mknd4lci`, `riecov42`, `bs3ubslo`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | Switch | bool | switch | On/Off |
| 101 | Mode | enum | select | push, switch, program |
| 102 | Down Position | int | number | 51-100% |
| 103 | Hold Time | int | number | 0-10s |
| 104 | Reverse Positions | bool | switch | |
| 105 | Battery | int | sensor | 0-100% |
| 106 | Up Position | int | number | 0-50% |
| 107 | Manual Control | bool | switch | |
| 109 | Program Repeats / Idle Pos | int | number | Multi-field DP |

---

## Thermostatic Radiator Valve (category `wk`)

Products: `drlajpqc`, `nhj2j7su`, `zmachryv`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 8 | Window Check | bool | switch | |
| 10 | Antifreeze | bool | switch | |
| 27 | Temp Calibration | int | number | -6 to +6 °C |
| 40 | Child Lock | bool | switch | |
| 101 | HVAC Switch | bool | climate | On=Heat, Off=Off |
| 102 | Current Temperature | int | climate | Coefficient 10 (÷10 for °C) |
| 103 | Target Temperature | int | climate | Coeff 10, step 0.5, 5-30°C |
| 105 | Battery Alarm | bool | binary_sensor | |
| 106 | Away Mode | bool | climate | Preset: Away / None |
| 107 | Programming Mode | bool | switch | |
| 108 | Programming Switch | bool | switch | |
| 130 | Water Scale Proof | bool | switch | |

### Example Config

```yaml
sensor:
  - platform: tuya_ble
    tuya_ble_device_id: my_trv
    sensors:
      - dp: 102
        name: "Current Temperature"
        unit_of_measurement: "°C"
        device_class: temperature
        coefficient: 10.0
      - dp: 103
        name: "Target Temperature"
        unit_of_measurement: "°C"
        device_class: temperature
        coefficient: 10.0

switch:
  - platform: tuya_ble
    tuya_ble_device_id: my_trv
    switches:
      - dp: 101
        name: "Heating"
      - dp: 40
        name: "Child Lock"

binary_sensor:
  - platform: tuya_ble
    tuya_ble_device_id: my_trv
    binary_sensors:
      - dp: 105
        name: "Battery Low"
        device_class: battery

number:
  - platform: tuya_ble
    tuya_ble_device_id: my_trv
    numbers:
      - dp: 27
        name: "Temperature Calibration"
        min_value: -6
        max_value: 6
        step: 1
        unit_of_measurement: "°C"
```

---

## CO2 Detector (category `co2bj`)

Product: `59s19z5m`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | CO2 Alarm State | enum | sensor | |
| 2 | CO2 Value | int | sensor | ppm |
| 11 | Alarm Bitmap | bitmap | switch | bit 0=CO2 alarm, bit 1=battery alarm |
| 13 | CO2 Alarm Switch | bool | switch | |
| 15 | Battery | int | sensor | 0-100% |
| 17 | Brightness | int | number | 0-100% |
| 18 | Temperature | int | sensor | °C |
| 19 | Humidity | int | sensor | % |
| 26 | CO2 Alarm Level | int | number | 400-5000 ppm, step 100 |
| 101 | Temperature Unit | enum | select | Celsius, Fahrenheit |

### Example Config

```yaml
sensor:
  - platform: tuya_ble
    tuya_ble_device_id: my_co2
    sensors:
      - dp: 2
        name: "CO2"
        unit_of_measurement: "ppm"
        device_class: carbon_dioxide
      - dp: 15
        name: "Battery"
        unit_of_measurement: "%"
        device_class: battery
      - dp: 18
        name: "Temperature"
        unit_of_measurement: "°C"
        device_class: temperature
      - dp: 19
        name: "Humidity"
        unit_of_measurement: "%"
        device_class: humidity

number:
  - platform: tuya_ble
    tuya_ble_device_id: my_co2
    numbers:
      - dp: 17
        name: "Brightness"
        min_value: 0
        max_value: 100
        unit_of_measurement: "%"
      - dp: 26
        name: "CO2 Alarm Level"
        min_value: 400
        max_value: 5000
        step: 100
        unit_of_measurement: "ppm"
```

---

## Temperature / Humidity Sensor (category `wsdcg`)

### Soil Moisture Sensor

Product: `ojzlzzsw`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | Temperature | int | sensor | °C |
| 2 | Moisture | int | sensor | % |
| 3 | Battery State | enum | sensor | |
| 4 | Battery | int | sensor | 0-100% |
| 9 | Temperature Unit | enum | select | Celsius, Fahrenheit |
| 17 | Reporting Period | int | number | 1-120 minutes |
| 21 | Switch | bool | switch | |

### Bluetooth Temp/Humidity Sensor

Products: `iv7hudlj`, `jm6iasmb`, `vlzqwckk`, `tr0kabuq`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | Temperature | int | sensor | Coefficient 10 (÷10 for °C) |
| 2 | Humidity | int | sensor | % |
| 4 | Battery | int | sensor | 0-100% |
| 9 | Temperature Unit | enum | select | Celsius, Fahrenheit |

### Soil Thermo-Hygrometer

Product: `tv6peegl`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 101 | Temperature | int | sensor | °C |
| 102 | Moisture | int | sensor | % |

### Example Config (Temp/Humidity Sensor)

```yaml
sensor:
  - platform: tuya_ble
    tuya_ble_device_id: my_sensor
    sensors:
      - dp: 1
        name: "Temperature"
        unit_of_measurement: "°C"
        device_class: temperature
        coefficient: 10.0
      - dp: 2
        name: "Humidity"
        unit_of_measurement: "%"
        device_class: humidity
      - dp: 4
        name: "Battery"
        unit_of_measurement: "%"
        device_class: battery
```

---

## Plant / Soil Sensor (category `zwjcy`)

Products: `gvygg3m8` (SGS01), `jabotj1z` (SRB-PM01)

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 3 | Soil Moisture | int | sensor | % |
| 5 | Temperature | int | sensor | Coefficient 10 (÷10 for °C) |
| 14 | Battery State | enum | sensor | |
| 15 | Battery | int | sensor | 0-100% |

---

## Smart Lock (category `ms`)

Products: `ludzroix`, `isk2p555`, `gumrixyt`, `uamrw6h3`, `sidhzylo`, `okkyfgfs`, `k53ok3u9`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 8 | Battery | int | sensor | 0-100% |
| 21 | Alarm / Lock State | enum | sensor | |
| 31 | Beep Volume | enum | select | mute, low, normal, high |
| 40 | Door Status | enum | sensor | |
| 46 | Manual Lock | bool | switch | Some models only |
| 47 | Lock Motor State | bool | switch / binary_sensor | |

### ESPHome Lock Config

```yaml
lock:
  - platform: tuya_ble
    tuya_ble_device_id: my_lock
    name: "Front Door"
    lock_state_dp: 47      # Motor state (inverted: locked when false)
    lock_control_dp: 46    # Manual lock control

sensor:
  - platform: tuya_ble
    tuya_ble_device_id: my_lock
    sensors:
      - dp: 8
        name: "Lock Battery"
        unit_of_measurement: "%"
        device_class: battery
```

Additional DPs on `mqc2hevy` (YSG_T8_8G_htr):

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 12 | Unlock Fingerprint | int | sensor | |
| 13 | Unlock Password | int | sensor | |
| 14 | Unlock Dynamic | int | sensor | |
| 19 | Unlock BLE | int | sensor | |
| 28 | Language | enum | select | chinese, english, japanese, etc. |
| 62 | Unlock Phone Remote | int | sensor | |
| 68 | Special Function | enum | select | function1, function2 |

---

## Smart Cylinder Lock (category `jtmspro`)

Products: `xicdxood` (Raycube K7 Pro+), `rlyxv7pe` (A1 Pro Max), `oyqux5vv` (LA-01), `ajk32biq` (B16), `z7lj676i`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 8 | Battery | int | sensor | 0-100% |
| 12 | Unlock Fingerprint | int | sensor | |
| 13 | Unlock Password | int | sensor | |
| 15 | Unlock Card | int | sensor | |
| 21 | Alarm / Lock State | enum | sensor | |
| 28 | Language | enum | select | Chinese, English, Arabic, etc. |
| 31 | Beep Volume | enum | select | Mute, Low, Normal, High |

---

## Water Valve / Irrigation (category `sfkzq`)

### Basic Valve Controller

Products: `0axr5s0b`, `46zia2nz`, `1fcnd8xk`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | Water Valve | bool | switch | On/Off |
| 7 | Battery | int | sensor | 0-100% |
| 9 | Time Use | int | sensor | seconds |
| 10 | Weather Delay | enum | select | cancel, 24h, 48h, 72h |
| 11 | Countdown Duration | int | number | 1-86400 seconds |
| 12 | Work State | enum | select/sensor | auto, manual, idle |
| 15 | Use Time (one session) | int | sensor | seconds |

### Smart Water Timer (SOP10)

Products: `nxquc5lb`, `svhikeyq`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | Water Valve | bool | switch | On/Off |
| 10 | Weather Delay | enum | select | cancel, 24h-168h |
| 11 | Countdown | int | number | 60-86400 seconds |
| 13 | Smart Weather | enum | select | sunny, cloudy, rainy |
| 14 | Weather Switch | bool | switch | SOP10 only |

---

## Irrigation Computer (category `ggq`)

### Single Outlet

Product: `6pahkcau` (PARKSIDE PPB A1)

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 1 | Water Valve | bool | switch | On/Off |
| 5 | Countdown Duration | int | number | 1-1440 minutes |
| 6 | Time Left | int | sensor | minutes |
| 11 | Battery | int | sensor | 0-100% |

### Dual Outlet

Products: `hfgdqhho`, `qycalacn`, `fnlw6npo`, `jjqi2syk`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 103 | CH2 Countdown Duration | int | number | 1-1440 minutes |
| 104 | CH2 Valve | bool | switch | On/Off |
| 105 | CH1 Valve | bool | switch | On/Off |
| 106 | CH1 Countdown Duration | int | number | 1-1440 minutes |
| 110 | CH2 Use Time | int | sensor | seconds |
| 111 | CH1 Use Time | int | sensor | seconds |
| 11 | Battery | int | sensor | 0-100% |

---

## Smart Water Bottle (category `znhsb`)

Product: `cdlandip`

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 101 | Temperature | int | sensor | °C |
| 102 | Water Intake | int | sensor | mL |
| 103 | Recommended Intake | int | number | 0-5000 mL |
| 104 | Battery | int | sensor | 0-100% |
| 106 | Temperature Unit | enum | select | Celsius, Fahrenheit |
| 107 | Reminder Mode | enum | select | interval, alarm |

---

## PARKSIDE Smart Battery (category `dcb`)

Products: `z5ztlw3k` (4Ah), `ajrhf1aj` (8Ah)

| DP | Name | Type | Platform | Values / Notes |
|----|------|------|----------|----------------|
| 2 | Charge Current | int | sensor | mA |
| 3 | Charge Voltage | int | sensor | mV |
| 8 | Charge Times | int | sensor | count |
| 9 | Discharge Times | int | sensor | count |
| 10 | Peak Current Times | int | sensor | count |
| 11 | Temperature | int | sensor | °C |
| 12 | Upper Temp Switch | bool | switch | |
| 14 | Use Time | int | sensor | minutes |
| 15 | Runtime Total | int | sensor | minutes |
| 16 | Battery | int | sensor | 0-100% |
| 19 | Product Type | int | sensor | |
| 21 | Fault | int | sensor | |
| 22 | Security Switch | bool | switch | |
| 101 | Discharging Current | int | sensor | mA |
| 102 | Battery Status | enum | sensor | |
| 103 | Charge to Full Time | int | sensor | minutes |
| 104 | Discharge to Empty Time | int | sensor | seconds |
| 105 | Battery Work Mode | enum | select | Performance, Balanced, Eco, Expert |
| 107-114 | Fault Counters | int | sensor | Various fault event counts |
| 116 | Low Discharge Voltage | int | number | mV |
| 117 | Discharge Current Limit | int | number | A |
| 118 | Power Indicator Time | int | number | seconds |
| 150 | Tool Product Type | int | sensor | |
| 152 | Tool Rotation Speed | int | sensor | |
| 153 | Tool Torque | int | sensor | |
| 154 | Tool Runtime Total | int | sensor | minutes |
| 155 | Kick Back Switch | bool | switch | |
| 156 | Tool Fault | int | sensor | |
| 163 | Lamp Switch | bool | switch | |
| 164 | Lamp Brightness | int | number | 0-100% |
| 165 | Lamp Delay Time | int | number | seconds |
| 170 | CW/CCW Control | bool | switch | |
| 171 | CW/CCW Display | bool | binary_sensor | |
| 172 | Battery Temp Current | int | sensor | °C |
| 173 | Kick Back Adjust | int | number | |
| 174 | Pack Work Mode | enum | select | Performance, Balanced, Eco, Expert |
| 178 | Speed Percentage | int | number | % |
| 185 | Laser Switch | bool | switch | |
| 186 | Laser Pulse Switch | bool | switch | |

---

## RGB Strip / LED Light (category `dd`)

Known products: `nvfrtxlq` (LGB102 Magic Strip), `umzu0c2y` (Floor Lamp),
`6jxcdae1` (Sunset Lamp), `0qgrjxum` (RGB Strip Light)

Light DPs vary by device. The HA integration uses DPCode names rather than
fixed numeric IDs. Check your device's DPs on the Tuya IoT Platform and look
for these function codes:

| DPCode | Function | Typical DP | Type |
|--------|----------|------------|------|
| switch_led | On/Off | 20 | bool |
| work_mode | Mode (white/colour/scene) | 21 | enum/string |
| bright_value / bright_value_v2 | Brightness | 22 | int (0-255 or 0-1000) |
| temp_value / temp_value_v2 | Color temperature | 23 | int |
| colour_data / colour_data_v2 | HSV color (hex string) | 24 | string |

### ESPHome Light Config

```yaml
light:
  - platform: tuya_ble
    tuya_ble_device_id: my_light
    name: "LED Strip"
    switch_dp: 20
    brightness_dp: 22
    brightness_max: 1000       # 1000 for v2 devices, 255 for v1
    color_temp_dp: 23
    color_temp_min_mireds: 153 # ~6500K (cool white)
    color_temp_max_mireds: 500 # ~2000K (warm white)
    color_dp: 24               # HSV data as 12-char hex string
    work_mode_dp: 21           # "white" or "colour"
```

For a brightness-only light (no color/color temp):

```yaml
light:
  - platform: tuya_ble
    tuya_ble_device_id: my_dimmer
    name: "Dimmer"
    switch_dp: 20
    brightness_dp: 22
    brightness_max: 1000
```

---

## How to Find Your Device's DPs

If your device isn't listed above:

1. Log in to [Tuya IoT Platform](https://iot.tuya.com)
2. Go to **Cloud > Development > your project > Devices**
3. Click your device and open the **Functions (DP)** tab
4. Note the `dp_id` values and their descriptions
5. Map them to ESPHome platforms:
   - Boolean DPs → `switch` or `binary_sensor`
   - Numeric DPs → `sensor` (read-only) or `number` (writable)
   - Enum DPs → `sensor` (read-only) or `select` (writable)
   - String DPs → `text`
   - Cover DPs → `cover` (state, position, tilt)
   - Light DPs → `light` (switch, brightness, color temp, color)
   - Climate DPs → `climate` (switch, current temp, target temp)
   - Lock DPs → `lock` (motor state, manual lock)
