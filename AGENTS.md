# AGENTS.md — Project Guide for AI Assistants

This file helps AI coding agents (and human developers) understand the
ha_tuya_ble project quickly without scanning every file.

## Project Overview

This repo contains two independent but related systems for controlling Tuya
BLE (Bluetooth Low Energy) smart home devices locally, without cloud access:

1. **Home Assistant Integration** (`custom_components/tuya_ble_jp/`) — a Python
   integration that runs inside Home Assistant. It uses the host machine's
   Bluetooth adapter.

2. **ESPHome Components** (`esphome/components/`) — C++ firmware components
   that run on an ESP32 microcontroller. The ESP32 handles BLE communication
   directly.

Both implement the same Tuya BLE protocol (GATT service `0000fd50`, AES-CBC
encryption, CRC16 framing) but are completely separate codebases with no shared
source files.

---

## Repository Layout

```
ha_tuya_ble/
├── custom_components/tuya_ble_jp/   # Home Assistant integration (Python)
│   ├── __init__.py                  # HA setup, platform loading
│   ├── cloud.py                     # Tuya Cloud API for fetching credentials
│   ├── devices.py                   # Device coordinators, product database
│   ├── const.py                     # Constants, DPCode enum (~280 codes)
│   ├── base.py                      # IntegerTypeData, EnumTypeData models
│   ├── config_flow.py               # UI config flow
│   ├── tuya_ble/                    # BLE protocol library (Python)
│   │   ├── tuya_ble.py              # TuyaBLEDevice — protocol + crypto
│   │   ├── const.py                 # GATT UUIDs, command codes
│   │   └── manager.py               # Credential management interface
│   ├── sensor.py                    # } Entity platforms — each file has
│   ├── binary_sensor.py             # } a `mapping` dict that maps device
│   ├── switch.py                    # } categories and product IDs to
│   ├── cover.py                     # } specific DP assignments.
│   ├── light.py                     # }
│   ├── lock.py                      # }
│   ├── climate.py                   # }
│   ├── number.py                    # }
│   ├── select.py                    # }
│   └── text.py                      # }
│
├── esphome/                         # ESPHome components (C++)
│   ├── components/
│   │   ├── tuya_ble_device/         # Core device — BLE + crypto + protocol
│   │   │   ├── __init__.py          # ESPHome config schema (local_key, uuid, device_id)
│   │   │   ├── tuya_ble_device.h    # TuyaBLEDevice class, TuyaBLEDatapoint struct
│   │   │   └── tuya_ble_device.cpp  # Protocol impl (AES, CRC, GATT, pairing)
│   │   │
│   │   └── tuya_ble/                # Entity platforms (each is 3 files)
│   │       ├── __init__.py          # Shared constants (DP_TYPES)
│   │       ├── sensor/              # Numeric sensors (dp, coefficient)
│   │       ├── binary_sensor/       # Boolean sensors (dp)
│   │       ├── number/              # Writable numbers (dp, min, max, step)
│   │       ├── text/                # String entities (dp)
│   │       ├── cover/               # Blinds/curtains (state, position, tilt DPs)
│   │       ├── light/               # RGB/brightness/color temp lights
│   │       ├── climate/             # Thermostat (switch, current/target temp)
│   │       └── lock/                # Smart lock (state DP, control DP)
│   │
│   ├── example.yaml                 # Reference ESPHome config with all platforms
│   └── DP_REFERENCE.md              # DP lookup table by device category
│
├── README.md                        # User-facing install & usage docs
├── CHANGELOG.md                     # Release history
└── CONTIBUTING.md                   # How to add new device support
```

---

## Key Concepts

### Datapoints (DPs)

Tuya devices communicate via numbered **datapoints**. Each DP has:
- **ID** (uint8): e.g., 1, 2, 13, 101
- **Type**: `DT_BOOL`, `DT_VALUE` (int32), `DT_STRING`, `DT_ENUM`, `DT_RAW`, `DT_BITMAP`
- **Value**: the actual data

DP assignments are device-specific. For example, DP 1 might be "state" on a
blind controller but "switch" on a fingerbot. The file `esphome/DP_REFERENCE.md`
documents known mappings by device category.

### Device Categories

Tuya groups devices by category codes:
- `cl` — Blinds/curtains
- `szjqr` — Fingerbots
- `wk` — Thermostatic radiator valves
- `ms` — Smart locks
- `dd` — LED strip lights
- `wsdcg` — Temp/humidity sensors
- `co2bj` — CO2 detectors
- See `DP_REFERENCE.md` for the full list

### Two-System Architecture

| Aspect | HA Integration | ESPHome Component |
|--------|---------------|-------------------|
| Language | Python | C++ |
| Runs on | HA host | ESP32 chip |
| BLE via | Host Bluetooth adapter | ESP32 BLE radio |
| Config | Auto-discovery + Tuya Cloud | Manual YAML with DPs |
| Device DB | Product ID → DP mapping tables in each platform `.py` | User specifies DPs in YAML |

The HA integration has rich per-product mapping tables. The ESPHome component
is generic — users configure DP numbers directly, consulting the DP reference.

---

## How to Add a New ESPHome Entity Platform

Each ESPHome platform follows the same 3-file pattern:

### 1. `__init__.py` — Config Schema + Code Generation

```python
# Define config keys and validation
CONFIG_SCHEMA = platform.SCHEMA.extend({
    cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
    cv.Required("some_dp"): cv.uint8_t,
})

# Generate C++ code from config
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_DEVICE_ID])
    cg.add(var.set_tuya_ble_device(parent))
    cg.add(var.set_some_dp(config["some_dp"]))
```

### 2. `.h` — C++ Header

- Inherit from the ESPHome entity base class + `Component`
- Store a pointer to `TuyaBLEDevice` and DP ID(s)
- Declare `setup()`, `dump_config()`, and the platform's control method

### 3. `.cpp` — C++ Implementation

- In `setup()`: call `device_->register_listener(dp_id, callback)`
- In the listener callback: convert the `TuyaBLEDatapoint` value to the
  entity's state and call `publish_state()`
- In the control method: build a `TuyaBLEDatapoint` and call
  `device_->send_datapoint(dp)`

### TuyaBLEDevice API (the only interface between platforms and BLE)

```cpp
// Register a callback for when a DP value arrives from the device
void register_listener(uint8_t dp_id, const DPListener &listener);

// Send a DP value to the device
void send_datapoint(const TuyaBLEDatapoint &dp);

// Check if device is connected and authenticated
bool is_paired() const;
```

The `TuyaBLEDatapoint` struct:
```cpp
struct TuyaBLEDatapoint {
  uint8_t id;                         // DP number
  TuyaBLEDataPointType type;          // DT_BOOL, DT_VALUE, DT_STRING, etc.
  union { bool value_bool; int32_t value_int; };
  std::string value_string;
  std::vector<uint8_t> value_raw;
};
```

---

## How to Add a New Device to the HA Integration

The HA integration uses per-category mapping dictionaries in each platform file
(`sensor.py`, `switch.py`, etc.). Each mapping entry specifies:

- **Category code** (e.g., `"cl"`)
- **Product ID** (e.g., `"4pbr8eig"`) or a category-wide default
- **DP ID**, data type, entity description, and optional coefficient

To add a new device:
1. Find the device's category and product ID on the Tuya IoT Platform
2. Get the DP list (Functions tab on the device page)
3. Add entries to the `mapping` dict in the relevant platform files
4. Add the product to `devices.py` if it's a new product ID
5. See `CONTIBUTING.md` for the full workflow

---

## Common Maintenance Tasks

### Adding support for a new DP on an existing ESPHome platform

Just update the YAML config — no code changes needed. The sensor, number,
binary_sensor, switch, and text platforms are fully generic. Users specify
arbitrary DP numbers in their YAML.

### Modifying BLE protocol or encryption

The protocol is implemented in two places:
- **ESPHome**: `esphome/components/tuya_ble_device/tuya_ble_device.cpp`
- **HA**: `custom_components/tuya_ble_jp/tuya_ble/tuya_ble.py`

These are independent implementations. Changes to one do not affect the other.

### Updating the DP reference

`esphome/DP_REFERENCE.md` is manually maintained. When new devices are added to
the HA integration's mapping tables, the reference should be updated to match.

### Files you almost never need to touch

- `tuya_ble_device.h/.cpp` — the protocol implementation is stable
- `esphome/components/tuya_ble/__init__.py` — just shared constants
- `config_flow.py` — HA UI flow, rarely changes

### Files that change most often

- `custom_components/tuya_ble_jp/sensor.py`, `switch.py`, `cover.py`, etc. —
  new device mappings are added here
- `custom_components/tuya_ble_jp/devices.py` — new product IDs registered here
- `esphome/DP_REFERENCE.md` — updated when new devices are documented

---

## Gotchas

- **`.gitignore` has `/cover/`** (changed from `cover/`) to avoid ignoring the
  ESPHome cover component directory. The original Python template rule was for
  test coverage reports.
- **DP numbers are not universal.** DP 1 means different things on different
  device categories. Always check the device's category first.
- **The HA integration uses DPCode names** (e.g., `SWITCH_LED`, `BRIGHT_VALUE`)
  while the **ESPHome component uses raw DP numbers**. The DP_REFERENCE.md
  bridges this gap.
- **Cover position is inverted** in the Tuya protocol: 0 = fully open,
  100 = fully closed. The ESPHome cover component handles this inversion.
- **Light color data** is a 12-character hex string encoding HSV values
  (`HHHHSSSSVVVV`). The brightness scale is 0-1000 for v2 devices, 0-255 for v1.
- **Lock state is inverted** from the motor state DP: `locked = !motor_state`.
  The `invert_state` config option (default: true) handles this.
- **Climate temperature coefficient**: TRV devices often send raw values
  multiplied by 10 (e.g., 215 = 21.5°C). Set `temperature_coefficient: 10.0`.
