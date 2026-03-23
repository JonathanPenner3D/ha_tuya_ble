import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, tuya_ble_device
from esphome.const import CONF_ID, CONF_OUTPUT_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "device_id"
CONF_SWITCH_DP = "switch_dp"
CONF_BRIGHTNESS_DP = "brightness_dp"
CONF_BRIGHTNESS_MAX = "brightness_max"
CONF_COLOR_TEMP_DP = "color_temp_dp"
CONF_COLOR_TEMP_MIN_MIREDS = "color_temp_min_mireds"
CONF_COLOR_TEMP_MAX_MIREDS = "color_temp_max_mireds"
CONF_COLOR_DP = "color_dp"
CONF_WORK_MODE_DP = "work_mode_dp"

tuya_ble_light_ns = cg.esphome_ns.namespace("tuya_ble_light")
TuyaBLELight = tuya_ble_light_ns.class_(
    "TuyaBLELight", light.LightOutput, cg.Component
)

CONFIG_SCHEMA = (
    light.light_schema(TuyaBLELight)
    .extend(
        {
            cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
            cv.Required(CONF_SWITCH_DP): cv.uint8_t,
            cv.Optional(CONF_BRIGHTNESS_DP): cv.uint8_t,
            cv.Optional(CONF_BRIGHTNESS_MAX, default=1000): cv.int_range(
                min=1, max=10000
            ),
            cv.Optional(CONF_COLOR_TEMP_DP): cv.uint8_t,
            cv.Optional(CONF_COLOR_TEMP_MIN_MIREDS, default=153): cv.int_range(
                min=1, max=1000
            ),
            cv.Optional(CONF_COLOR_TEMP_MAX_MIREDS, default=500): cv.int_range(
                min=1, max=1000
            ),
            cv.Optional(CONF_COLOR_DP): cv.uint8_t,
            cv.Optional(CONF_WORK_MODE_DP): cv.uint8_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)

    parent = await cg.get_variable(config[CONF_DEVICE_ID])
    cg.add(var.set_tuya_ble_device(parent))

    cg.add(var.set_switch_dp(config[CONF_SWITCH_DP]))
    if CONF_BRIGHTNESS_DP in config:
        cg.add(var.set_brightness_dp(config[CONF_BRIGHTNESS_DP]))
        cg.add(var.set_brightness_max(config[CONF_BRIGHTNESS_MAX]))
    if CONF_COLOR_TEMP_DP in config:
        cg.add(var.set_color_temp_dp(config[CONF_COLOR_TEMP_DP]))
        cg.add(var.set_color_temp_min_mireds(config[CONF_COLOR_TEMP_MIN_MIREDS]))
        cg.add(var.set_color_temp_max_mireds(config[CONF_COLOR_TEMP_MAX_MIREDS]))
    if CONF_COLOR_DP in config:
        cg.add(var.set_color_dp(config[CONF_COLOR_DP]))
    if CONF_WORK_MODE_DP in config:
        cg.add(var.set_work_mode_dp(config[CONF_WORK_MODE_DP]))
