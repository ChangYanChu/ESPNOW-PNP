#ifndef HAND_ESPNOW_H
#define HAND_ESPNOW_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "hand_config.h"
#include "../common/espnow_protocol.h"

// 前向声明
class HandServo;

class HandESPNow {
public:
    HandESPNow();
    
    bool begin(HandServo* servoCtrl);
    void update();
    
    // 发送响应
    bool sendStatus(ESPNowStatusCode status, const char* message = "");
    bool sendResponse(const ESPNowPacket& originalPacket, ESPNowStatusCode status, const char* message = "");
    
    // 状态查询
    bool isBrainOnline();
    unsigned long getLastCommand();

private:
    HandServo* servoController;
    bool brainOnline;
    unsigned long lastCommandTime;
    unsigned long lastHeartbeatResponse;
    
    // ESP8266 ESP-NOW回调函数
    static void onDataSent(uint8_t *mac_addr, uint8_t sendStatus);
    static void onDataReceived(uint8_t *mac_addr, uint8_t *data, uint8_t len);
    
    // 命令处理
    void handleCommand(const uint8_t *mac_addr, const ESPNowPacket& packet);
    void processServoAngle(const ESPNowPacket& packet);
    void processFeederAdvance(const ESPNowPacket& packet);
    void processStatusRequest(const ESPNowPacket& packet);
    void processHeartbeat(const ESPNowPacket& packet);
    
    // 内部辅助
    bool addBrainPeer();
    void checkBrainConnection();
};

#endif // HAND_ESPNOW_H
