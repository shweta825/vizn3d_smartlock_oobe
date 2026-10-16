/*
 * Integrated with FP — KCT202-compatible UART interface.
 *
 * The framing and enrollment directive follow Seeed-Studio/Seeed_Arduino_KCT202.
 * This file is a starter integration; validate against the exact sensor revision.
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
    kKCT202_UartError = -7
} kct202_status_t;

/*
 * Build and send the Seeed KCT202 auto-register command.
 * UART must already be initialized to 115200 8-N-1, with valid board pin mux.
 * record_count is normally 4. flags 0x0007 match the Seeed registration example.
 */
kct202_status_t KCT202_EnrollStart(LPUART_Type *uart,
                                   uint16_t finger_id,
                                   uint8_t record_count,
                                   uint16_t flags);

/*
 * Read one response packet with a finite timeout.
 * confirm_code receives the sensor confirmation/error code.
 * params is optional when params_len points to zero; otherwise *params_len is
 * input capacity and output actual parameter length.
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
