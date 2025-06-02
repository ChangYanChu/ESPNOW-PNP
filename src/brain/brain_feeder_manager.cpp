#include "brain_feeder_manager.h"
#include "brain_espnow.h"

FeederManager::FeederManager() : espnowManager(nullptr), systemEnabled(false), lastHeartbeat(0) {
    // 初始化所有虚拟喂料器
    for (uint8_t i = 0; i < TOTAL_FEEDERS; i++) {
        feeders[i].feederId = i;
        feeders[i].handId = MAX_HANDS; // 未分配状态
        feeders[i].online = false;
    }
}

void FeederManager::begin(BrainESPNow* espnowMgr) {
    espnowManager = espnowMgr;
    
    // 启动时清除所有注册并请求重新注册
    DEBUG_FEEDER_MANAGER_PRINT("Clearing all hand registrations...");
    espnowManager->clearRegistrations();
    
    // 等待一下让系统稳定
    delay(500);
    
    // 发送发现广播
    DEBUG_FEEDER_MANAGER_PRINT("Broadcasting discovery to all hands...");
    espnowManager->broadcastDiscovery();
    
    DEBUG_FEEDER_MANAGER_PRINT("FeederManager initialized - waiting for hand registrations");
}

void FeederManager::update() {
    updateFeederMappings();
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
    DEBUG_FEEDER_MANAGER_PRINTF("advanceFeeder called - feederId: %d, length: %d\n", feederId, length);
    
    if (feederId >= TOTAL_FEEDERS) {
        DEBUG_FEEDER_MANAGER_PRINT("feederId >= TOTAL_FEEDERS");
        return false;
    }
    if (!systemEnabled) {
        DEBUG_FEEDER_MANAGER_PRINT("System not enabled");
        return false;
    }
    if (!feeders[feederId].online) {
        DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d not online. HandId: %d, Online: %d\n", 
                                   feederId, feeders[feederId].handId, feeders[feederId].online);
        return false;
    }
    
    uint8_t handId = feeders[feederId].handId;
    if (handId >= MAX_HANDS) {
        DEBUG_FEEDER_MANAGER_PRINTF("Invalid handId: %d >= %d\n", handId, MAX_HANDS);
        return false;
    }
    
    DEBUG_FEEDER_MANAGER_PRINTF("Sending to handId: %d\n", handId);
    return espnowManager->sendFeederAdvance(handId, length);
}

bool FeederManager::retractFeeder(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return false;
    if (!systemEnabled) return false;
    if (!feeders[feederId].online) return false;
    
    uint8_t handId = feeders[feederId].handId;
    if (handId >= MAX_HANDS) return false; // 未分配
    
    // 发送回缩角度命令
    return espnowManager->sendServoAngle(handId, feeders[feederId].retractAngle,
                                        feeders[feederId].minPulse, feeders[feederId].maxPulse);
}

bool FeederManager::setServoAngle(uint8_t feederId, uint16_t angle) {
    if (feederId >= TOTAL_FEEDERS) return false;
    if (!systemEnabled) return false;
    if (!feeders[feederId].online) return false;
    
    uint8_t handId = feeders[feederId].handId;
    if (handId >= MAX_HANDS) return false; // 未分配
    
    return espnowManager->sendServoAngle(handId, angle,
                                        feeders[feederId].minPulse, feeders[feederId].maxPulse);
}

bool FeederManager::requestFeederStatus(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return false;
    
    uint8_t handId = feeders[feederId].handId;
    if (handId >= MAX_HANDS) return false; // 未分配
    
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
    
    DEBUG_FEEDER_MANAGER_PRINTF("Updated config for feeder %d (Hand %d)\n", 
                                feederId, feeders[feederId].handId);
    
    // TODO: 如果需要，可以将配置发送到对应的手部
    
    return true;
}

void FeederManager::handleResponse(uint8_t handId, const ESPNowResponse& response) {
    // 更新对应喂料器的状态
    for (uint8_t i = 0; i < TOTAL_FEEDERS; i++) {
        if (feeders[i].handId == handId) {
            feeders[i].online = true;
            feeders[i].lastResponse = millis();
            
            DEBUG_FEEDER_MANAGER_PRINTF("Response from feeder %d (Hand %d): %s\n", 
                                       i, handId, response.message);
            
            break;
        }
    }
}

void FeederManager::updateFeederMappings() {
    static unsigned long lastDebug = 0;
    bool debugOutput = (millis() - lastDebug > 10000); // 每10秒输出一次调试信息
    
    if (debugOutput) {
        DEBUG_FEEDER_MANAGER_PRINT("updateFeederMappings called");
        lastDebug = millis();
    }
    
    // 检查ESPNow管理器中的注册状态并更新喂料器映射
    for (uint8_t handId = 0; handId < MAX_HANDS; handId++) {
        if (espnowManager->isHandOnline(handId)) {
            uint8_t feederId = espnowManager->getHandFeederId(handId);
            if (debugOutput) {
                DEBUG_FEEDER_MANAGER_PRINTF("Hand %d online, feederId: %d\n", handId, feederId);
            }
            
            if (feederId < TOTAL_FEEDERS) {
                if (feeders[feederId].handId != handId) {
                    feeders[feederId].handId = handId;
                    feeders[feederId].online = true;
                    DEBUG_FEEDER_MANAGER_PRINTF("Mapped feeder %d to hand %d\n", feederId, handId);
                } else if (debugOutput) {
                    DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d already mapped to hand %d, online: %d\n", 
                                               feederId, handId, feeders[feederId].online);
                }
            }
        }
    }
    
    // 检查离线的手部并更新状态
    for (uint8_t i = 0; i < TOTAL_FEEDERS; i++) {
        if (feeders[i].handId < MAX_HANDS && !espnowManager->isHandOnline(feeders[i].handId)) {
            if (feeders[i].online && debugOutput) {
                DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d going offline\n", i);
            }
            feeders[i].online = false;
        }
    }
}

uint8_t FeederManager::findFeederByHandId(uint8_t handId) {
    for (uint8_t i = 0; i < TOTAL_FEEDERS; i++) {
        if (feeders[i].handId == handId) {
            return i;
        }
    }
    return TOTAL_FEEDERS; // 未找到
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
            
            DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d (Hand %d) is now %s\n", 
                                       i, feeders[i].handId, handOnline ? "ONLINE" : "OFFLINE");
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
    
    DEBUG_FEEDER_MANAGER_PRINT("Heartbeat sent to all hands");
}

// 反馈系统操作实现
bool FeederManager::checkFeedback(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS || !feeders[feederId].online) {
        DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d not online for feedback check\n", feederId);
        return false;
    }
    
    return sendCommand(feederId, CMD_CHECK_FEEDBACK);
}

bool FeederManager::enableFeedback(uint8_t feederId, bool enable) {
    if (feederId >= TOTAL_FEEDERS || !feeders[feederId].online) {
        DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d not online for feedback enable\n", feederId);
        return false;
    }
    
    DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d feedback %s\n", feederId, enable ? "enabled" : "disabled");
    
    // 使用feedLength字段传递启用标志
    return sendCommand(feederId, CMD_ENABLE_FEEDBACK, 0, enable ? 1 : 0);
}

bool FeederManager::clearManualFeedFlag(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS || !feeders[feederId].online) {
        DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d not online for manual feed clear\n", feederId);
        return false;
    }
    
    DEBUG_FEEDER_MANAGER_PRINTF("Clearing manual feed flag for feeder %d\n", feederId);
    
    // 使用angle字段传递清除标志
    return sendCommand(feederId, CMD_ENABLE_FEEDBACK, 1, 0);
}

bool FeederManager::processManualFeed(uint8_t feederId) {
    // 简化版本：手动进料已在Hand端本地完成，Brain端仅清除标志
    DEBUG_FEEDER_MANAGER_PRINTF("Manual feed for feeder %d - processed locally at Hand, clearing flag\n", feederId);
    
    if (feederId < TOTAL_FEEDERS) {
        feeders[feederId].manualFeedDetected = false;
        return true;
    }
    return false;
}

// 状态查询实现
bool FeederManager::isTapeLoaded(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return false;
    return feeders[feederId].tapeLoaded;
}

bool FeederManager::isFeedbackEnabled(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return false;
    return feeders[feederId].feedbackEnabled;
}

bool FeederManager::hasManualFeedDetected(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return false;
    return feeders[feederId].manualFeedDetected;
}

uint16_t FeederManager::getFeederErrorCount(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) return 0;
    return feeders[feederId].errorCount;
}

// 反馈状态处理
void FeederManager::handleFeedbackStatus(uint8_t handId, const ESPNowFeedbackPacket& feedbackPacket) {
    uint8_t feederId = findFeederByHandId(handId);
    if (feederId >= TOTAL_FEEDERS) {
        DEBUG_FEEDER_MANAGER_PRINTF("Unknown hand %d for feedback status\n", handId);
        return;
    }
    
    // 更新反馈状态
    feeders[feederId].tapeLoaded = (feedbackPacket.tapeLoaded == 1);
    feeders[feederId].feedbackEnabled = (feedbackPacket.feedbackEnabled == 1);
    feeders[feederId].manualFeedDetected = (feedbackPacket.manualFeedDetected == 1);
    feeders[feederId].errorCount = feedbackPacket.errorCount;
    feeders[feederId].lastFeedbackUpdate = millis();
    
    DEBUG_FEEDER_MANAGER_PRINTF("Feeder %d feedback status - Tape: %s, Enabled: %s, Manual: %s, Errors: %d\n",
                                feederId,
                                feeders[feederId].tapeLoaded ? "LOADED" : "NOT_LOADED",
                                feeders[feederId].feedbackEnabled ? "YES" : "NO",
                                feeders[feederId].manualFeedDetected ? "YES" : "NO",
                                feeders[feederId].errorCount);
    
    // 注意：由于已实施本地处理方案，手动进料在Hand端本地完成
    // Brain端不再需要处理手动进料逻辑，仅记录状态用于监控
    if (feeders[feederId].manualFeedDetected) {
        DEBUG_FEEDER_MANAGER_PRINTF("Manual feed reported from feeder %d (processed locally at Hand)\n", feederId);
        // 仅作为状态记录，实际进料已在Hand端完成
    }
    
    // 如果tape未加载，记录错误
    if (!feeders[feederId].tapeLoaded) {
        DEBUG_FEEDER_MANAGER_PRINTF("WARNING: Feeder %d tape not loaded!\n", feederId);
    }
}
