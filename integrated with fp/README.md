# Integrated with FP — NXP SLN-VIZN3D-IOT + Grove KCT202

This folder contains a standalone MCUXpresso/MCUX SDK C driver starter for the Seeed Grove Capacitive Fingerprint Sensor (KCT202-compatible protocol), intended to be integrated with the existing NXP `smart_lock` application.

## Important integration boundary

- The existing face-recognition code and its registrations are not modified by this starter.
- The KCT202 UART driver accepts an `LPUART_Type *`; it does **not** choose pins or initialize a UART. This is deliberate: the current project already uses UARTs for board communication/debug/display, and a new UART must be selected only after checking the exact board revision and pin mux.
- Do not connect the fingerprint sensor to LPUART8 or LPUART12 until their current project use has been checked. LPUART12 is used by the debug/display/communication configuration in some builds; LPUART8 is used for board communication in the inspected branch.
- Configure the selected UART for 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control, and use 3.3 V logic-level UART. Power the sensor according to its hardware documentation; do not assume a signal pin can power it.
- This starter has not been built on the user's MCUXpresso workspace or hardware-tested yet. Treat it as integration code to compile and validate, not as a verified production change.

## Files

- `kct202_uart.h` — minimal driver API.
- `kct202_uart.c` — KCT202 packet framing, checksum, enrollment command, and bounded response read.

## Enrollment

Call `KCT202_EnrollStart(&sensor_uart, finger_id, 4, 0x0007)`. Keep calling `KCT202_ReadResponse(...)` while the user completes the four capture steps; check the final confirmation code. The `0x0007` flags mirror Seeed's example settings: LED off after capture, image pretreatment, and suppress intermediate responses. Check the sensor's current datasheet for supported flags and error codes.

Example integration outline (call from a task, not from an interrupt):

```c
kct202_status_t status = KCT202_EnrollStart(FINGERPRINT_LPUART, 1U, 4U, 0x0007U);
if (status == kKCT202_Ok)
{
    /* Prompt user to place/remove the same finger as directed by the sensor LED. */
    uint8_t confirm = 0xFFU;
    uint8_t params[16];
    size_t params_len = sizeof(params);
    status = KCT202_ReadResponse(FINGERPRINT_LPUART, 15000U,
                                 &confirm, params, &params_len);
    /* confirm == 0 indicates success; any non-zero value is a sensor error. */
}
```

The API uses a timeout in milliseconds. Call it from a FreeRTOS task where task scheduling is allowed. If your SDK's LPUART register definitions or clocking differ, adapt the bounded receive helper to the selected SDK driver.

## References

- Seeed library and protocol source: https://github.com/Seeed-Studio/Seeed_Arduino_KCT202
- Sensor guide: https://wiki.seeedstudio.com/Grove-Capacitive-Fingerprint-Sensor/
- NXP project: https://github.com/NXP/vizn3d_smartlock_oobe
