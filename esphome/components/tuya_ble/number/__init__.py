import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number, tuya_ble_device
from esphome.const import CONF_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "tuya_ble_device_id"
CONF_DP = "dp"
CONF_DP_TYPE = "dp_type"
CONF_COEFFICIENT = "coefficient"
CONF_NUMBERS = "numbers"
CONF_MIN_VALUE = "min_value"
CONF_MAX_VALUE = "max_value"
CONF_STEP = "step"

tuya_ble_number_ns = cg.esphome_ns.namespace("tuya_ble_number")
TuyaBLENumber = tuya_ble_number_ns.class_(
    "TuyaBLENumber", number.Number, cg.Component
)

DP_TYPES = {
    "raw": 0,
    "bool": 1,
    "int": 2,
    "string": 3,
    "enum": 4,
    "bitmap": 5,
}

SINGLE_NUMBER_SCHEMA = (
    number.number_schema(TuyaBLENumber)
    .extend(
        {
            cv.Required(CONF_DP): cv.uint8_t,
            cv.Optional(CONF_DP_TYPE): cv.enum(DP_TYPES, lower=True),
            cv.Optional(CONF_COEFFICIENT, default=1.0): cv.float_,
            cv.Optional(CONF_MIN_VALUE, default=0): cv.float_,
            cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
            cv.Optional(CONF_STEP, default=1): cv.float_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
        cv.Required(CONF_NUMBERS): cv.ensure_list(SINGLE_NUMBER_SCHEMA),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DEVICE_ID])

    for num_conf in config[CONF_NUMBERS]:
        var = await number.new_number(
            num_conf,
            min_value=num_conf[CONF_MIN_VALUE],
            max_value=num_conf[CONF_MAX_VALUE],
            step=num_conf[CONF_STEP],
        )
        await cg.register_component(var, num_conf)
        cg.add(var.set_tuya_ble_device(parent))
        cg.add(var.set_dp_id(num_conf[CONF_DP]))
        if CONF_DP_TYPE in num_conf:
            cg.add(var.set_dp_type(num_conf[CONF_DP_TYPE]))
        cg.add(var.set_coefficient(num_conf[CONF_COEFFICIENT]))
