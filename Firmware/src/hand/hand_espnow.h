#ifndef BRAIN_ESPNOW_H
#define BRAIN_ESPNOW_H

#include <Arduino.h>
void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast);
void espnow_setup();
void esp_update();
void processReceivedCommand();
void handleFeederAdvanceCommand(uint8_t feederID, uint8_t feedLength);
void handleHeartbeatCommand();
void schedulePendingResponse(uint8_t feederID, uint8_t status, const char *message);
void processPendingResponse();
void sendSuccessResponse(uint8_t feederID, const char *message);
void sendErrorResponse(uint8_t feederID, uint8_t errorCode, const char *message);

// 远程配置相关函数
void handleSetFeederIDCommand(uint8_t newFeederID);

// 新增的信道发现功能
bool scanForBrainChannel();
bool tryChannelDiscovery(uint8_t channel);
void sendDiscoveryRequest();
void handleDiscoveryResponse();
bool waitForDiscoveryResponse(uint32_t timeoutMs);

#endif // BRAIN_ESPNOW_H
