#ifndef ESPNOW_PROTOCOL_H
#define ESPNOW_PROTOCOL_H

#include <Arduino.h>

// ESP-NOW command types
#define CMD_FEEDER_ADVANCE 0x01
#define CMD_FEEDER_STATUS  0x02

// ESP-NOW packet structure
typedef struct {
    uint8_t commandType; // Command type
    uint8_t feederId;    // Feeder ID
    uint8_t feedLength;  // Feed length
    uint8_t reserved[5]; // Reserved for future use
} ESPNowPacket;

// Function prototypes
void sendESPNowPacket(const uint8_t *address, ESPNowPacket *packet);
void onESPNowDataReceived(uint8_t *address, uint8_t *data, uint8_t len);

#endif // ESPNOW_PROTOCOL_H