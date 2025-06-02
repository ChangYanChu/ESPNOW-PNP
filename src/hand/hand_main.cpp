#include <Arduino.h>
#include <ESP8266WiFi.h>  // 确保包含WiFi库
#include "hand_config.h"
#include "hand_espnow.h"
#include "hand_servo.h"
#include "hand_status_led.h"
#include "hand_feedback.h"


// 全局对象
HandESPNow espnowManager;
HandServo servoController;
HandStatusLED statusLED;
HandFeedbackManager feedbackManager;

// 喂料器ID配置 - 将从EEPROM加载
uint8_t currentFeederId = 255; // 未配置状态

// 舵机测试函数
void runServoTest() {
    DEBUG_HAND_PRINT("=== Manual Servo Test Started ===");
    
    // 保存当前LED状态
    LEDStatus originalStatus = statusLED.getStatus();
    statusLED.setStatus(LED_WORKING);
    
    if (servoController.attach()) {
        DEBUG_HAND_PRINT("Servo attached, starting movement sequence...");
        delay(100);
        
        DEBUG_HAND_PRINT("Test 1/4: Moving to 0 degrees");
        servoController.setAngle(0);
        delay(SERVO_TEST_DELAY);
        
        DEBUG_HAND_PRINT("Test 2/4: Moving to 90 degrees");
        servoController.setAngle(90);
        delay(SERVO_TEST_DELAY);
        
        DEBUG_HAND_PRINT("Test 3/4: Moving to 180 degrees");
        servoController.setAngle(180);
        delay(SERVO_TEST_DELAY);
        
        DEBUG_HAND_PRINT("Test 4/4: Returning to 90 degrees");
        servoController.setAngle(90);
        delay(SERVO_TEST_DELAY);
        
        servoController.detach();
        DEBUG_HAND_PRINT("Manual servo test completed successfully");
    } else {
        DEBUG_HAND_PRINT("Error: Failed to attach servo for manual test");
    }
    
    // 恢复原来的LED状态
    statusLED.setStatus(originalStatus);
    DEBUG_HAND_PRINT("=== Manual Servo Test Ended ===");
}

// 串口配置功能
void checkSerialConfig() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.startsWith("set_feeder_id ")) {
            uint8_t feederId = input.substring(14).toInt();
            if (feederId < TOTAL_FEEDERS) {
                currentFeederId = feederId;
                espnowManager.setFeederId(feederId);
                DEBUG_HAND_PRINT("Feeder ID set to: " + String(feederId));
            } else {
                DEBUG_HAND_PRINT("Invalid feeder ID. Must be 0-" + String(TOTAL_FEEDERS - 1));
            }
        } else if (input == "get_feeder_id") {
            DEBUG_HAND_PRINT("Current feeder ID: " + String(currentFeederId));
        } else if (input == "register") {
            if (currentFeederId < TOTAL_FEEDERS) {
                espnowManager.sendRegistration();
                DEBUG_HAND_PRINT("Manual registration sent");
            } else {
                DEBUG_HAND_PRINT("Please set feeder ID first using: set_feeder_id <id>");
            }
        } else if (input == "help") {
            DEBUG_HAND_PRINT("Available commands:");
            DEBUG_HAND_PRINT("  set_feeder_id <0-49> - Set feeder ID");
            DEBUG_HAND_PRINT("  get_feeder_id - Show current feeder ID");
            DEBUG_HAND_PRINT("  register - Send manual registration");
            DEBUG_HAND_PRINT("  test_servo - Run servo functionality test");
            DEBUG_HAND_PRINT("  feedback_status - Show feedback system status");
            DEBUG_HAND_PRINT("  enable_feedback - Enable feedback monitoring");
            DEBUG_HAND_PRINT("  disable_feedback - Disable feedback monitoring");
            DEBUG_HAND_PRINT("  clear_manual_feed - Clear manual feed flag");
            DEBUG_HAND_PRINT("  help - Show this help");
        } else if (input == "test_servo") {
            runServoTest();
        } else if (input == "feedback_status") {
            feedbackManager.printStatus();
        } else if (input == "enable_feedback") {
            feedbackManager.enableFeedback(true);
            DEBUG_HAND_PRINT("Feedback monitoring enabled");
        } else if (input == "disable_feedback") {
            feedbackManager.enableFeedback(false);
            DEBUG_HAND_PRINT("Feedback monitoring disabled");
        } else if (input == "clear_manual_feed") {
            feedbackManager.clearManualFeedFlag();
            DEBUG_HAND_PRINT("Manual feed flag cleared");
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // 减少初始延迟
    for(int i = 0; i < 100; i++) {
        yield();
        if(i % 10 == 0) delay(10); // 短暂延迟
    }
    
    Serial.println();
    DEBUG_HAND_PRINT("=== ESP01S Hand Controller Starting ===");
    DEBUG_HAND_PRINTF("Hand ID: %d\n", HAND_ID);
    DEBUG_HAND_PRINTF("Version: %s\n", HAND_VERSION);
    DEBUG_HAND_PRINT("Use 'help' command for configuration options");
    
    // 初始化状态LED
    statusLED.begin();
    statusLED.setStatus(LED_INITIALIZING);
    
    // 先初始化ESP-NOW以从EEPROM加载feeder ID，并传递反馈管理器
    if (!espnowManager.begin(&servoController, &feedbackManager)) {
        DEBUG_HAND_PRINT("ESP-NOW initialization failed!");
        statusLED.setStatus(LED_ERROR);
        while(1) {
            statusLED.update();
            yield();
            delay(100);
            DEBUG_HAND_PRINT("System halted - ESP-NOW failed");
        }
    }
    
    // 从ESP-NOW管理器获取已加载的feeder ID
    currentFeederId = espnowManager.getFeederId();
    
    // 如果还是未配置状态，等待用户配置
    if (currentFeederId == 255) {
        statusLED.setStatus(LED_SEARCHING);
        DEBUG_HAND_PRINT("Please configure feeder ID using: set_feeder_id <0-49>");
        while (currentFeederId == 255) {
            statusLED.update();
            checkSerialConfig();
            delay(100);
            yield();
        }
    } else {
        DEBUG_HAND_PRINTF("Using saved feeder ID: %d\n", currentFeederId);
    }
    
    // 初始化舵机控制器并建立LED关联
    servoController.begin();
    servoController.setStatusLED(&statusLED);
    
    // 初始化反馈管理器
    feedbackManager.begin();
    feedbackManager.setStatusLED(&statusLED);
    feedbackManager.setServoController(&servoController); // 设置舵机控制器引用，实现本地处理
    DEBUG_HAND_PRINT("Feedback Manager initialized with local servo control");
    
#ifdef ENABLE_SERVO_STARTUP_TEST
    // 舵机功能测试（可通过配置开关控制）
    DEBUG_HAND_PRINT("Starting servo functionality test...");
    statusLED.setStatus(LED_WORKING);
    
    if (servoController.attach()) {
        #ifdef SERVO_TEST_VERBOSE
        DEBUG_HAND_PRINT("Servo attached successfully, beginning movement test");
        #endif
        
        delay(100);
        
        DEBUG_HAND_PRINT("Moving to 90 degrees");
        servoController.setAngle(90);
        delay(SERVO_TEST_DELAY);
        
        DEBUG_HAND_PRINT("Moving to 0 degrees");
        servoController.setAngle(0);
        delay(SERVO_TEST_DELAY);
        
        DEBUG_HAND_PRINT("Moving to 180 degrees");
        servoController.setAngle(180);
        delay(SERVO_TEST_DELAY);
        
        DEBUG_HAND_PRINT("Moving back to 90 degrees");
        servoController.setAngle(90);
        delay(SERVO_TEST_DELAY);
        
        servoController.detach();
        DEBUG_HAND_PRINT("Servo test completed successfully");
        
        #ifdef SERVO_TEST_VERBOSE
        DEBUG_HAND_PRINT("Servo detached, test sequence finished");
        #endif
    } else {
        DEBUG_HAND_PRINT("Warning: Servo attach failed during test");
        statusLED.setStatus(LED_ERROR);
        delay(1000);  // 显示错误状态
    }
#else
    DEBUG_HAND_PRINT("Servo startup test disabled (ENABLE_SERVO_STARTUP_TEST not defined)");
#endif
    
    // 设置为搜索Brain状态
    statusLED.setStatus(LED_SEARCHING);
    
    DEBUG_HAND_PRINTF("MAC Address: %s\n", WiFi.macAddress().c_str());
    DEBUG_HAND_PRINT("=== Hand Controller Ready ===");
    
    // 发送初始状态 - 使用非阻塞方式
    for(int i = 0; i < 200; i++) {
        statusLED.update();
        yield();
        if(i % 20 == 0) delay(10);
    }
    
    espnowManager.sendStatus(STATUS_OK, "Hand ready");
}

void loop() {
    // 更新状态LED
    statusLED.update();
    
    // 检查串口配置命令
    checkSerialConfig();
    
    // 更新舵机控制器（处理异步操作）
    servoController.update();
    
    // 更新反馈管理器
    feedbackManager.update();
    
    // 更新ESP-NOW通信
    espnowManager.update();
    
    // 简化的手动进料处理：只需要清除已处理的标志
    if (feedbackManager.isManualFeedDetected()) {
        // 手动进料已经在feedbackManager.update()中本地处理完成
        // 这里只需要清除标志，保持简单
        feedbackManager.clearManualFeedFlag();
    }
    
    // 更新LED状态基于连接状态
    static bool lastBrainOnlineStatus = false;
    bool currentBrainOnline = espnowManager.isBrainOnline();
    if (currentBrainOnline != lastBrainOnlineStatus) {
        lastBrainOnlineStatus = currentBrainOnline;
        if (currentBrainOnline) {
            statusLED.setStatus(LED_CONNECTED);
            DEBUG_HAND_PRINT("Brain connection established");
        } else {
            statusLED.setStatus(LED_SEARCHING);
            DEBUG_HAND_PRINT("Brain connection lost");
        }
    }
    
    // 输出状态信息（每5秒一次）
    static unsigned long lastStatusOutput = 0;
    if (millis() - lastStatusOutput > 5000) {
        lastStatusOutput = millis();
        #ifdef DEBUG_HAND
        DEBUG_HAND_PRINTF("Servo status - Attached: %s, Current angle: %d, Moving: %s\n",
                          servoController.isAttached() ? "YES" : "NO",
                          servoController.getCurrentAngle(),
                          servoController.isMoving() ? "YES" : "NO");
        
        // 输出反馈状态
        if (feedbackManager.isFeedbackEnabled()) {
            DEBUG_HAND_PRINTF("Feedback - Tape: %s, Manual feed: %s, Errors: %d\n",
                              feedbackManager.isTapeLoaded() ? "LOADED" : "NOT_LOADED",
                              feedbackManager.isManualFeedDetected() ? "DETECTED" : "NONE",
                              feedbackManager.getErrorCount());
        }
        #endif
    }
    
    // 给系统更多时间处理其他任务
    yield();
    
    // 非常短暂的延迟
    delay(1);
}
