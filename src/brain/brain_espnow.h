#ifndef BRAIN_ESPNOW_H
#define BRAIN_ESPNOW_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "brain_config.h"
#include "../common/espnow_protocol.h"

// 前向声明
class FeederManager;

class BrainESPNow {
public:
    BrainESPNow();
    
    bool begin();
    void update();
    void setFeederManager(FeederManager* feederMgr) { feederManager = feederMgr; }
    
    // 发送命令
    bool sendCommand(uint8_t handId, const ESPNowPacket& packet);
    bool sendServoAngle(uint8_t handId, uint16_t angle, uint16_t minPulse = 500, uint16_t maxPulse = 2400);
    bool sendFeederAdvance(uint8_t handId, uint8_t feedLength);
    bool sendStatusRequest(uint8_t handId);
    
    // 广播和注册管理
    bool broadcastDiscovery();
    bool requestRegistration();
    void clearRegistrations();
    
    // 状态查询
    bool isHandOnline(uint8_t handId);
    unsigned long getLastResponse(uint8_t handId);
    uint8_t getRegisteredHandCount();
    void printRegistrationStatus();
    uint8_t getHandFeederId(uint8_t handId);
    
private:
    struct HandInfo {
        bool registered;
        bool online;
        unsigned long lastResponse;
        uint8_t macAddress[6];
        uint8_t feederId;  // 喂料器ID (0-49)
        
        HandInfo() : registered(false), online(false), lastResponse(0), feederId(255) {
            memset(macAddress, 0, 6);
        }
    };
    
    HandInfo hands[MAX_HANDS];
    uint32_t sequenceNumber;
    FeederManager* feederManager;
    
    // ESP-NOW回调
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len);
    
    // 内部处理
    void handleResponse(const uint8_t *mac_addr, const ESPNowResponse& response);
    void handleRegistration(const uint8_t *mac_addr, const ESPNowPacket& packet);
    void handleFeedbackStatus(const uint8_t *mac_addr, const ESPNowFeedbackPacket& feedbackPacket);
    uint8_t findHandByMac(const uint8_t *mac_addr);
    uint8_t findHandByFeeder(uint8_t feederId);
    bool addPeer(uint8_t handId);
    bool addBroadcastPeer();
    void checkHandsStatus();
};

#endif // BRAIN_ESPNOW_H
