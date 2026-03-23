import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, tuya_ble_device
from esphome.const import CONF_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "device_id"
CONF_SWITCH_DP = "switch_dp"
CONF_CURRENT_TEMP_DP = "current_temperature_dp"
CONF_TARGET_TEMP_DP = "target_temperature_dp"
CONF_TEMP_COEFFICIENT = "temperature_coefficient"
CONF_TEMP_STEP = "temperature_step"
CONF_TEMP_MIN = "temperature_min"
CONF_TEMP_MAX = "temperature_max"

tuya_ble_climate_ns = cg.esphome_ns.namespace("tuya_ble_climate")
TuyaBLEClimate = tuya_ble_climate_ns.class_(
    "TuyaBLEClimate", climate.Climate, cg.Component
)

CONFIG_SCHEMA = (
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(TuyaBLEClimate),
            cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
            cv.Required(CONF_SWITCH_DP): cv.uint8_t,
            cv.Required(CONF_CURRENT_TEMP_DP): cv.uint8_t,
            cv.Required(CONF_TARGET_TEMP_DP): cv.uint8_t,
            cv.Optional(CONF_TEMP_COEFFICIENT, default=1.0): cv.float_,
            cv.Optional(CONF_TEMP_STEP, default=1.0): cv.float_,
            cv.Optional(CONF_TEMP_MIN, default=5.0): cv.float_,
            cv.Optional(CONF_TEMP_MAX, default=30.0): cv.float_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    parent = await cg.get_variable(config[CONF_DEVICE_ID])
    cg.add(var.set_tuya_ble_device(parent))

    cg.add(var.set_switch_dp(config[CONF_SWITCH_DP]))
    cg.add(var.set_current_temp_dp(config[CONF_CURRENT_TEMP_DP]))
    cg.add(var.set_target_temp_dp(config[CONF_TARGET_TEMP_DP]))
    cg.add(var.set_temp_coefficient(config[CONF_TEMP_COEFFICIENT]))
    cg.add(var.set_temp_step(config[CONF_TEMP_STEP]))
    cg.add(var.set_temp_min(config[CONF_TEMP_MIN]))
    cg.add(var.set_temp_max(config[CONF_TEMP_MAX]))
