#ifndef BRAIN_ESPNOW_H
#define BRAIN_ESPNOW_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "brain_config.h"
#include "../common/espnow_protocol.h"

class BrainESPNow {
public:
    BrainESPNow();
    
    bool begin();
    void update();
    
    // 发送命令
    bool sendCommand(uint8_t handId, const ESPNowPacket& packet);
    bool sendServoAngle(uint8_t handId, uint16_t angle, uint16_t minPulse = 500, uint16_t maxPulse = 2400);
    bool sendFeederAdvance(uint8_t handId, uint8_t feedLength);
    bool sendStatusRequest(uint8_t handId);
    
    // 状态查询
    bool isHandOnline(uint8_t handId);
    unsigned long getLastResponse(uint8_t handId);
    
private:
    struct HandInfo {
        bool registered;
        bool online;
        unsigned long lastResponse;
        uint8_t macAddress[6];
        
        HandInfo() : registered(false), online(false), lastResponse(0) {
            memset(macAddress, 0, 6);
        }
    };
    
    HandInfo hands[MAX_HANDS];
    uint32_t sequenceNumber;
    
    // ESP-NOW回调
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len);
    
    // 内部处理
    void handleResponse(const uint8_t *mac_addr, const ESPNowResponse& response);
    uint8_t findHandByMac(const uint8_t *mac_addr);
    bool addPeer(uint8_t handId);
    void checkHandsStatus();
};

#endif // BRAIN_ESPNOW_H
