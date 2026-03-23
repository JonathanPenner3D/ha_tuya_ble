import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import lock, tuya_ble_device
from esphome.const import CONF_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "device_id"
CONF_LOCK_STATE_DP = "lock_state_dp"
CONF_LOCK_CONTROL_DP = "lock_control_dp"
CONF_INVERT_STATE = "invert_state"

tuya_ble_lock_ns = cg.esphome_ns.namespace("tuya_ble_lock")
TuyaBLELock = tuya_ble_lock_ns.class_("TuyaBLELock", lock.Lock, cg.Component)

CONFIG_SCHEMA = (
    lock.LOCK_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(TuyaBLELock),
            cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
            cv.Required(CONF_LOCK_STATE_DP): cv.uint8_t,
            cv.Required(CONF_LOCK_CONTROL_DP): cv.uint8_t,
            cv.Optional(CONF_INVERT_STATE, default=True): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await lock.register_lock(var, config)

    parent = await cg.get_variable(config[CONF_DEVICE_ID])
    cg.add(var.set_tuya_ble_device(parent))

    cg.add(var.set_lock_state_dp(config[CONF_LOCK_STATE_DP]))
    cg.add(var.set_lock_control_dp(config[CONF_LOCK_CONTROL_DP]))
    cg.add(var.set_invert_state(config[CONF_INVERT_STATE]))
