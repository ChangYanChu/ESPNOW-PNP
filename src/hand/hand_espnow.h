#ifndef BRAIN_ESPNOW_H
#define BRAIN_ESPNOW_H

#include <Arduino.h>
void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast);
void espnow_setup();
void esp_update();
void processReceivedCommand();
void handleFeederAdvanceCommand(uint8_t feederID, uint8_t feedLength);
void sendSuccessResponse(uint8_t feederID, const char *message);
void sendErrorResponse(uint8_t feederID, uint8_t errorCode, const char *message);
#endif // BRAIN_ESPNOW_H
