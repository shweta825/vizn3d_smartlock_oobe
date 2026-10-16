/*
 * Integrated with FP — KCT202-compatible UART driver starter.
 *
 * Protocol reference: Seeed-Studio/Seeed_Arduino_KCT202/src/Protocol.cpp
 * and src/KCT202.cpp. Header is 0xEF01, default address 0xFFFFFFFF,
 * command packet ID 0x01, and checksum is the 16-bit sum from packet ID
 * through the payload (inclusive of the two length bytes).
 *
 * This implementation intentionally does not initialize a UART or select pins.
 */
#include "kct202_uart.h"

#include <stdbool.h>
#include "fsl_common.h"
#include "FreeRTOS.h"
#include "task.h"

#define KCT202_HEADER_HI       (0xEFU)
#define KCT202_HEADER_LO       (0x01U)
#define KCT202_PACKET_COMMAND  (0x01U)
#define KCT202_CMD_ENROLL      (0x31U)
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

static bool KCT202_WaitRx(LPUART_Type *uart, uint32_t timeout_ms, uint8_t *value)
{
    TickType_t start = xTaskGetTickCount();
    const TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

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
    uint8_t packet[17];
    uint16_t packet_length = 8U; /* directive + ID + count + flags + checksum */
    uint16_t checksum;

    if ((uart == NULL) || (record_count == 0U))
    {
        return kKCT202_InvalidArgument;
    }

    packet[0] = KCT202_HEADER_HI;
    packet[1] = KCT202_HEADER_LO;
    packet[2] = (uint8_t)(KCT202_DEFAULT_ADDR >> 24);
    packet[3] = (uint8_t)(KCT202_DEFAULT_ADDR >> 16);
    packet[4] = (uint8_t)(KCT202_DEFAULT_ADDR >> 8);
    packet[5] = (uint8_t)KCT202_DEFAULT_ADDR;
    packet[6] = KCT202_PACKET_COMMAND;
    packet[7] = (uint8_t)(packet_length >> 8);
    packet[8] = (uint8_t)packet_length;
    packet[9] = KCT202_CMD_ENROLL;
    packet[10] = (uint8_t)(finger_id >> 8);
    packet[11] = (uint8_t)finger_id;
    packet[12] = record_count;
    packet[13] = (uint8_t)(flags >> 8);
    packet[14] = (uint8_t)flags;

    /* Sum packet ID, length bytes, and all command payload bytes. */
    checksum = KCT202_Checksum(&packet[6], 9U);
    packet[15] = (uint8_t)(checksum >> 8);
    packet[16] = (uint8_t)checksum;

    LPUART_WriteBlocking(uart, packet, sizeof(packet));
    return kKCT202_Ok;
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
    size_t total_length;
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

    /* The first 9 bytes are header/address/packet ID/length; length bytes follow. */
    status = KCT202_ReadExact(uart, &packet[9], length, timeout_ms);
    if (status != kKCT202_Ok) return status;
    total_length = KCT202_FIXED_BYTES + (size_t)length;

    checksum = KCT202_Checksum(&packet[6], (size_t)length + 1U);
    received_checksum = (uint16_t)(((uint16_t)packet[7U + length] << 8) |
                                   packet[8U + length]);

    if (checksum != received_checksum)
    {
        return kKCT202_BadChecksum;
    }

    /* Response packet ID should be 0x07; payload starts at byte 9. */
    if (packet[6] != 0x07U)
    {
        return kKCT202_BadHeader;
    }

    *confirm_code = packet[9];
    *params_len = (size_t)length - 3U;

    if (*params_len > 0U)
    {
        if ((params == NULL) || (capacity < *params_len))
        {
            *params_len = (size_t)length - 3U;
            return kKCT202_BufferTooSmall;
        }
        for (size_t i = 0U; i < *params_len; ++i)
        {
            params[i] = packet[10U + i];
        }
    }

    (void)total_length;
    return kKCT202_Ok;
}
