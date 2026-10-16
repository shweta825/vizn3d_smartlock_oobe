/*
 * KCT202 fingerprint-management commands.
 * Protocol directives verified against Seeed-Studio/Seeed_Arduino_KCT202/src/KCT202.h
 * and src/KCT202.cpp. Hardware behavior still requires validation on the exact sensor.
 */
#include "kct202_manager.h"

#define KCT202_HEADER_HI      (0xEFU)
#define KCT202_HEADER_LO      (0x01U)
#define KCT202_COMMAND_PACKET (0x01U)
#define KCT202_DEFAULT_ADDR  (0xFFFFFFFFUL)
#define KCT202_CMD_VERIFY    (0x32U)
#define KCT202_CMD_DELETE    (0x0CU)
#define KCT202_CMD_CLEAR_ALL (0x0DU)
#define KCT202_CMD_READ_COUNT (0x1DU)
#define KCT202_CMD_READ_INDEX (0x1FU)

static uint16_t KCT202_ManagerChecksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0U;
    size_t i;
    for (i = 0U; i < len; ++i) sum += data[i];
    return (uint16_t)(sum & 0xFFFFU);
}

/* Payload includes directive. Length field is payload bytes + two checksum bytes. */
static kct202_status_t KCT202_SendCommand(LPUART_Type *uart,
                                          const uint8_t *payload,
                                          size_t payload_len)
{
    uint8_t packet[32];
    size_t packet_len;
    uint16_t length_field;
    uint16_t checksum;
    size_t i;

    if ((uart == NULL) || (payload == NULL) || (payload_len == 0U) ||
        (payload_len > (sizeof(packet) - 11U)))
        return kKCT202_InvalidArgument;

    length_field = (uint16_t)(payload_len + 2U);
    packet[0] = KCT202_HEADER_HI;
    packet[1] = KCT202_HEADER_LO;
    packet[2] = (uint8_t)(KCT202_DEFAULT_ADDR >> 24);
    packet[3] = (uint8_t)(KCT202_DEFAULT_ADDR >> 16);
    packet[4] = (uint8_t)(KCT202_DEFAULT_ADDR >> 8);
    packet[5] = (uint8_t)KCT202_DEFAULT_ADDR;
    packet[6] = KCT202_COMMAND_PACKET;
    packet[7] = (uint8_t)(length_field >> 8);
    packet[8] = (uint8_t)length_field;
    for (i = 0U; i < payload_len; ++i) packet[9U + i] = payload[i];

    checksum = KCT202_ManagerChecksum(&packet[6], payload_len + 3U);
    packet[9U + payload_len] = (uint8_t)(checksum >> 8);
    packet[10U + payload_len] = (uint8_t)checksum;
    packet_len = 11U + payload_len;

    LPUART_WriteBlocking(uart, packet, packet_len);
    return kKCT202_Ok;
}

kct202_status_t KCT202_AddFingerprintStart(LPUART_Type *uart,
                                           uint16_t finger_id,
                                           uint8_t captures,
                                           uint16_t flags)
{
    /* KCT202 enrollment directive 0x31, ID, capture count, control flags. */
    return KCT202_EnrollStart(uart, finger_id, captures, flags);
}

kct202_status_t KCT202_DeleteFingerprintStart(LPUART_Type *uart,
                                              uint16_t finger_id,
                                              uint8_t count)
{
    uint8_t payload[5];
    if ((count == 0U) || (finger_id > (uint16_t)(0xFFFFU - (uint16_t)(count - 1U))))
        return kKCT202_InvalidArgument;
    payload[0] = KCT202_CMD_DELETE;
    payload[1] = (uint8_t)(finger_id >> 8);
    payload[2] = (uint8_t)finger_id;
    payload[3] = 0U;
    payload[4] = count;
    return KCT202_SendCommand(uart, payload, sizeof(payload));
}

kct202_status_t KCT202_VerifyFingerprintStart(LPUART_Type *uart,
                                              uint16_t finger_id,
                                              uint16_t param)
{
    uint8_t payload[6];
    payload[0] = KCT202_CMD_VERIFY;
    payload[1] = 0U; /* fixed security-level field per Seeed library */
    payload[2] = (uint8_t)(finger_id >> 8);
    payload[3] = (uint8_t)finger_id;
    payload[4] = (uint8_t)(param >> 8);
    payload[5] = (uint8_t)param;
    return KCT202_SendCommand(uart, payload, sizeof(payload));
}

kct202_status_t KCT202_ReadFingerprintCountStart(LPUART_Type *uart)
{
    const uint8_t payload[] = { KCT202_CMD_READ_COUNT };
    return KCT202_SendCommand(uart, payload, sizeof(payload));
}

kct202_status_t KCT202_ReadFingerprintIndexStart(LPUART_Type *uart, uint8_t page)
{
    uint8_t payload[2];
    if (page > 3U) return kKCT202_InvalidArgument;
    payload[0] = KCT202_CMD_READ_INDEX;
    payload[1] = page;
    return KCT202_SendCommand(uart, payload, sizeof(payload));
}

kct202_status_t KCT202_DeleteAllFingerprintsStart(LPUART_Type *uart)
{
    const uint8_t payload[] = { KCT202_CMD_CLEAR_ALL };
    return KCT202_SendCommand(uart, payload, sizeof(payload));
}
