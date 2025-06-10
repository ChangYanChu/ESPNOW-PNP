#ifndef BRAIN_ESPNOW_H
#define BRAIN_ESPNOW_H

#include <Arduino.h>
void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast);
void espnow_setup();
void esp_update();
void processReceivedResponse();
void checkCommandTimeout();
void sendHeartbeat();
int getOnlineHandCount();
bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength);
#endif // BRAIN_ESPNOW_H
