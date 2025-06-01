#include "brain_espnow.h"

// 静态实例指针用于回调
static BrainESPNow* brainInstance = nullptr;

// 手部MAC地址配置 (使用实际ESP01S的MAC地址)
uint8_t hand_mac_addresses[MAX_HANDS][6] = {
    {0xC4, 0xD8, 0xD5, 0x00, 0x04, 0x61}, // Hand 0 - 实际ESP01S MAC
    {0x24, 0x0A, 0xC4, 0x00, 0x00, 0x01}, // Hand 1 - 需要修改为实际MAC
    {0x24, 0x0A, 0xC4, 0x00, 0x00, 0x02}, // Hand 2 - 需要修改为实际MAC
    {0x24, 0x0A, 0xC4, 0x00, 0x00, 0x03}, // Hand 3 - 需要修改为实际MAC
    {0x24, 0x0A, 0xC4, 0x00, 0x00, 0x04}, // Hand 4 - 需要修改为实际MAC
    {0x24, 0x0A, 0xC4, 0x00, 0x00, 0x05}, // Hand 5 - 需要修改为实际MAC
    {0x24, 0x0A, 0xC4, 0x00, 0x00, 0x06}, // Hand 6 - 需要修改为实际MAC
    {0x24, 0x0A, 0xC4, 0x00, 0x00, 0x07}  // Hand 7 - 需要修改为实际MAC
};

BrainESPNow::BrainESPNow() : sequenceNumber(0) {
    brainInstance = this;
}

bool BrainESPNow::begin() {
    // 设置WiFi模式
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    // 初始化ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println(F("ESP-NOW init failed"));
        return false;
    }
    
    // 注册回调函数
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceived);
    
    // 添加所有手部为对等设备
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        if (!addPeer(i)) {
            Serial.print(F("Failed to add hand "));
            Serial.println(i);
        }
    }
    
    Serial.print(F("Brain MAC: "));
    Serial.println(WiFi.macAddress());
    Serial.println(F("ESP-NOW Brain initialized"));
    
    return true;
}

void BrainESPNow::update() {
    checkHandsStatus();
}

bool BrainESPNow::sendCommand(uint8_t handId, const ESPNowPacket& packet) {
    if (handId >= MAX_HANDS) {
        return false;
    }
    
    esp_err_t result = esp_now_send(hand_mac_addresses[handId], (uint8_t*)&packet, sizeof(packet));
    
    #ifdef DEBUG_ESPNOW
    Serial.print(F("Sending to hand "));
    Serial.print(handId);
    Serial.print(F(", result: "));
    Serial.println(result == ESP_OK ? "OK" : "FAIL");
    #endif
    
    return result == ESP_OK;
}

bool BrainESPNow::sendServoAngle(uint8_t handId, uint16_t angle, uint16_t minPulse, uint16_t maxPulse) {
    ESPNowPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = ESPNOW_MAGIC;
    packet.handId = handId;
    packet.commandType = CMD_SERVO_SET_ANGLE;
    packet.angle = angle;
    packet.pulseMin = minPulse;
    packet.pulseMax = maxPulse;
    packet.sequence = ++sequenceNumber;
    setPacketChecksum(&packet);
    
    return sendCommand(handId, packet);
}

bool BrainESPNow::sendFeederAdvance(uint8_t handId, uint8_t feedLength) {
    ESPNowPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = ESPNOW_MAGIC;
    packet.handId = handId;
    packet.commandType = CMD_FEEDER_ADVANCE;
    packet.feedLength = feedLength;
    packet.sequence = ++sequenceNumber;
    setPacketChecksum(&packet);
    
    return sendCommand(handId, packet);
}

bool BrainESPNow::sendStatusRequest(uint8_t handId) {
    ESPNowPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = ESPNOW_MAGIC;
    packet.handId = handId;
    packet.commandType = CMD_STATUS_REQUEST;
    packet.sequence = ++sequenceNumber;
    setPacketChecksum(&packet);
    
    return sendCommand(handId, packet);
}

bool BrainESPNow::isHandOnline(uint8_t handId) {
    if (handId >= MAX_HANDS) return false;
    return hands[handId].online;
}

unsigned long BrainESPNow::getLastResponse(uint8_t handId) {
    if (handId >= MAX_HANDS) return 0;
    return hands[handId].lastResponse;
}

// 静态回调函数
void BrainESPNow::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    #ifdef DEBUG_ESPNOW
    Serial.print(F("Send status: "));
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    #endif
}

void BrainESPNow::onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (brainInstance && len == sizeof(ESPNowResponse)) {
        ESPNowResponse response;
        memcpy(&response, data, sizeof(response));
        brainInstance->handleResponse(mac_addr, response);
    }
}

void BrainESPNow::handleResponse(const uint8_t *mac_addr, const ESPNowResponse& response) {
    // 验证魔数
    if (response.magic != ESPNOW_MAGIC) {
        return;
    }
    
    uint8_t handId = findHandByMac(mac_addr);
    if (handId < MAX_HANDS) {
        hands[handId].online = true;
        hands[handId].lastResponse = millis();
        
        #ifdef DEBUG_ESPNOW
        Serial.print(F("Response from hand "));
        Serial.print(handId);
        Serial.print(F(", status: "));
        Serial.print(response.status);
        Serial.print(F(", message: "));
        Serial.println(response.message);
        #endif
    }
}

uint8_t BrainESPNow::findHandByMac(const uint8_t *mac_addr) {
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        if (memcmp(hands[i].macAddress, mac_addr, 6) == 0) {
            return i;
        }
    }
    return MAX_HANDS; // 未找到
}

bool BrainESPNow::addPeer(uint8_t handId) {
    if (handId >= MAX_HANDS) return false;
    
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    
    memcpy(peerInfo.peer_addr, hand_mac_addresses[handId], 6);
    memcpy(hands[handId].macAddress, hand_mac_addresses[handId], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK) {
        hands[handId].registered = true;
        return true;
    }
    
    return false;
}

void BrainESPNow::checkHandsStatus() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 1000) return; // 每秒检查一次
    
    lastCheck = millis();
    
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        if (hands[i].registered) {
            if (millis() - hands[i].lastResponse > HAND_OFFLINE_TIMEOUT_MS) {
                if (hands[i].online) {
                    hands[i].online = false;
                    Serial.print(F("Hand "));
                    Serial.print(i);
                    Serial.println(F(" went offline"));
                }
            }
        }
    }
}
