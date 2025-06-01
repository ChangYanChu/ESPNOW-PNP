#include "brain_gcode.h"
#include "brain_feeder_manager.h"

BrainGCode::BrainGCode() : feederManager(nullptr), systemEnabled(false) {
    inputBuffer.reserve(MAX_GCODE_LINE_LENGTH);
}

void BrainGCode::begin(FeederManager* feederMgr) {
    feederManager = feederMgr;
    inputBuffer.reserve(MAX_GCODE_LINE_LENGTH);
    Serial.println(F("Brain G-code processor initialized"));
}

void BrainGCode::update() {
    processSerialInput();
}

void BrainGCode::processSerialInput() {
    while (Serial.available()) {
        char receivedChar = (char)Serial.read();
        
        #ifdef DEBUG_GCODE
        Serial.print(receivedChar);
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
    int cmd = parseParameter('M', -1);
    
    #ifdef DEBUG_GCODE
    Serial.print(F("Processing command: M"));
    Serial.println(cmd);
    #endif
    
    switch (cmd) {
        case MCODE_FEEDER_ENABLE:
            processM610();
            break;
        case MCODE_ADVANCE:
            processM600();
            break;
        case MCODE_RETRACT:
            processM601();
            break;
        case MCODE_STATUS:
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
    if (!systemEnabled) {
        sendResponse(false, F("Enable system first! M610 S1"));
        return;
    }
    
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    int feedLength = parseParameter('F', DEFAULT_FEED_LENGTH);
    if (feedLength <= 0 || feedLength > 24 || (feedLength % 2) != 0) {
        sendResponse(false, F("Invalid feed length"));
        return;
    }
    
    bool success = feederManager->advanceFeeder(feederId, feedLength);
    sendResponse(success, success ? F("Feed command sent") : F("Failed to send feed command"));
}

void BrainGCode::processM601() {
    if (!systemEnabled) {
        sendResponse(false, F("Enable system first! M610 S1"));
        return;
    }
    
    int feederId = parseParameter('N', -1);
    if (!validateFeederNumber(feederId)) {
        sendResponse(false, F("Invalid feeder number"));
        return;
    }
    
    bool success = feederManager->retractFeeder(feederId);
    sendResponse(success, success ? F("Retract command sent") : F("Failed to send retract command"));
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
