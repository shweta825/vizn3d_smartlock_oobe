# Integrated with FP — NXP SLN-VIZN3D-IOT + Grove KCT202

This folder contains a protocol-level MCUXpresso/MCUX SDK C starter for the Seeed Grove Capacitive Fingerprint Sensor (KCT202-compatible protocol), intended for integration with the existing NXP `smart_lock` application.

## Verified public protocol commands

The official Seeed library header defines:
- `AUTO_REGISTER_FINGER_TEMP = 0x31` — enroll a fingerprint.
- `AUTO_DELETE_FINGER_TEMP = 0x0C` — delete a range of records by starting ID and count.
- `CLEAN_ALL_FINGER_TEMP = 0x0D` — delete all registered fingerprint templates.
- `READ_VALID_TEMP_CNT = 0x1D` and `READ_INDEX_TABLE_TEMP = 0x1F` — query count/index.
- `CONFIG_MODULE = 0x0E` — configure a documented module setting.

The public Seeed KCT202 header/source does **not** define or document a general factory-reset command. Therefore this branch does not send a guessed command. `KCT202_FactoryResetStart()` returns `kKCT202_UnsupportedCommand` and transmits no bytes. A complete reset beyond deleting fingerprint templates requires an official firmware/manual confirmation from Seeed or the module supplier.

## Important integration boundary

- The existing face-recognition code is not modified by this driver.
- The driver accepts an `LPUART_Type *`; it does **not** choose pins or initialize the UART. Select a free UART only after checking the exact board revision and pin mux.
- Do not reuse LPUART8 or LPUART12 until their current use is checked in the project configuration.
- Configure the selected UART according to the sensor's official interface specification. Seeed's library README says the sensor transport UART is fixed at 57600 baud; use that documented rate unless the exact sensor firmware documentation says otherwise.
- This starter has not been compiled in the user's MCUXpresso workspace or tested on hardware. Build and bench-test before use.

## API

### Enroll

```c
KCT202_EnrollStart(sensor_uart, 1U, 4U, 0x0007U);
```

The `0x0007` flags mirror Seeed's example settings. Read and process sensor responses until enrollment finishes.

### Delete one fingerprint

```c
KCT202_DeleteFingerprintStart(sensor_uart, 1U, 1U);
```

This sends the documented delete-range command with start ID 1 and count 1. Read the response and check its confirmation code.

### Delete all fingerprints (destructive)

```c
KCT202_DeleteAllFingerprintsStart(sensor_uart);
```

This sends official directive `0x0D`, which the Seeed library identifies as clearing the template library. Require an explicit user/admin confirmation in the application before calling it. Read the sensor response and check its confirmation code.

### Full factory reset

```c
kct202_status_t status = KCT202_FactoryResetStart(sensor_uart);
/* Returns kKCT202_UnsupportedCommand; no command is transmitted. */
```

The public Seeed library does not document a full factory-reset operation. Do not treat delete-all as a full factory reset: it only clears enrolled fingerprint templates.

## Response handling

After sending a command, call `KCT202_ReadResponse()` from a FreeRTOS task and check both the driver status and the sensor's confirmation code. A command-send success only means the UART packet was sent; it does not mean the sensor accepted or completed the operation.

## Hardware and safety

- Confirm the UART TX/RX pin mapping and voltage levels against the exact SLN-VIZN3D-IOT board revision.
- Do not assume the Grove connector's sensor power requirements from the UART code.
- Do not connect a fingerprint result directly to vehicle unlock or engine-start controls until response parsing, failure paths, authorization, and the physical system have been validated.

## References

- Official Seeed KCT202 library and command definitions: https://github.com/Seeed-Studio/Seeed_Arduino_KCT202
- Official Seeed sensor guide: https://wiki.seeedstudio.com/Grove-Capacitive-Fingerprint-Sensor/
- NXP project: https://github.com/NXP/vizn3d_smartlock_oobe
