#include "brain_gcode.h"
#include "brain_feeder_manager.h"
#include "brain_espnow.h"

BrainGCode::BrainGCode() : feederManager(nullptr), espnowManager(nullptr), systemEnabled(false) {
    inputBuffer.reserve(MAX_GCODE_LINE_LENGTH);
}

void BrainGCode::begin(FeederManager* feederMgr, BrainESPNow* espnowMgr) {
    feederManager = feederMgr;
    espnowManager = espnowMgr;
    inputBuffer.reserve(MAX_GCODE_LINE_LENGTH);
    // Serial.println(F("Brain G-code processor initialized"));
}

void BrainGCode::update() {
    processSerialInput();
}

void BrainGCode::processSerialInput() {
    while (Serial.available()) {
        char receivedChar = (char)Serial.read();
        
        #ifdef DEBUG_VERBOSE_GCODE
        DEBUG_PRINT(receivedChar);
        #endif
        
        inputBuffer += receivedChar;
        
        if (receivedChar == '\n') {
            // 移除注释
            int commentPos = inputBuffer.indexOf(";");
            if (commentPos != -1) {
                inputBuffer.remove(commentPos);
            }
            inputBuffer.trim();
            
            if (inputBuffer.length() > 0) {
                processCommand();
            }
            
            inputBuffer = "";
        }
        
        // 防止缓冲区溢出
        if (inputBuffer.length() >= MAX_GCODE_LINE_LENGTH) {
            inputBuffer = "";
        }
    }
}

void BrainGCode::processCommand() {
    // 首先检查是否是管理命令
    String cmd = inputBuffer;
    cmd.toLowerCase();
    
    if (cmd == "discovery" || cmd == "broadcast") {
        if (espnowManager) {
            Serial.println(F("Broadcasting discovery..."));
            espnowManager->broadcastDiscovery();
        } else {
            Serial.println(F("error ESP-NOW manager not available"));
        }
        return;
    } else if (cmd == "request_registration") {
        if (espnowManager) {
            Serial.println(F("Requesting hand registration..."));
            espnowManager->requestRegistration();
        } else {
            Serial.println(F("error ESP-NOW manager not available"));
        }
        return;
    } else if (cmd == "clear_registration") {
        if (espnowManager) {
            Serial.println(F("Clearing all registrations..."));
            espnowManager->clearRegistrations();
        } else {
            Serial.println(F("error ESP-NOW manager not available"));
        }
        return;
    } else if (cmd == "status" || cmd == "hands") {
        if (espnowManager) {
            espnowManager->printRegistrationStatus();
        } else {
            Serial.println(F("error ESP-NOW manager not available"));
        }
        return;
    } else if (cmd == "help") {
        Serial.println(F("  Available commands:"));
        Serial.println(F("  discovery - Broadcast discovery to hands"));
        Serial.println(F("  request_registration - Request hands to register"));
        Serial.println(F("  clear_registration - Clear all hand registrations"));
        Serial.println(F("  status/hands - Show hand registration status"));
        Serial.println(F("  help - Show this help"));
        return;
    }
    
    // 如果不是管理命令，检查是否是G-code命令
    int mcmd = parseParameter('M', -1);
    
    if (mcmd == -1) {
        // 不是M代码，忽略
        DEBUG_GCODE_PRINT("Unknown command ignored");
        return;
    }
    
    DEBUG_GCODE_PRINTF("Processing G-code command: M%d\n", mcmd);
    DEBUG_GCODE_PRINTF("Raw command: %s\n", inputBuffer.c_str());
    
    switch (mcmd) {
        case MCODE_FEEDER_ENABLE:
            DEBUG_GCODE_PRINT("Calling processM610");
            processM610();
            break;
        case MCODE_ADVANCE:
            DEBUG_GCODE_PRINT("Calling processM600");
            processM600();
            break;
        case MCODE_STATUS:
            DEBUG_GCODE_PRINT("Calling processM602");
            processM602();
            break;
        case MCODE_SERVO_ANGLE:
            processM280();
            break;
        case MCODE_CONFIG_UPDATE:
            processM603();
            break;
        case MCODE_HANDS_STATUS:
            processM620();
            break;
        case MCODE_CHECK_FEEDBACK:
            DEBUG_GCODE_PRINT("Calling processM604");
            processM604();
            break;
        case MCODE_ENABLE_FEEDBACK:
            DEBUG_GCODE_PRINT("Calling processM605");
            processM605();
            break;
        case MCODE_CLEAR_MANUAL_FEED:
            DEBUG_GCODE_PRINT("Calling processM606");
            processM606();
            break;
        case MCODE_PROCESS_MANUAL_FEED:
            DEBUG_GCODE_PRINT("Calling processM607");
            processM607();
            break;
        default:
            sendResponse(true, F("unknown or empty command ignored"));
            break;
    }
}

float BrainGCode::parseParameter(char code, float defaultVal) {
    int codePosition = inputBuffer.indexOf(code);
    if (codePosition != -1) {
        int delimiterPosition = inputBuffer.indexOf(" ", codePosition + 1);
        if (delimiterPosition == -1) {
            delimiterPosition = inputBuffer.length();
        }
        return inputBuffer.substring(codePosition + 1, delimiterPosition).toFloat();
    }
    return defaultVal;
}

void BrainGCode::sendResponse(bool success, const String& message) {
    if (success) {
        Serial.print(F("ok "));
    } else {
        Serial.print(F("error "));
    }
    Serial.println(message);
}

bool BrainGCode::validateFeederNumber(int feederId) {
    return (feederId >= 0 && feederId < TOTAL_FEEDERS);
}

void BrainGCode::processM610() {
    int enable = parseParameter('S', -1);
    
    if (enable == 0 || enable == 1) {
        bool success = feederManager->enableSystem(enable == 1);
        if (success) {
            systemEnabled = (enable == 1);
            sendResponse(true, enable == 1 ? F("Feeder system enabled") : F("Feeder system disabled"));
        } else {
            sendResponse(false, F("Failed to change system state"));
        }
    } else if (enable == -1) {
        sendResponse(true, String("System state: ") + (systemEnabled ? "enabled" : "disabled"));
    } else {
        sendResponse(false, F("Invalid parameter"));
    }
}

void BrainGCode::processM600() {
    DEBUG_GCODE_PRINT("Processing M600 command");
    
    if (!systemEnabled) {
        DEBUG_GCODE_PRINT("System not enabled");
        sendResponse(false, F("Enable system first! M610 S1"));
        return;
    }
    
    int feederId = parseParameter('N', -1);
    DEBUG_GCODE_PRINTF("Parsed feeder ID: %d\n", feederId);
    
    if (!validateFeederNumber(feederId)) {
        DEBUG_GCODE_PRINT("Invalid feeder number");
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    int feedLength = parseParameter('F', DEFAULT_FEED_LENGTH);
    DEBUG_GCODE_PRINTF("Parsed feed length: %d\n", feedLength);
    
    if (feedLength <= 0 || feedLength > 24 || (feedLength % 2) != 0) {
        DEBUG_GCODE_PRINT("Invalid feed length");
        sendResponse(false, F("Invalid feed length"));
        return;
    }
    
    DEBUG_GCODE_PRINTF("Calling feederManager->completeFeedCycle(%d, %d)\n", feederId, feedLength);
    
    bool success = feederManager->completeFeedCycle(feederId, feedLength);
    
    DEBUG_GCODE_PRINTF("completeFeedCycle result: %s\n", success ? "SUCCESS" : "FAILED");
    
    sendResponse(success, success ? F("Complete feed cycle sent") : F("Failed to send feed cycle command"));
}

void BrainGCode::processM602() {
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    String status = feederManager->getFeederStatus(feederId);
    sendResponse(true, status);
}

void BrainGCode::processM280() {
    if (!systemEnabled) {
        sendResponse(false, F("Enable system first! M610 S1"));
        return;
    }
    
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    int angle = parseParameter('A', 90);
    if (angle < 0 || angle > 180) {
        sendResponse(false, F("Invalid angle"));
        return;
    }
    
    bool success = feederManager->setServoAngle(feederId, angle);
    sendResponse(success, success ? F("Servo angle set") : F("Failed to set servo angle"));
}

void BrainGCode::processM603() {
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    // 解析配置参数
    uint16_t fullAngle = parseParameter('A', DEFAULT_FULL_ADVANCE_ANGLE);
    uint16_t halfAngle = parseParameter('B', DEFAULT_HALF_ADVANCE_ANGLE);
    uint16_t retractAngle = parseParameter('C', DEFAULT_RETRACT_ANGLE);
    uint8_t feedLength = parseParameter('F', DEFAULT_FEED_LENGTH);
    uint16_t settleTime = parseParameter('U', DEFAULT_SETTLE_TIME);
    uint16_t minPulse = parseParameter('V', DEFAULT_MIN_PULSE);
    uint16_t maxPulse = parseParameter('W', DEFAULT_MAX_PULSE);
    
    bool success = feederManager->updateFeederConfig(feederId, fullAngle, halfAngle, 
                                                    retractAngle, feedLength, settleTime, 
                                                    minPulse, maxPulse);
    sendResponse(success, success ? F("Config updated") : F("Failed to update config"));
}

void BrainGCode::processM620() {
    String status = feederManager->getAllHandsStatus();
    sendResponse(true, status);
}

// 反馈系统M命令实现
void BrainGCode::processM604() {
    // M604 Nx - 查询反馈状态
    // N参数：喂料器ID
    
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    DEBUG_GCODE_PRINTF("Checking feedback status for feeder %d\n", feederId);
    
    bool success = feederManager->checkFeedback(feederId);
    if (success) {
        // 查询成功，等待反馈状态更新
        delay(100); // 给一点时间让反馈状态更新
        
        // 输出当前状态
        bool tapeLoaded = feederManager->isTapeLoaded(feederId);
        bool feedbackEnabled = feederManager->isFeedbackEnabled(feederId);
        bool manualFeedDetected = feederManager->hasManualFeedDetected(feederId);
        uint16_t errorCount = feederManager->getFeederErrorCount(feederId);
        
        Serial.printf("Feeder %d feedback status:\n", feederId);
        Serial.printf("  Tape loaded: %s\n", tapeLoaded ? "YES" : "NO");
        Serial.printf("  Feedback enabled: %s\n", feedbackEnabled ? "YES" : "NO");
        Serial.printf("  Manual feed detected: %s\n", manualFeedDetected ? "YES" : "NO");
        Serial.printf("  Error count: %d\n", errorCount);
        
        sendResponse(true, F("Feedback status checked"));
    } else {
        sendResponse(false, F("Failed to check feedback status"));
    }
}

void BrainGCode::processM605() {
    // M605 Nx Sy - 启用/禁用反馈
    // N参数：喂料器ID
    // S参数：0=禁用，1=启用
    
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    int enable = parseParameter('S', -1);
    if (enable < 0 || enable > 1) {
        sendResponse(false, F("Invalid S parameter. Use S0 (disable) or S1 (enable)"));
        return;
    }
    
    DEBUG_GCODE_PRINTF("Feeder %d feedback %s\n", feederId, enable ? "enable" : "disable");
    
    bool success = feederManager->enableFeedback(feederId, enable == 1);
    const char* action = enable ? "enabled" : "disabled";
    sendResponse(success, success ? String("Feedback ") + action : String("Failed to change feedback state"));
}

void BrainGCode::processM606() {
    // M606 Nx - 清除手动进料标志
    // N参数：喂料器ID
    
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    DEBUG_GCODE_PRINTF("Clearing manual feed flag for feeder %d\n", feederId);
    
    bool success = feederManager->clearManualFeedFlag(feederId);
    sendResponse(success, success ? F("Manual feed flag cleared") : F("Failed to clear manual feed flag"));
}

void BrainGCode::processM607() {
    // M607 Nx - 查询手动进料状态（实际处理在Hand端本地完成）
    // N参数：喂料器ID
    
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    DEBUG_GCODE_PRINTF("Checking manual feed status for feeder %d\n", feederId);
    
    // 由于手动进料现在在Hand端本地处理，这里只清理标志状态
    bool success = feederManager->processManualFeed(feederId);
    sendResponse(success, success ? F("Manual feed status cleared") : F("Failed to clear manual feed status"));
}
