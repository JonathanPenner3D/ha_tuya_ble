import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, tuya_ble_device
from esphome.const import CONF_ID, DEVICE_CLASS_CONNECTIVITY

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "tuya_ble_device_id"

tuya_ble_connection_ns = cg.esphome_ns.namespace("tuya_ble_connection_status")
TuyaBLEConnectionStatus = tuya_ble_connection_ns.class_(
    "TuyaBLEConnectionStatus", binary_sensor.BinarySensor, cg.Component
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(
        TuyaBLEConnectionStatus,
        device_class=DEVICE_CLASS_CONNECTIVITY,
    )
    .extend(
        {
            cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_DEVICE_ID])
    cg.add(var.set_tuya_ble_device(parent))
