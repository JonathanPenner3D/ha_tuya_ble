import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text, tuya_ble_device
from esphome.const import CONF_ID

DEPENDENCIES = ["tuya_ble_device"]
CODEOWNERS = ["@JonathanPenner3D"]

CONF_DEVICE_ID = "device_id"
CONF_DP = "dp"
CONF_TEXTS = "texts"

tuya_ble_text_ns = cg.esphome_ns.namespace("tuya_ble_text")
TuyaBLEText = tuya_ble_text_ns.class_("TuyaBLEText", text.Text, cg.Component)

SINGLE_TEXT_SCHEMA = (
    text.text_schema(TuyaBLEText)
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
        cv.Required(CONF_TEXTS): cv.ensure_list(SINGLE_TEXT_SCHEMA),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DEVICE_ID])

    for text_conf in config[CONF_TEXTS]:
        var = await text.new_text(text_conf)
        await cg.register_component(var, text_conf)
        cg.add(var.set_tuya_ble_device(parent))
        cg.add(var.set_dp_id(text_conf[CONF_DP]))
