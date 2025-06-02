#include "hand_feedback.h"
#include "hand_status_led.h"
#include "hand_servo.h"

// 静态实例指针（用于中断处理）
HandFeedbackManager* HandFeedbackManager::instance = nullptr;

HandFeedbackManager::HandFeedbackManager() 
    : feedbackEnabled(false)
    , tapeLoaded(false)
    , lastPinState(HIGH)
    , manualFeedDetected(false)
    , lastManualFeedPinState(HIGH)
    , lastCheckTime(0)
    , lastStateChangeTime(0)
    , manualFeedStartTime(0)
    , errorCount(0)
    , totalChecks(0)
    , statusLED(nullptr)
    , servoController(nullptr)
    , manualFeedCallback(nullptr)
{
    instance = this; // 设置静态实例指针
}

void HandFeedbackManager::begin() {
    DEBUG_HAND_FEEDBACK_PRINT("=== Initializing Feedback Manager ===");
    
    // 配置反馈引脚
    pinMode(FEEDBACK_PIN, INPUT_PULLUP);
    
    // 读取初始状态
    bool initialState = readFeedbackPin();
    lastPinState = initialState;
    lastManualFeedPinState = initialState; // 初始化手动进料状态跟踪
    tapeLoaded = !initialState; // 反转逻辑：LOW = tape loaded
    lastStateChangeTime = millis();
    
    DEBUG_HAND_FEEDBACK_PRINTF("Feedback pin configured on GPIO%d\n", FEEDBACK_PIN);
    DEBUG_HAND_FEEDBACK_PRINTF("Initial pin state: %s\n", initialState ? "HIGH" : "LOW");
    DEBUG_HAND_FEEDBACK_PRINTF("Initial tape status: %s\n", tapeLoaded ? "LOADED" : "NOT_LOADED");
    
    // 默认启用反馈
    feedbackEnabled = true;
    
    #ifdef FEEDBACK_USE_INTERRUPT
    // 如果启用中断模式，附加中断处理
    attachInterrupt(digitalPinToInterrupt(FEEDBACK_PIN), handlePinInterrupt, CHANGE);
    DEBUG_HAND_FEEDBACK_PRINT("Feedback interrupt enabled");
    #endif
    
    DEBUG_HAND_FEEDBACK_PRINT("Feedback Manager initialized successfully");
}

void HandFeedbackManager::update() {
    if (!feedbackEnabled) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // 检查是否到了更新时间
    if (currentTime - lastCheckTime >= FEEDBACK_CHECK_INTERVAL) {
        lastCheckTime = currentTime;
        totalChecks++;
        
        // 重要：先检查手动进料，再更新tape状态
        // 因为两个功能都依赖lastPinState，必须避免状态冲突
        checkManualFeed();
        updateTapeStatus();
    }
}

bool HandFeedbackManager::readFeedbackPin() {
    return digitalRead(FEEDBACK_PIN);
}

void HandFeedbackManager::updateTapeStatus() {
    bool currentPinState = readFeedbackPin();
    
    // 检查状态变化
    if (currentPinState != lastPinState) {
        handleStateChange(currentPinState);
    }
    
    lastPinState = currentPinState;
}

void HandFeedbackManager::handleStateChange(bool newState) {
    unsigned long currentTime = millis();
    
    // 防抖处理
    if (currentTime - lastStateChangeTime < FEEDBACK_DEBOUNCE_TIME) {
        return; // 忽略抖动
    }
    
    lastStateChangeTime = currentTime;
    
    // 更新tape状态（反转逻辑）
    bool newTapeStatus = !newState; // LOW = loaded, HIGH = not loaded
    
    if (newTapeStatus != tapeLoaded) {
        tapeLoaded = newTapeStatus;
        
        DEBUG_HAND_FEEDBACK_PRINTF("Tape status changed: %s (pin: %s)\n", 
                          tapeLoaded ? "LOADED" : "NOT_LOADED",
                          newState ? "HIGH" : "LOW");
        
        // 更新LED状态
        if (statusLED) {
            if (!tapeLoaded) {
                statusLED->setStatus(LED_ERROR);
                errorCount++;
            } else {
                statusLED->setStatus(LED_CONNECTED);
            }
        }
    }
}

void HandFeedbackManager::checkManualFeed() {
    bool currentPinState = readFeedbackPin();
    unsigned long currentTime = millis();
    
    // 参考0816feeder实现：简单的本地手动进料检测
    // 检测按压开始（HIGH → LOW，tensioner pressed）
    if (!currentPinState && lastManualFeedPinState) {
        // 下降沿 - 开始计时
        manualFeedStartTime = currentTime;
        DEBUG_HAND_FEEDBACK_PRINT("Manual feed press detected - timing started");
    }
    // 检测按压结束（LOW → HIGH，tensioner released）
    else if (currentPinState && !lastManualFeedPinState && manualFeedStartTime > 0) {
        // 上升沿 - 检查持续时间
        unsigned long pulseDuration = currentTime - manualFeedStartTime;
        
        // 0816feeder使用50ms作为有效按压的最小时间
        if (pulseDuration >= MANUAL_FEED_MIN_TIME && 
            pulseDuration <= MANUAL_FEED_MAX_TIME) {
            
            DEBUG_HAND_FEEDBACK_PRINTF("Valid manual feed detected! Duration: %lu ms\n", pulseDuration);
            
            // 立即本地处理手动进料 - 类似0816feeder的 this->advance(feedLength, true)
            executeManualFeed();
        }
        else {
            DEBUG_HAND_FEEDBACK_PRINTF("Invalid pulse duration: %lu ms (expected: %d-%d ms)\n", 
                              pulseDuration, MANUAL_FEED_MIN_TIME, MANUAL_FEED_MAX_TIME);
        }
        
        manualFeedStartTime = 0;
    }
    
    // 更新手动进料专用的状态跟踪
    lastManualFeedPinState = currentPinState;
}

bool HandFeedbackManager::isManualFeedDetected() const {
    return manualFeedDetected;
}

void HandFeedbackManager::clearManualFeedFlag() {
    manualFeedDetected = false;
    DEBUG_HAND_FEEDBACK_PRINT("Manual feed flag cleared");
}

void HandFeedbackManager::enableFeedback(bool enable) {
    if (feedbackEnabled != enable) {
        feedbackEnabled = enable;
        DEBUG_HAND_FEEDBACK_PRINTF("Feedback %s\n", enable ? "ENABLED" : "DISABLED");
        
        if (enable) {
            // 重新读取状态
            bool currentState = readFeedbackPin();
            tapeLoaded = !currentState;
            lastPinState = currentState;
            lastStateChangeTime = millis();
        }
    }
}

HandFeedbackManager::FeedbackStatus HandFeedbackManager::getStatus() const {
    FeedbackStatus status;
    status.tapeLoaded = tapeLoaded;
    status.feedbackEnabled = feedbackEnabled;
    status.manualFeedDetected = manualFeedDetected;
    status.lastCheckTime = lastCheckTime;
    status.errorCount = errorCount;
    return status;
}

void HandFeedbackManager::printStatus() const {
    DEBUG_HAND_FEEDBACK_PRINT("=== Feedback Status ===");
    DEBUG_HAND_FEEDBACK_PRINTF("Enabled: %s\n", feedbackEnabled ? "YES" : "NO");
    DEBUG_HAND_FEEDBACK_PRINTF("Tape Loaded: %s\n", tapeLoaded ? "YES" : "NO");
    DEBUG_HAND_FEEDBACK_PRINTF("Manual Feed Detected: %s\n", manualFeedDetected ? "YES" : "NO");
    DEBUG_HAND_FEEDBACK_PRINTF("Pin State: %s\n", lastPinState ? "HIGH" : "LOW");
    DEBUG_HAND_FEEDBACK_PRINTF("Error Count: %d\n", errorCount);
    DEBUG_HAND_FEEDBACK_PRINTF("Total Checks: %lu\n", totalChecks);
    DEBUG_HAND_FEEDBACK_PRINTF("Last Check: %lu ms ago\n", millis() - lastCheckTime);
}

void HandFeedbackManager::executeManualFeed() {
    DEBUG_HAND_FEEDBACK_PRINT("Executing manual feed locally");
    
    // 设置工作状态LED
    if (statusLED) {
        statusLED->setStatus(LED_WORKING);
    }
    
    // 使用回调函数执行进料（如果设置了）
    if (manualFeedCallback) {
        DEBUG_HAND_FEEDBACK_PRINT("Calling manual feed callback");
        manualFeedCallback();
        
        // 设置完成标志
        manualFeedDetected = true;
        
        // 恢复LED状态
        if (statusLED) {
            statusLED->setStatus(LED_CONNECTED);
        }
        return;
    }
    
    // 如果有直接舵机控制器引用，执行进料动作
    if (servoController) {
        DEBUG_HAND_FEEDBACK_PRINT("Executing servo feed sequence directly");
        
        // 参考0816feeder的advance逻辑：推进到80度，然后回缩到0度
        if (servoController->attach()) {
            bool feedSuccess = true;
            
            // 推进阶段
            DEBUG_HAND_FEEDBACK_PRINT("Manual feed: moving to 80 degrees");
            if (servoController->setAngle(80)) {
                // 等待舵机到位（非阻塞检查）
                unsigned long moveStartTime = millis();
                while (servoController->isMoving() && (millis() - moveStartTime < 3000)) {
                    servoController->update(); // 关键：更新舵机状态
                    yield();
                    delay(10);
                }
                
                // 检查超时
                if (millis() - moveStartTime >= 3000) {
                    DEBUG_HAND_FEEDBACK_PRINT("Warning: Servo advance timeout");
                    feedSuccess = false;
                }
                
                // 短暂停留
                delay(200);
                
                // 回缩阶段
                DEBUG_HAND_FEEDBACK_PRINT("Manual feed: retracting to 0 degrees");
                if (servoController->setAngle(0)) {
                    // 等待回缩完成
                    moveStartTime = millis();
                    while (servoController->isMoving() && (millis() - moveStartTime < 3000)) {
                        servoController->update(); // 关键：更新舵机状态
                        yield();
                        delay(10);
                    }
                    
                    // 检查超时
                    if (millis() - moveStartTime >= 3000) {
                        DEBUG_HAND_FEEDBACK_PRINT("Warning: Servo retract timeout");
                        feedSuccess = false;
                    }
                } else {
                    DEBUG_HAND_FEEDBACK_PRINT("Failed to set retract angle");
                    feedSuccess = false;
                }
            } else {
                DEBUG_HAND_FEEDBACK_PRINT("Failed to set advance angle");
                feedSuccess = false;
            }
            
            servoController->detach();
            
            if (feedSuccess) {
                DEBUG_HAND_FEEDBACK_PRINT("Manual feed cycle completed successfully");
            } else {
                DEBUG_HAND_FEEDBACK_PRINT("Manual feed cycle completed with warnings");
            }
        } else {
            DEBUG_HAND_FEEDBACK_PRINT("Failed to attach servo for manual feed");
        }
    } else {
        DEBUG_HAND_FEEDBACK_PRINT("No servo controller available for manual feed");
    }
    
    // 设置手动进料完成标志（用于状态查询）
    manualFeedDetected = true;
    
    // 恢复LED状态
    if (statusLED) {
        statusLED->setStatus(LED_CONNECTED);
    }
}

// 中断处理函数（如果启用）
void ICACHE_RAM_ATTR HandFeedbackManager::handlePinInterrupt() {
    if (instance) {
        // 在中断中只设置标志，实际处理在主循环中进行
        // 这里可以添加中断特定的处理逻辑
    }
}
