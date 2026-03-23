import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import tuya_ble_device

CODEOWNERS = ["@JonathanPenner3D"]
DEPENDENCIES = ["tuya_ble_device"]

CONF_TUYA_BLE_DEVICE_ID = "device_id"
CONF_DP = "dp"
CONF_DP_TYPE = "dp_type"

# DP type enum values for optional override
DP_TYPES = {
    "raw": 0,
    "bool": 1,
    "int": 2,
    "string": 3,
    "enum": 4,
    "bitmap": 5,
}
