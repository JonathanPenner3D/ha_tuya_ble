import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client
from esphome.const import CONF_ID

CODEOWNERS = ["@JonathanPenner3D"]
DEPENDENCIES = ["ble_client"]
MULTI_CONF = True

CONF_LOCAL_KEY = "local_key"
CONF_UUID = "uuid"
CONF_DEVICE_ID = "device_id"

tuya_ble_device_ns = cg.esphome_ns.namespace("tuya_ble_device")
TuyaBLEDevice = tuya_ble_device_ns.class_(
    "TuyaBLEDevice", cg.Component, ble_client.BLEClientNode
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TuyaBLEDevice),
            cv.Required(CONF_LOCAL_KEY): cv.string,
            cv.Required(CONF_UUID): cv.string,
            cv.Required(CONF_DEVICE_ID): cv.string,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(ble_client.BLE_CLIENT_SCHEMA)
)


async def to_code(config):
    cg.add_build_flag('-DMBEDTLS_CONFIG_FILE="mbedtls/esp_config.h"')
    cg.add_platformio_option(
        "build_flags", ['-DMBEDTLS_CONFIG_FILE="mbedtls/esp_config.h"']
    )

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)

    cg.add(var.set_local_key(config[CONF_LOCAL_KEY]))
    cg.add(var.set_uuid(config[CONF_UUID]))
    cg.add(var.set_device_id(config[CONF_DEVICE_ID]))
