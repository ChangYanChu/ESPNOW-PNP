#ifndef BRAIN_ESPNOW_H
#define BRAIN_ESPNOW_H

#include <Arduino.h>
#include "gcode.h"
void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast);
void espnow_setup();
void esp_update();
void processReceivedResponse();
void checkCommandTimeout();
void sendHeartbeat();
int getOnlineHandCount();
bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength, uint32_t timeoutMs = 0);

void initFeederStatus();

// 添加按设备管理的状态结构
struct FeederStatus {
    bool waitingForResponse;
    uint32_t commandSentTime;
    uint32_t timeoutMs;
};

extern FeederStatus feederStatusArray[NUMBER_OF_FEEDER];
#endif // BRAIN_ESPNOW_H
