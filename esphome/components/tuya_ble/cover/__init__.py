import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover, tuya_ble_device
from esphome.const import CONF_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "device_id"
CONF_STATE_DP = "state_dp"
CONF_POSITION_DP = "position_dp"
CONF_POSITION_SET_DP = "position_set_dp"
CONF_TILT_DP = "tilt_dp"

tuya_ble_cover_ns = cg.esphome_ns.namespace("tuya_ble_cover")
TuyaBLECover = tuya_ble_cover_ns.class_("TuyaBLECover", cover.Cover, cg.Component)

# Standard Tuya cover (category "cl") DP assignments
DEFAULT_STATE_DP = 1         # 0=Open, 1=Stop, 2=Close
DEFAULT_POSITION_SET_DP = 2  # Target position (0-100)
DEFAULT_POSITION_DP = 3      # Current position (0-100)

CONFIG_SCHEMA = (
    cover.COVER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(TuyaBLECover),
            cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
            cv.Optional(CONF_STATE_DP, default=DEFAULT_STATE_DP): cv.uint8_t,
            cv.Optional(CONF_POSITION_DP, default=DEFAULT_POSITION_DP): cv.uint8_t,
            cv.Optional(CONF_POSITION_SET_DP, default=DEFAULT_POSITION_SET_DP): cv.uint8_t,
            cv.Optional(CONF_TILT_DP): cv.uint8_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    parent = await cg.get_variable(config[CONF_DEVICE_ID])
    cg.add(var.set_tuya_ble_device(parent))

    cg.add(var.set_state_dp(config[CONF_STATE_DP]))
    cg.add(var.set_position_dp(config[CONF_POSITION_DP]))
    cg.add(var.set_position_set_dp(config[CONF_POSITION_SET_DP]))
    if CONF_TILT_DP in config:
        cg.add(var.set_tilt_dp(config[CONF_TILT_DP]))
