#ifndef BRAIN_FEEDER_MANAGER_H
#define BRAIN_FEEDER_MANAGER_H

#include <Arduino.h>
#include "brain_config.h"

// 前向声明
class BrainESPNow;

// 虚拟喂料器状态
struct VirtualFeeder {
    uint8_t feederId;
    uint8_t handId;                  // 对应的手部ID
    bool enabled;
    bool online;
    unsigned long lastResponse;
    
    // 喂料器设置
    uint16_t fullAdvanceAngle;
    uint16_t halfAdvanceAngle;
    uint16_t retractAngle;
    uint8_t feedLength;
    uint16_t settleTime;
    uint16_t minPulse;
    uint16_t maxPulse;
    
    // 反馈系统状态
    bool feedbackEnabled;
    bool tapeLoaded;
    bool manualFeedDetected;
    uint16_t errorCount;
    unsigned long lastFeedbackUpdate;
    
    VirtualFeeder() : feederId(0), handId(0), enabled(false), online(false), 
                     lastResponse(0), fullAdvanceAngle(DEFAULT_FULL_ADVANCE_ANGLE),
                     halfAdvanceAngle(DEFAULT_HALF_ADVANCE_ANGLE), 
                     retractAngle(DEFAULT_RETRACT_ANGLE),
                     feedLength(DEFAULT_FEED_LENGTH), settleTime(DEFAULT_SETTLE_TIME),
                     minPulse(DEFAULT_MIN_PULSE), maxPulse(DEFAULT_MAX_PULSE),
                     feedbackEnabled(false), tapeLoaded(false), manualFeedDetected(false),
                     errorCount(0), lastFeedbackUpdate(0) {}
};

class FeederManager {
public:
    FeederManager();
    
    void begin(BrainESPNow* espnowMgr);
    void update();
    
    // 喂料器操作
    bool enableSystem(bool enable);
    bool advanceFeeder(uint8_t feederId, uint8_t length);
    bool retractFeeder(uint8_t feederId);
    bool completeFeedCycle(uint8_t feederId, uint8_t length); // OpenPnP兼容：完整进料循环
    bool setServoAngle(uint8_t feederId, uint16_t angle);
    bool requestFeederStatus(uint8_t feederId);
    
    // 反馈系统操作
    bool checkFeedback(uint8_t feederId);
    bool enableFeedback(uint8_t feederId, bool enable);
    bool clearManualFeedFlag(uint8_t feederId);
    bool processManualFeed(uint8_t feederId);  // 注意：已改为本地处理方案，此方法仅清除标志
    
    // 状态查询
    bool isFeederOnline(uint8_t feederId);
    bool isSystemEnabled() const { return systemEnabled; }
    bool isTapeLoaded(uint8_t feederId);
    bool isFeedbackEnabled(uint8_t feederId);
    bool hasManualFeedDetected(uint8_t feederId);
    uint16_t getFeederErrorCount(uint8_t feederId);
    String getFeederStatus(uint8_t feederId);
    String getAllHandsStatus();
    
    // 配置管理
    bool updateFeederConfig(uint8_t feederId, uint16_t fullAngle, uint16_t halfAngle, 
                           uint16_t retractAngle, uint8_t feedLen, uint16_t settleTime,
                           uint16_t minPulse, uint16_t maxPulse);
    
    // 响应处理
    void handleResponse(uint8_t handId, const ESPNowResponse& response);
    void handleFeedbackStatus(uint8_t handId, const ESPNowFeedbackPacket& feedbackPacket);

private:
    BrainESPNow* espnowManager;
    VirtualFeeder feeders[TOTAL_FEEDERS];
    bool systemEnabled;
    unsigned long lastHeartbeat;
    
    // 辅助函数
    void updateFeederMappings();
    uint8_t findFeederByHandId(uint8_t handId);
    bool sendCommand(uint8_t feederId, ESPNowCommandType cmd, uint16_t angle = 0, 
                    uint8_t feedLength = 0, uint16_t pulseMin = 0, uint16_t pulseMax = 0);
    void checkHandsOnlineStatus();
    void sendHeartbeat();
};

#endif // BRAIN_FEEDER_MANAGER_H
