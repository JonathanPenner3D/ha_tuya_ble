import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, tuya_ble_device
from esphome.const import CONF_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "tuya_ble_device_id"
CONF_DP = "dp"
CONF_DP_TYPE = "dp_type"
CONF_COEFFICIENT = "coefficient"
CONF_SENSORS = "sensors"

tuya_ble_sensor_ns = cg.esphome_ns.namespace("tuya_ble_sensor")
TuyaBLESensor = tuya_ble_sensor_ns.class_(
    "TuyaBLESensor", sensor.Sensor, cg.Component
)

DP_TYPES = {
    "raw": 0,
    "bool": 1,
    "int": 2,
    "string": 3,
    "enum": 4,
    "bitmap": 5,
}

SINGLE_SENSOR_SCHEMA = (
    sensor.sensor_schema(TuyaBLESensor)
    .extend(
        {
            cv.Required(CONF_DP): cv.uint8_t,
            cv.Optional(CONF_DP_TYPE): cv.enum(DP_TYPES, lower=True),
            cv.Optional(CONF_COEFFICIENT, default=1.0): cv.float_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
        cv.Required(CONF_SENSORS): cv.ensure_list(SINGLE_SENSOR_SCHEMA),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DEVICE_ID])

    for sens_conf in config[CONF_SENSORS]:
        var = await sensor.new_sensor(sens_conf)
        await cg.register_component(var, sens_conf)
        cg.add(var.set_tuya_ble_device(parent))
        cg.add(var.set_dp_id(sens_conf[CONF_DP]))
        if CONF_DP_TYPE in sens_conf:
            cg.add(var.set_dp_type(sens_conf[CONF_DP_TYPE]))
        cg.add(var.set_coefficient(sens_conf[CONF_COEFFICIENT]))
