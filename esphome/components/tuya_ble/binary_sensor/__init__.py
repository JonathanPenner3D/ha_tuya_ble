import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, tuya_ble_device
from esphome.const import CONF_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "tuya_ble_device_id"
CONF_DP = "dp"
CONF_BINARY_SENSORS = "binary_sensors"

tuya_ble_binary_sensor_ns = cg.esphome_ns.namespace("tuya_ble_binary_sensor")
TuyaBLEBinarySensor = tuya_ble_binary_sensor_ns.class_(
    "TuyaBLEBinarySensor", binary_sensor.BinarySensor, cg.Component
)

SINGLE_BINARY_SENSOR_SCHEMA = (
    binary_sensor.binary_sensor_schema(TuyaBLEBinarySensor)
    .extend(
        {
            cv.Required(CONF_DP): cv.uint8_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
        cv.Required(CONF_BINARY_SENSORS): cv.ensure_list(SINGLE_BINARY_SENSOR_SCHEMA),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DEVICE_ID])

    for bs_conf in config[CONF_BINARY_SENSORS]:
        var = await binary_sensor.new_binary_sensor(bs_conf)
        await cg.register_component(var, bs_conf)
        cg.add(var.set_tuya_ble_device(parent))
        cg.add(var.set_dp_id(bs_conf[CONF_DP]))
