#include "brain_feeder_manager.h"
#include "brain_espnow.h"

FeederManager::FeederManager() : espnowManager(nullptr), systemEnabled(false), lastHeartbeat(0) {
    // 初始化所有虚拟喂料器
    for (uint8_t i = 0; i < TOTAL_FEEDERS; i++) {
        feeders[i].feederId = i;
        feeders[i].handId = feederIdToHandId(i);
    }
}

void FeederManager::begin(BrainESPNow* espnowMgr) {
    espnowManager = espnowMgr;
    
    // 发送初始状态查询给所有手部
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        espnowManager->sendStatusRequest(i);
        delay(50); // 避免同时发送太多数据包
    }
    
    Serial.println(F("FeederManager initialized"));
}

void FeederManager::update() {
    checkHandsOnlineStatus();
    
    // 定期发送心跳包
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
        sendHeartbeat();
        lastHeartbeat = millis();
    }
}

bool FeederManager::enableSystem(bool enable) {
    systemEnabled = enable;
    
    // 向所有在线的手部发送启用/禁用命令
    bool allSuccess = true;
    for (uint8_t i = 0; i < TOTAL_FEEDERS; i++) {
        if (feeders[i].online) {
            feeders[i].enabled = enable;
            // 这里可以发送特定的启用/禁用命令到手部
            // 目前通过系统状态标志控制
        }
    }
    
    #ifdef DEBUG_BRAIN
    Serial.print(F("System "));
    Serial.println(enable ? F("enabled") : F("disabled"));
    #endif
    
    return allSuccess;
}

bool FeederManager::advanceFeeder(uint8_t feederId, uint8_t length) {
    if (feederId >= TOTAL_FEEDERS) return false;
    if (!systemEnabled) return false;
    if (!feeders[feederId].online) return false;
    
    uint8_t handId = feeders[feederId].handId;
    return espnowManager->sendFeederAdvance(handId, length);
}

bool FeederManager::retractFeeder(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return false;
    if (!systemEnabled) return false;
    if (!feeders[feederId].online) return false;
    
    uint8_t handId = feeders[feederId].handId;
    
    // 发送回缩角度命令
    return espnowManager->sendServoAngle(handId, feeders[feederId].retractAngle,
                                        feeders[feederId].minPulse, feeders[feederId].maxPulse);
}

bool FeederManager::setServoAngle(uint8_t feederId, uint16_t angle) {
    if (feederId >= TOTAL_FEEDERS) return false;
    if (!systemEnabled) return false;
    if (!feeders[feederId].online) return false;
    
    uint8_t handId = feeders[feederId].handId;
    return espnowManager->sendServoAngle(handId, angle,
                                        feeders[feederId].minPulse, feeders[feederId].maxPulse);
}

bool FeederManager::requestFeederStatus(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return false;
    
    uint8_t handId = feeders[feederId].handId;
    return espnowManager->sendStatusRequest(handId);
}

bool FeederManager::isFeederOnline(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return false;
    return feeders[feederId].online;
}

String FeederManager::getFeederStatus(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) {
        return F("Invalid feeder ID");
    }
    
    String status = F("Feeder ");
    status += feederId;
    status += F(" (Hand ");
    status += feeders[feederId].handId;
    status += F("): ");
    
    if (!feeders[feederId].online) {
        status += F("OFFLINE");
    } else if (!feeders[feederId].enabled) {
        status += F("DISABLED");
    } else {
        status += F("ONLINE");
    }
    
    status += F(", Last response: ");
    status += (millis() - feeders[feederId].lastResponse);
    status += F("ms ago");
    
    return status;
}

String FeederManager::getAllHandsStatus() {
    String status = F("System: ");
    status += systemEnabled ? F("ENABLED") : F("DISABLED");
    status += F("\nHands status:\n");
    
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        status += F("Hand ");
        status += i;
        status += F(": ");
        
        if (espnowManager->isHandOnline(i)) {
            status += F("ONLINE (");
            status += (millis() - espnowManager->getLastResponse(i));
            status += F("ms ago)");
        } else {
            status += F("OFFLINE");
        }
        
        if (i < MAX_HANDS - 1) status += F("\n");
    }
    
    return status;
}

bool FeederManager::updateFeederConfig(uint8_t feederId, uint16_t fullAngle, uint16_t halfAngle, 
                                      uint16_t retractAngle, uint8_t feedLen, uint16_t settleTime,
                                      uint16_t minPulse, uint16_t maxPulse) {
    if (feederId >= TOTAL_FEEDERS) return false;
    
    // 更新本地配置
    feeders[feederId].fullAdvanceAngle = fullAngle;
    feeders[feederId].halfAdvanceAngle = halfAngle;
    feeders[feederId].retractAngle = retractAngle;
    feeders[feederId].feedLength = feedLen;
    feeders[feederId].settleTime = settleTime;
    feeders[feederId].minPulse = minPulse;
    feeders[feederId].maxPulse = maxPulse;
    
    #ifdef DEBUG_BRAIN
    Serial.print(F("Updated config for feeder "));
    Serial.print(feederId);
    Serial.print(F(" (Hand "));
    Serial.print(feeders[feederId].handId);
    Serial.println(F(")"));
    #endif
    
    // TODO: 如果需要，可以将配置发送到对应的手部
    
    return true;
}

void FeederManager::handleResponse(uint8_t handId, const ESPNowResponse& response) {
    // 更新对应喂料器的状态
    for (uint8_t i = 0; i < TOTAL_FEEDERS; i++) {
        if (feeders[i].handId == handId) {
            feeders[i].online = true;
            feeders[i].lastResponse = millis();
            
            #ifdef DEBUG_BRAIN
            Serial.print(F("Response from feeder "));
            Serial.print(i);
            Serial.print(F(" (Hand "));
            Serial.print(handId);
            Serial.print(F("): "));
            Serial.println(response.message);
            #endif
            
            break;
        }
    }
}

uint8_t FeederManager::feederIdToHandId(uint8_t feederId) {
    // 简单映射：每个手部控制一个喂料器
    return feederId % MAX_HANDS;
}

bool FeederManager::sendCommand(uint8_t feederId, ESPNowCommandType cmd, uint16_t angle, 
                               uint8_t feedLength, uint16_t pulseMin, uint16_t pulseMax) {
    if (feederId >= TOTAL_FEEDERS) return false;
    
    ESPNowPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = ESPNOW_MAGIC;
    packet.handId = feeders[feederId].handId;
    packet.commandType = cmd;
    packet.angle = angle;
    packet.pulseMin = pulseMin;
    packet.pulseMax = pulseMax;
    packet.feedLength = feedLength;
    packet.sequence = millis(); // 使用时间戳作为序列号
    setPacketChecksum(&packet);
    
    return espnowManager->sendCommand(feeders[feederId].handId, packet);
}

void FeederManager::checkHandsOnlineStatus() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 2000) return; // 每2秒检查一次
    
    lastCheck = millis();
    
    for (uint8_t i = 0; i < TOTAL_FEEDERS; i++) {
        bool handOnline = espnowManager->isHandOnline(feeders[i].handId);
        
        if (feeders[i].online != handOnline) {
            feeders[i].online = handOnline;
            
            #ifdef DEBUG_BRAIN
            Serial.print(F("Feeder "));
            Serial.print(i);
            Serial.print(F(" (Hand "));
            Serial.print(feeders[i].handId);
            Serial.print(F(") is now "));
            Serial.println(handOnline ? F("ONLINE") : F("OFFLINE"));
            #endif
        }
    }
}

void FeederManager::sendHeartbeat() {
    // 向所有手部发送心跳包
    for (uint8_t i = 0; i < MAX_HANDS; i++) {
        ESPNowPacket packet;
        memset(&packet, 0, sizeof(packet));
        
        packet.magic = ESPNOW_MAGIC;
        packet.handId = i;
        packet.commandType = CMD_HEARTBEAT;
        packet.sequence = millis();
        setPacketChecksum(&packet);
        
        espnowManager->sendCommand(i, packet);
        delay(10); // 避免同时发送
    }
    
    #ifdef DEBUG_BRAIN
    Serial.println(F("Heartbeat sent to all hands"));
    #endif
}
