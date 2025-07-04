#ifndef BRAIN_WEB_H
#define BRAIN_WEB_H

#include <WiFi.h>

// Web服务器初始化
void web_setup();

// Web服务器更新函数（定期推送状态）
void web_update();

// WebSocket事件推送函数（轻量级实现）
void notifyFeederStatusChange(uint8_t feederId, uint8_t status);
void notifyCommandReceived(uint8_t feederId, uint8_t feedLength);
void notifyCommandCompleted(uint8_t feederId, bool success, const char* message);
void notifyHandOnline(uint8_t feederId);
void notifyHandOffline(uint8_t feederId);

#endif // BRAIN_WEB_H
