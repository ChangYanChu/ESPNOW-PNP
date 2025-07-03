#ifndef HAND_ESPNOW_H
#define HAND_ESPNOW_H

#include <Arduino.h>

// 核心ESP-NOW函数
void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast);
void espnow_setup();
void esp_update();

// 命令处理函数
void processReceivedCommand();
void schedulePendingResponse(uint8_t feederID, uint8_t status, const char *message);
void processPendingResponse();

// WiFi扫描函数
int scanForSSIDChannel(const char* targetSSID);

#endif // HAND_ESPNOW_H
