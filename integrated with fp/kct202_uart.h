/*
 * Integrated with FP — KCT202-compatible UART interface.
 *
 * Command IDs are taken from Seeed-Studio/Seeed_Arduino_KCT202/src/KCT202.h.
 * This is a protocol-level starter; UART setup and board pin mux remain external.
 */
#ifndef KCT202_UART_H
#define KCT202_UART_H

#include <stddef.h>
#include <stdint.h>
#include "fsl_lpuart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    kKCT202_Ok = 0,
    kKCT202_InvalidArgument = -1,
    kKCT202_Timeout = -2,
    kKCT202_BadHeader = -3,
    kKCT202_BadLength = -4,
    kKCT202_BadChecksum = -5,
    kKCT202_BufferTooSmall = -6,
    kKCT202_UartError = -7,
    /* The public Seeed KCT202 protocol/library does not document factory reset. */
    kKCT202_UnsupportedCommand = -8
} kct202_status_t;

/*
 * Start enrollment. UART must already be initialized for the sensor and have
 * verified pin mux. Seeed's example uses record_count=4 and flags=0x0007.
 */
kct202_status_t KCT202_EnrollStart(LPUART_Type *uart,
                                   uint16_t finger_id,
                                   uint8_t record_count,
                                   uint16_t flags);

/* Delete record_count templates beginning at finger_id (official 0x0C command). */
kct202_status_t KCT202_DeleteFingerprintStart(LPUART_Type *uart,
                                              uint16_t finger_id,
                                              uint16_t record_count);

/* Erase all enrolled fingerprint templates (official 0x0D command). DESTRUCTIVE. */
kct202_status_t KCT202_DeleteAllFingerprintsStart(LPUART_Type *uart);

/*
 * No factory-reset directive is defined in the public Seeed KCT202 command
 * header/library. This function deliberately sends no bytes and returns
 * kKCT202_UnsupportedCommand rather than guessing a command code.
 */
kct202_status_t KCT202_FactoryResetStart(LPUART_Type *uart);

/*
 * Read one response packet with a finite timeout.
 * confirm_code receives the sensor confirmation/error code. params_len is
 * input capacity and output actual parameter length; params may be NULL only
 * when the response has no parameter bytes.
 */
kct202_status_t KCT202_ReadResponse(LPUART_Type *uart,
                                    uint32_t timeout_ms,
                                    uint8_t *confirm_code,
                                    uint8_t *params,
                                    size_t *params_len);

#ifdef __cplusplus
}
#endif

#endif /* KCT202_UART_H */
