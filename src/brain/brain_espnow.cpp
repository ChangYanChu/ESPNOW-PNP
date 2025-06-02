#include "brain_espnow.h"
#include "brain_feeder_manager.h"

// 静态实例指针用于回调
static BrainESPNow* brainInstance = nullptr;

// 广播MAC地址
uint8_t broadcast_mac[6] = BROADCAST_MAC;

BrainESPNow::BrainESPNow() : sequenceNumber(0), feederManager(nullptr) {
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
    
    // 清除所有注册并添加广播对等设备
    clearRegistrations();
    if (!addBroadcastPeer()) {
        Serial.println(F("Failed to add broadcast peer"));
        return false;
    }
    
    Serial.print(F("Brain MAC: "));
    Serial.println(WiFi.macAddress());
    Serial.println(F("ESP-NOW Brain initialized"));
    
    // 发送发现广播
    broadcastDiscovery();
    
    return true;
}

void BrainESPNow::update() {
    checkHandsStatus();
}

bool BrainESPNow::sendCommand(uint8_t handId, const ESPNowPacket& packet) {
    if (handId >= MAX_HANDS) {
        DEBUG_ESPNOW_PRINTF("Invalid handId: %d\n", handId);
        return false;
    }
    
    if (!hands[handId].registered) {
        DEBUG_ESPNOW_PRINTF("Hand %d not registered\n", handId);
        return false;
    }
    
    esp_err_t result = esp_now_send(hands[handId].macAddress, (uint8_t*)&packet, sizeof(packet));
    
    DEBUG_ESPNOW_PRINTF("Sending to hand %d (feeder %d), result: %s", 
                        handId, hands[handId].feederId, result == ESP_OK ? "OK" : "FAIL");
    if (result != ESP_OK) {
        DEBUG_ESPNOW_PRINTF(" (error code: %d)", result);
    }
    DEBUG_ESPNOW_PRINT("");
    
    #ifdef DEBUG_VERBOSE_ESPNOW
    DEBUG_ESPNOW_PRINTF("Packet: magic=0x%04X, cmd=%d, seq=%d\n", 
                        packet.magic, packet.commandType, packet.sequence);
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

uint8_t BrainESPNow::getRegisteredHandCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        if (hands[i].registered) {
            count++;
        }
    }
    return count;
}

void BrainESPNow::printRegistrationStatus() {
    Serial.println(F("=== Hand Registration Status ==="));
    uint8_t registeredCount = 0;
    
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        if (hands[i].registered) {
            registeredCount++;
            Serial.print(F("Hand "));
            Serial.print(i);
            Serial.print(F(" (Feeder "));
            Serial.print(hands[i].feederId);
            Serial.print(F("): "));
            Serial.print(hands[i].online ? "ONLINE" : "OFFLINE");
            Serial.print(F(", MAC: "));
            for (int j = 0; j < 6; j++) {
                if (hands[i].macAddress[j] < 16) Serial.print("0");
                Serial.print(hands[i].macAddress[j], HEX);
                if (j < 5) Serial.print(":");
            }
            Serial.println();
        }
    }
    
    Serial.print(F("Total registered hands: "));
    Serial.print(registeredCount);
    Serial.print(F("/"));
    Serial.println(MAX_HANDS);
    Serial.println(F("================================"));
}

// 静态回调函数
void BrainESPNow::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    #ifdef DEBUG_ESPNOW
    Serial.print(F("Send status: "));
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    #endif
}

void BrainESPNow::onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (!brainInstance) return;
    
    if (len == sizeof(ESPNowResponse)) {
        ESPNowResponse response;
        memcpy(&response, data, sizeof(response));
        brainInstance->handleResponse(mac_addr, response);
    } else if (len == sizeof(ESPNowFeedbackPacket)) {
        ESPNowFeedbackPacket feedbackPacket;
        memcpy(&feedbackPacket, data, sizeof(feedbackPacket));
        if (feedbackPacket.commandType == CMD_FEEDBACK_STATUS) {
            brainInstance->handleFeedbackStatus(mac_addr, feedbackPacket);
        }
    } else if (len == sizeof(ESPNowPacket)) {
        ESPNowPacket packet;
        memcpy(&packet, data, sizeof(packet));
        if (packet.commandType == CMD_HAND_REGISTER) {
            brainInstance->handleRegistration(mac_addr, packet);
        }
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
    
    memcpy(peerInfo.peer_addr, hands[handId].macAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    esp_err_t result = esp_now_add_peer(&peerInfo);
    
    #ifdef DEBUG_ESPNOW
    Serial.print(F("Adding peer for hand "));
    Serial.print(handId);
    Serial.print(F(", result: "));
    Serial.println(result == ESP_OK ? "OK" : "FAIL");
    if (result != ESP_OK) {
        Serial.print(F("ESP-NOW add peer error: "));
        Serial.println(result);
    }
    #endif
    
    return result == ESP_OK;
}

bool BrainESPNow::addBroadcastPeer() {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    
    memcpy(peerInfo.peer_addr, broadcast_mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    esp_err_t result = esp_now_add_peer(&peerInfo);
    return result == ESP_OK;
}

bool BrainESPNow::broadcastDiscovery() {
    ESPNowPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = ESPNOW_MAGIC;
    packet.handId = 0xFF; // 广播标识
    packet.commandType = CMD_BRAIN_DISCOVERY;
    packet.sequence = ++sequenceNumber;
    setPacketChecksum(&packet);
    
    esp_err_t result = esp_now_send(broadcast_mac, (uint8_t*)&packet, sizeof(packet));
    
    #ifdef DEBUG_ESPNOW
    Serial.println(F("Broadcasting discovery"));
    #endif
    
    return result == ESP_OK;
}

bool BrainESPNow::requestRegistration() {
    ESPNowPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = ESPNOW_MAGIC;
    packet.handId = 0xFF; // 广播标识
    packet.commandType = CMD_REQUEST_REGISTRATION;
    packet.sequence = ++sequenceNumber;
    setPacketChecksum(&packet);
    
    esp_err_t result = esp_now_send(broadcast_mac, (uint8_t*)&packet, sizeof(packet));
    
    #ifdef DEBUG_ESPNOW
    Serial.println(F("Requesting registration"));
    #endif
    
    return result == ESP_OK;
}

void BrainESPNow::clearRegistrations() {
    // 清除所有对等设备（除了广播）
    esp_now_deinit();
    esp_now_init();
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceived);
    
    // 重置所有手部信息
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        hands[i].registered = false;
        hands[i].online = false;
        hands[i].lastResponse = 0;
        hands[i].feederId = 255;
        memset(hands[i].macAddress, 0, 6);
    }
    
    Serial.println(F("All registrations cleared"));
}

void BrainESPNow::handleRegistration(const uint8_t *mac_addr, const ESPNowPacket& packet) {
    // 验证魔数和校验和
    if (packet.magic != ESPNOW_MAGIC || !verifyChecksum(&packet)) {
        return;
    }
    
    uint8_t feederId = packet.feederId;
    if (feederId >= TOTAL_FEEDERS) {
        Serial.print(F("Invalid feeder ID: "));
        Serial.println(feederId);
        return;
    }
    
    // 查找空闲的手部槽位
    uint8_t handId = MAX_HANDS;
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        if (!hands[i].registered) {
            handId = i;
            break;
        }
    }
    
    if (handId >= MAX_HANDS) {
        Serial.println(F("No free hand slots"));
        return;
    }
    
    // 检查是否已经注册了相同的MAC地址或喂料器ID
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        if (hands[i].registered && 
            (memcmp(hands[i].macAddress, mac_addr, 6) == 0 || hands[i].feederId == feederId)) {
            Serial.print(F("Hand already registered: MAC or feeder ID "));
            Serial.println(feederId);
            return;
        }
    }
    
    // 注册新的手部
    memcpy(hands[handId].macAddress, mac_addr, 6);
    hands[handId].feederId = feederId;
    hands[handId].lastResponse = millis();
    hands[handId].online = true;
    
    // 先尝试添加peer，成功后才设置registered标志
    if (addPeer(handId)) {
        hands[handId].registered = true;  // 设置注册标志
        Serial.print(F("Hand "));
        Serial.print(handId);
        Serial.print(F(" registered with feeder ID "));
        Serial.print(feederId);
        Serial.print(F(", MAC: "));
        for (int i = 0; i < 6; i++) {
            Serial.print(mac_addr[i], HEX);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
    } else {
        Serial.print(F("Failed to add peer for hand "));
        Serial.print(handId);
        Serial.print(F(" with feeder ID "));
        Serial.println(feederId);
        // 清理失败的注册
        memset(&hands[handId], 0, sizeof(hands[handId]));
    }
}

uint8_t BrainESPNow::findHandByFeeder(uint8_t feederId) {
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        if (hands[i].registered && hands[i].feederId == feederId) {
            return i;
        }
    }
    return MAX_HANDS; // 未找到
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

uint8_t BrainESPNow::getHandFeederId(uint8_t handId) {
    if (handId >= MAX_HANDS || !hands[handId].registered) {
        return 255; // 无效或未注册
    }
    return hands[handId].feederId;
}

void BrainESPNow::handleFeedbackStatus(const uint8_t *mac_addr, const ESPNowFeedbackPacket& feedbackPacket) {
    // 验证魔数
    if (feedbackPacket.magic != ESPNOW_MAGIC) {
        DEBUG_BRAIN_ESPNOW_PRINT("Invalid magic number in feedback packet");
        return;
    }
    
    // 验证校验和
    uint16_t receivedChecksum = feedbackPacket.checksum;
    ESPNowFeedbackPacket tempPacket = feedbackPacket;
    tempPacket.checksum = 0;
    
    uint16_t calculatedChecksum = 0;
    uint8_t* data = (uint8_t*)&tempPacket;
    for (size_t i = 0; i < sizeof(tempPacket) - sizeof(tempPacket.checksum); i++) {
        calculatedChecksum += data[i];
    }
    
    if (receivedChecksum != calculatedChecksum) {
        DEBUG_BRAIN_ESPNOW_PRINT("Feedback packet checksum verification failed");
        return;
    }
    
    uint8_t handId = findHandByMac(mac_addr);
    if (handId >= MAX_HANDS) {
        DEBUG_BRAIN_ESPNOW_PRINT("Feedback packet from unknown hand");
        return;
    }
    
    // 更新hand状态
    hands[handId].online = true;
    hands[handId].lastResponse = millis();
    
    DEBUG_BRAIN_ESPNOW_PRINTF("Feedback status from hand %d (feeder %d) - Tape: %s, Enabled: %s, Manual: %s, Errors: %d\n",
                              handId, feedbackPacket.feederId,
                              feedbackPacket.tapeLoaded ? "LOADED" : "NOT_LOADED",
                              feedbackPacket.feedbackEnabled ? "YES" : "NO",
                              feedbackPacket.manualFeedDetected ? "YES" : "NO",
                              feedbackPacket.errorCount);
    
    // 将反馈状态传递给FeederManager
    if (feederManager) {
        feederManager->handleFeedbackStatus(handId, feedbackPacket);
    } else {
        DEBUG_BRAIN_ESPNOW_PRINT("Warning: FeederManager not set - cannot process feedback status");
    }
}
