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


## Fingerprint management commands

Include `kct202_manager.h` in the NXP application. Initialize a dedicated UART first, then call these functions from a FreeRTOS task:

```c
#include "kct202_manager.h"

/* Add/enroll fingerprint ID 1 using four captures and Seeed example flags. */
KCT202_AddFingerprintStart(FINGERPRINT_LPUART, 1U, 4U, 0x0007U);

/* Read the sensor response after sending a command. */
uint8_t confirm = 0xFFU;
uint8_t response_params[32];
size_t response_len = sizeof(response_params);
kct202_status_t status = KCT202_ReadResponse(
    FINGERPRINT_LPUART, 15000U, &confirm,
    response_params, &response_len);

/* Delete only fingerprint ID 1. */
KCT202_DeleteFingerprintStart(FINGERPRINT_LPUART, 1U, 1U);

/* Ask the sensor to verify/identify a finger; firmware-specific behavior applies. */
KCT202_VerifyFingerprintStart(FINGERPRINT_LPUART, 0xFFFFU, 0U);

/* Read count or index page 0 (IDs 0–255). */
KCT202_ReadFingerprintCountStart(FINGERPRINT_LPUART);
KCT202_ReadFingerprintIndexStart(FINGERPRINT_LPUART, 0U);

/* DESTRUCTIVE: erases every stored fingerprint. Require explicit UI confirmation. */
// KCT202_DeleteAllFingerprintsStart(FINGERPRINT_LPUART);
```

**Response handling:** after each command, read and interpret its response before sending the next command. `confirm == 0` indicates success; non-zero values are sensor confirmation/error codes. For verification, validate the returned parameters against the exact sensor firmware manual before connecting the result to unlock/start actions. Never unlock the vehicle based only on a command-send return value.

**Build integration:** add both `.c` files to the MCUXpresso project and add this folder to the include paths. Confirm the selected UART is free in the board pin mux. These functions send protocol commands; they do not yet register an NXP application UI/task or authorize door/engine actions. Build and bench-test before vehicle use.
