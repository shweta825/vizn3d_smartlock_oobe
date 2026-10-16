/*
 * KCT202 fingerprint-management commands for NXP SLN-VIZN3D-IOT.
 * Uses the UART framing and response reader declared in kct202_uart.h.
 * UART must be initialized separately; no pin mux is selected here.
 */
#ifndef KCT202_MANAGER_H
#define KCT202_MANAGER_H

#include "kct202_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Start enrollment. Place the finger on the sensor as prompted by its LED/status.
 * flags: Seeed KCT202 flags, e.g. 0x0007 for the documented example.
 */
kct202_status_t KCT202_AddFingerprintStart(LPUART_Type *uart,
                                           uint16_t finger_id,
                                           uint8_t captures,
                                           uint16_t flags);

/* Delete count consecutive templates starting at finger_id. Use count=1 for one ID. */
kct202_status_t KCT202_DeleteFingerprintStart(LPUART_Type *uart,
                                              uint16_t finger_id,
                                              uint8_t count);

/* Verify a specific ID, or use 0xFFFF to ask the sensor to identify a matching finger
 * if supported by the installed KCT202 firmware. param is the sensor control field.
 */
kct202_status_t KCT202_VerifyFingerprintStart(LPUART_Type *uart,
                                              uint16_t finger_id,
                                              uint16_t param);

/* Read the number of enrolled templates. Read the reply with KCT202_ReadResponse(). */
kct202_status_t KCT202_ReadFingerprintCountStart(LPUART_Type *uart);

/* Read one index-table page (0..3 according to Seeed library documentation). */
kct202_status_t KCT202_ReadFingerprintIndexStart(LPUART_Type *uart, uint8_t page);

/* Clear every enrolled template. Destructive: call only after explicit user confirmation. */
kct202_status_t KCT202_DeleteAllFingerprintsStart(LPUART_Type *uart);

#ifdef __cplusplus
}
#endif
#endif
