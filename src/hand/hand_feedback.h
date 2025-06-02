#ifndef HAND_FEEDBACK_H
#define HAND_FEEDBACK_H

#include <Arduino.h>
#include "hand_config.h"

// 前向声明
class HandStatusLED;
class HandServo;

// 手动进料回调函数类型
typedef void (*ManualFeedCallback)(void);

/**
 * @brief 喂料器反馈管理器类
 * 
 * 该类负责管理tape loading检测系统，通过微动开关监测tape状态
 * 并检测手动进料操作。基于原始feederFeedbackPinMap系统实现。
 */
class HandFeedbackManager {
public:
    HandFeedbackManager();
    
    // 生命周期管理
    void begin();
    void update();
    void setStatusLED(HandStatusLED* led) { statusLED = led; }
    void setServoController(HandServo* servo) { servoController = servo; }
    void setManualFeedCallback(ManualFeedCallback callback) { manualFeedCallback = callback; }
    
    // 状态查询
    bool isTapeLoaded() const { return tapeLoaded; }
    bool isFeedbackEnabled() const { return feedbackEnabled; }
    bool isManualFeedDetected() const;
    
    // 控制方法
    void enableFeedback(bool enable = true);
    void disableFeedback() { enableFeedback(false); }
    void clearManualFeedFlag();
    
    // 状态获取（用于ESP-NOW通信）
    struct FeedbackStatus {
        bool tapeLoaded;
        bool feedbackEnabled;
        bool manualFeedDetected;
        uint32_t lastCheckTime;
        uint16_t errorCount;
    };
    
    FeedbackStatus getStatus() const;
    
    // 调试和诊断
    void printStatus() const;
    uint16_t getErrorCount() const { return errorCount; }
    void resetErrorCount() { errorCount = 0; }

private:
    // 硬件状态
    bool feedbackEnabled;
    bool tapeLoaded;
    bool lastPinState;
    volatile bool manualFeedDetected;
    
    // 手动进料独立状态跟踪
    bool lastManualFeedPinState;
    
    // 时间管理
    unsigned long lastCheckTime;
    unsigned long lastStateChangeTime;
    unsigned long manualFeedStartTime;
    
    // 错误计数和统计
    uint16_t errorCount;
    uint32_t totalChecks;
    
    // LED状态管理和舵机控制
    HandStatusLED* statusLED;
    HandServo* servoController;
    ManualFeedCallback manualFeedCallback;
    
    // 私有方法
    bool readFeedbackPin();
    void updateTapeStatus();
    void checkManualFeed();
    void executeManualFeed();
    void handleStateChange(bool newState);
    
    // 中断处理（如果需要）
    static void ICACHE_RAM_ATTR handlePinInterrupt();
    static HandFeedbackManager* instance; // 用于中断处理
};

#endif // HAND_FEEDBACK_H
