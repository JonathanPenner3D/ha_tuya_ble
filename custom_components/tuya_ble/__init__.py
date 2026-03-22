"""The Tuya BLE integration."""

from __future__ import annotations

import logging

from bleak_retry_connector import BLEAK_RETRY_EXCEPTIONS as BLEAK_EXCEPTIONS, get_device

from homeassistant.components import bluetooth
from homeassistant.components.bluetooth.match import ADDRESS, BluetoothCallbackMatcher
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_ADDRESS, EVENT_HOMEASSISTANT_STOP, Platform
from homeassistant.core import Event, HomeAssistant, callback
from homeassistant.exceptions import ConfigEntryAuthFailed, ConfigEntryNotReady
from homeassistant.helpers import entity_registry as er

from .tuya_ble import TuyaBLEDevice

from .cloud import HASSTuyaBLEDeviceManager
from .const import DOMAIN
from .devices import TuyaBLECoordinator, TuyaBLEData, get_device_product_info

PLATFORMS: list[Platform] = [
    Platform.BUTTON,
    Platform.CLIMATE,
    Platform.LOCK,
    Platform.NUMBER,
    Platform.SENSOR,
    Platform.BINARY_SENSOR,
    Platform.LIGHT,
    Platform.SELECT,
    Platform.SWITCH,
    Platform.TEXT,
    Platform.COVER,
]

_LOGGER = logging.getLogger(__name__)


async def async_migrate_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Migrate old config entry to new version."""
    _LOGGER.debug("Migrating config entry from version %s", entry.version)

    if entry.version == 1:
        # Version 1 → 2: Remove stale entities created by the
        # generate_entity_id("sensor.{}") bug. All non-sensor entities
        # (switch, lock, light, climate, etc.) were registered under a
        # sensor.* entity ID. Remove them so the correct platform domains
        # take over when the entry is set up with the fixed code.
        entity_registry = er.async_get(hass)
        stale_entities = [
            entity_entry
            for entity_entry in er.async_entries_for_config_entry(
                entity_registry, entry.entry_id
            )
            if entity_entry.entity_id.startswith("sensor.")
            and entity_entry.platform == DOMAIN
        ]
        for entity_entry in stale_entities:
            _LOGGER.info(
                "Removing stale entity %s (incorrect sensor. domain from entity ID bug)",
                entity_entry.entity_id,
            )
            entity_registry.async_remove(entity_entry.entity_id)

        hass.config_entries.async_update_entry(entry, version=2)
        _LOGGER.info(
            "Migrated config entry %s to version 2 (%d stale entities removed)",
            entry.entry_id,
            len(stale_entities),
        )

    return True


async def async_setup_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Set up Tuya BLE from a config entry."""
    address: str = entry.data[CONF_ADDRESS]
    ble_device = bluetooth.async_ble_device_from_address(
        hass, address.upper(), True
    ) or await get_device(address)
    if not ble_device:
        raise ConfigEntryNotReady(
            f"Could not find Tuya BLE device with address {address}"
        )
    manager = HASSTuyaBLEDeviceManager(hass, entry.options.copy())
    device = TuyaBLEDevice(manager, ble_device)
    try:
        await device.initialize()
    except BLEAK_EXCEPTIONS as ex:
        raise ConfigEntryNotReady(
            f"Could not initialize Tuya BLE device with address {address}"
        ) from ex
    except Exception as ex:
        raise ConfigEntryNotReady(
            f"Unexpected error initializing Tuya BLE device with address {address}"
        ) from ex

    if not device.device_id:
        if manager.cloud_auth_failed:
            raise ConfigEntryAuthFailed(
                f"Tuya cloud authentication failed for device {address}"
            )
        if HASSTuyaBLEDeviceManager._has_login(entry.options):
            raise ConfigEntryNotReady(
                f"Could not retrieve credentials for Tuya BLE device with address {address}; "
                "cloud may be temporarily unreachable"
            )

    product_info = get_device_product_info(device)

    coordinator = TuyaBLECoordinator(hass, device)

    try:
        await device.update()
    except BLEAK_EXCEPTIONS as ex:
        raise ConfigEntryNotReady(
            f"Could not communicate with Tuya BLE device with address {address}"
        ) from ex

    @callback
    def _async_update_ble(
        service_info: bluetooth.BluetoothServiceInfoBleak,
        change: bluetooth.BluetoothChange,
    ) -> None:
        """Update from a ble callback."""
        device.set_ble_device_and_advertisement_data(
            service_info.device, service_info.advertisement
        )

    entry.async_on_unload(
        bluetooth.async_register_callback(
            hass,
            _async_update_ble,
            BluetoothCallbackMatcher({ADDRESS: address}),
            bluetooth.BluetoothScanningMode.ACTIVE,
        )
    )

    hass.data.setdefault(DOMAIN, {})[entry.entry_id] = TuyaBLEData(
        entry.title,
        device,
        product_info,
        manager,
        coordinator,
    )

    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    entry.async_on_unload(entry.add_update_listener(_async_update_listener))

    async def _async_stop(event: Event) -> None:
        """Close the connection."""
        await device.stop()

    entry.async_on_unload(
        hass.bus.async_listen_once(EVENT_HOMEASSISTANT_STOP, _async_stop)
    )
    return True


async def _async_update_listener(hass: HomeAssistant, entry: ConfigEntry) -> None:
    """Handle options update."""
    data: TuyaBLEData = hass.data[DOMAIN][entry.entry_id]
    if entry.title != data.title:
        await hass.config_entries.async_reload(entry.entry_id)


async def async_unload_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Unload a config entry."""
    if unload_ok := await hass.config_entries.async_unload_platforms(entry, PLATFORMS):
        data: TuyaBLEData = hass.data[DOMAIN].pop(entry.entry_id)
        await data.device.stop()

    return unload_ok
