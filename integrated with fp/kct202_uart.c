/*
 * Integrated with FP — KCT202-compatible UART command starter.
 *
 * Protocol verified against the public Seeed source:
 * https://github.com/Seeed-Studio/Seeed_Arduino_KCT202
 * Official command IDs used here: enroll 0x31, delete range 0x0C,
 * delete-all templates 0x0D. No factory-reset command is documented by the
 * public library, so KCT202_FactoryResetStart intentionally sends nothing.
 *
 * This file does not initialize a UART or select board pins.
 */
#include "kct202_uart.h"

#include <stdbool.h>
#include "fsl_common.h"
#include "FreeRTOS.h"
#include "task.h"

#define KCT202_HEADER_HI       (0xEFU)
#define KCT202_HEADER_LO       (0x01U)
#define KCT202_PACKET_COMMAND  (0x01U)
#define KCT202_PACKET_RESPONSE (0x07U)
#define KCT202_CMD_ENROLL      (0x31U)
#define KCT202_CMD_DELETE      (0x0CU)
#define KCT202_CMD_DELETE_ALL  (0x0DU)
#define KCT202_DEFAULT_ADDR    (0xFFFFFFFFUL)
#define KCT202_MAX_PACKET      (128U)
#define KCT202_FIXED_BYTES     (9U)

static uint16_t KCT202_Checksum(const uint8_t *data, size_t length)
{
    uint32_t sum = 0U;
    size_t i;

    for (i = 0U; i < length; ++i)
    {
        sum += data[i];
    }
    return (uint16_t)(sum & 0xFFFFU);
}

/*
 * KCT202 packet: EF01 + address(4) + packet-id(1) + length(2) +
 * payload + checksum(2). Length includes payload and checksum, not header.
 */
static kct202_status_t KCT202_SendCommand(LPUART_Type *uart,
                                          const uint8_t *payload,
                                          size_t payload_len)
{
    uint8_t packet[KCT202_MAX_PACKET];
    size_t packet_len;
    uint16_t length_field;
    uint16_t checksum;

    if ((uart == NULL) || (payload == NULL) || (payload_len == 0U) ||
        (payload_len + 2U > (KCT202_MAX_PACKET - KCT202_FIXED_BYTES)) ||
        (payload_len + 2U > 0xFFFFU))
    {
        return kKCT202_InvalidArgument;
    }

    length_field = (uint16_t)(payload_len + 2U);
    packet[0] = KCT202_HEADER_HI;
    packet[1] = KCT202_HEADER_LO;
    packet[2] = 0xFFU;
    packet[3] = 0xFFU;
    packet[4] = 0xFFU;
    packet[5] = 0xFFU;
    packet[6] = KCT202_PACKET_COMMAND;
    packet[7] = (uint8_t)(length_field >> 8);
    packet[8] = (uint8_t)length_field;

    for (size_t i = 0U; i < payload_len; ++i)
    {
        packet[9U + i] = payload[i];
    }

    /* Sum packet ID, both length bytes and payload; checksum itself excluded. */
    checksum = KCT202_Checksum(&packet[6], payload_len + 3U);
    packet[9U + payload_len] = (uint8_t)(checksum >> 8);
    packet[10U + payload_len] = (uint8_t)checksum;
    packet_len = KCT202_FIXED_BYTES + (size_t)length_field;

    LPUART_WriteBlocking(uart, packet, packet_len);
    return kKCT202_Ok;
}

static bool KCT202_WaitRx(LPUART_Type *uart, uint32_t timeout_ms, uint8_t *value)
{
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    if (timeout_ticks == 0U) timeout_ticks = 1U;

    while ((xTaskGetTickCount() - start) < timeout_ticks)
    {
        uint32_t flags = LPUART_GetStatusFlags(uart);

        if ((flags & (uint32_t)kLPUART_RxOverrunFlag) != 0U)
        {
            LPUART_ClearStatusFlags(uart, (uint32_t)kLPUART_RxOverrunFlag);
            return false;
        }

        if ((flags & (uint32_t)kLPUART_RxDataRegFullFlag) != 0U)
        {
            *value = LPUART_ReadByte(uart);
            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(1U));
    }

    return false;
}

static kct202_status_t KCT202_ReadExact(LPUART_Type *uart,
                                        uint8_t *data,
                                        size_t length,
                                        uint32_t timeout_ms)
{
    size_t i;

    for (i = 0U; i < length; ++i)
    {
        if (!KCT202_WaitRx(uart, timeout_ms, &data[i]))
        {
            return kKCT202_Timeout;
        }
    }
    return kKCT202_Ok;
}

kct202_status_t KCT202_EnrollStart(LPUART_Type *uart,
                                   uint16_t finger_id,
                                   uint8_t record_count,
                                   uint16_t flags)
{
    uint8_t payload[6];

    if ((uart == NULL) || (record_count == 0U))
    {
        return kKCT202_InvalidArgument;
    }

    payload[0] = KCT202_CMD_ENROLL;
    payload[1] = (uint8_t)(finger_id >> 8);
    payload[2] = (uint8_t)finger_id;
    payload[3] = record_count;
    payload[4] = (uint8_t)(flags >> 8);
    payload[5] = (uint8_t)flags;
    return KCT202_SendCommand(uart, payload, sizeof(payload));
}

kct202_status_t KCT202_DeleteFingerprintStart(LPUART_Type *uart,
                                              uint16_t finger_id,
                                              uint16_t record_count)
{
    uint8_t payload[5];

    if ((uart == NULL) || (record_count == 0U))
    {
        return kKCT202_InvalidArgument;
    }

    /* Seeed deleteParamData(): directive, start ID (u16), count (u16). */
    payload[0] = KCT202_CMD_DELETE;
    payload[1] = (uint8_t)(finger_id >> 8);
    payload[2] = (uint8_t)finger_id;
    payload[3] = (uint8_t)(record_count >> 8);
    payload[4] = (uint8_t)record_count;
    return KCT202_SendCommand(uart, payload, sizeof(payload));
}

kct202_status_t KCT202_DeleteAllFingerprintsStart(LPUART_Type *uart)
{
    const uint8_t payload[1] = { KCT202_CMD_DELETE_ALL };
    return KCT202_SendCommand(uart, payload, sizeof(payload));
}

kct202_status_t KCT202_FactoryResetStart(LPUART_Type *uart)
{
    if (uart == NULL)
    {
        return kKCT202_InvalidArgument;
    }

    /* Intentionally do not transmit an undocumented/guessed command. */
    return kKCT202_UnsupportedCommand;
}

kct202_status_t KCT202_ReadResponse(LPUART_Type *uart,
                                    uint32_t timeout_ms,
                                    uint8_t *confirm_code,
                                    uint8_t *params,
                                    size_t *params_len)
{
    uint8_t packet[KCT202_MAX_PACKET];
    uint8_t byte;
    uint16_t length;
    uint16_t checksum;
    uint16_t received_checksum;
    size_t capacity = 0U;
    kct202_status_t status;

    if ((uart == NULL) || (confirm_code == NULL) || (params_len == NULL))
    {
        return kKCT202_InvalidArgument;
    }

    if (params != NULL)
    {
        capacity = *params_len;
    }

    /* Find packet header, allowing unrelated bytes before the next frame. */
    status = KCT202_ReadExact(uart, &byte, 1U, timeout_ms);
    if (status != kKCT202_Ok) return status;

    while (byte != KCT202_HEADER_HI)
    {
        status = KCT202_ReadExact(uart, &byte, 1U, timeout_ms);
        if (status != kKCT202_Ok) return status;
    }

    status = KCT202_ReadExact(uart, &packet[1], 1U, timeout_ms);
    if (status != kKCT202_Ok) return status;
    if (packet[1] != KCT202_HEADER_LO) return kKCT202_BadHeader;
    packet[0] = KCT202_HEADER_HI;

    status = KCT202_ReadExact(uart, &packet[2], 7U, timeout_ms);
    if (status != kKCT202_Ok) return status;

    length = (uint16_t)(((uint16_t)packet[7] << 8) | packet[8]);
    if ((length < 3U) || ((size_t)length + KCT202_FIXED_BYTES > sizeof(packet)))
    {
        return kKCT202_BadLength;
    }

    status = KCT202_ReadExact(uart, &packet[9], length, timeout_ms);
    if (status != kKCT202_Ok) return status;

    checksum = KCT202_Checksum(&packet[6], (size_t)length + 1U);
    received_checksum = (uint16_t)(((uint16_t)packet[7U + length] << 8) |
                                   packet[8U + length]);

    if (checksum != received_checksum)
    {
        return kKCT202_BadChecksum;
    }

    if (packet[6] != KCT202_PACKET_RESPONSE)
    {
        return kKCT202_BadHeader;
    }

    *confirm_code = packet[9];
    *params_len = (size_t)length - 3U;

    if (*params_len > 0U)
    {
        if ((params == NULL) || (capacity < *params_len))
        {
            return kKCT202_BufferTooSmall;
        }
        for (size_t i = 0U; i < *params_len; ++i)
        {
            params[i] = packet[10U + i];
        }
    }

    return kKCT202_Ok;
}
