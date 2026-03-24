import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor, tuya_ble_device
from esphome.const import CONF_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "tuya_ble_device_id"
CONF_DP = "dp"
CONF_TEXT_SENSORS = "text_sensors"
CONF_ENUM_VALUES = "enum_values"

tuya_ble_text_sensor_ns = cg.esphome_ns.namespace("tuya_ble_text_sensor")
TuyaBLETextSensor = tuya_ble_text_sensor_ns.class_(
    "TuyaBLETextSensor", text_sensor.TextSensor, cg.Component
)

SINGLE_TEXT_SENSOR_SCHEMA = (
    text_sensor.text_sensor_schema(TuyaBLETextSensor)
    .extend(
        {
            cv.Required(CONF_DP): cv.uint8_t,
            cv.Optional(CONF_ENUM_VALUES): cv.Schema(
                {cv.int_: cv.string}
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DEVICE_ID): cv.use_id(tuya_ble_device.TuyaBLEDevice),
        cv.Required(CONF_TEXT_SENSORS): cv.ensure_list(SINGLE_TEXT_SENSOR_SCHEMA),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DEVICE_ID])

    for conf in config[CONF_TEXT_SENSORS]:
        var = await text_sensor.new_text_sensor(conf)
        await cg.register_component(var, conf)
        cg.add(var.set_tuya_ble_device(parent))
        cg.add(var.set_dp_id(conf[CONF_DP]))
        if CONF_ENUM_VALUES in conf:
            for int_val, str_val in conf[CONF_ENUM_VALUES].items():
                cg.add(var.add_enum_value(int_val, str_val))
