#ifndef HAND_ESPNOW_H
#define HAND_ESPNOW_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "hand_config.h"
#include "../common/espnow_protocol.h"

// 前向声明
class HandServo;
class HandFeedbackManager;

class HandESPNow {
public:
    HandESPNow();
    
    bool begin(HandServo* servoCtrl, HandFeedbackManager* feedbackMgr = nullptr);
    void update();
    
    // 发送响应
    bool sendStatus(ESPNowStatusCode status, const char* message = "");
    bool sendResponse(const ESPNowPacket& originalPacket, ESPNowStatusCode status, const char* message = "");
    bool sendRegistration();
    bool sendFeedbackStatus();  // 发送反馈状态
    
    // 配置和注册
    void setFeederId(uint8_t feederId);
    uint8_t getFeederId();
    void saveFeederId();                // 保存到EEPROM
    void loadFeederId();                // 从EEPROM加载
    
    // 状态查询
    bool isBrainOnline();
    unsigned long getLastCommand();

private:
    HandServo* servoController;
    HandFeedbackManager* feedbackManager;
    bool brainOnline;
    unsigned long lastCommandTime;
    unsigned long lastHeartbeatResponse;
    uint8_t feederId;
    bool registered;
    
    // 非阻塞注册延迟
    unsigned long registrationScheduledTime;
    bool registrationPending;
    
    // ESP8266 ESP-NOW回调函数
    static void onDataSent(uint8_t *mac_addr, uint8_t sendStatus);
    static void onDataReceived(uint8_t *mac_addr, uint8_t *data, uint8_t len);
    
    // 命令处理
    void handleCommand(const uint8_t *mac_addr, const ESPNowPacket& packet);
    void processServoAngle(const ESPNowPacket& packet);
    void processFeederAdvance(const ESPNowPacket& packet);
    void processStatusRequest(const ESPNowPacket& packet);
    void processHeartbeat(const ESPNowPacket& packet);
    void processBrainDiscovery(const ESPNowPacket& packet);
    void processRegistrationRequest(const ESPNowPacket& packet);
    void processCheckFeedback(const ESPNowPacket& packet);       // 处理反馈查询
    void processEnableFeedback(const ESPNowPacket& packet);      // 处理反馈启用/禁用
    
    // 内部辅助
    bool addBroadcastPeer();
    void checkBrainConnection();
    void scheduleRegistration();        // 安排延迟注册
    void processScheduledRegistration(); // 处理延迟注册

    // 推进后自动回缩相关变量
    bool feederRetractPending = false;
    unsigned long feederRetractTime = 0;
    uint16_t feederAdvanceAngle = 80; // 推进角度
    uint16_t feederRetractAngle = 0;  // 回缩角度
    unsigned long feederRetractDelay = 400; // ms, 推进后等待多久回缩
};

#endif // HAND_ESPNOW_H
