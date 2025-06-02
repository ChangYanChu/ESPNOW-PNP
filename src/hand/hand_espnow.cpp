#include "hand_espnow.h"
#include "hand_servo.h"
#include "hand_feedback.h"
#include <EEPROM.h>

// 静态实例指针用于回调
static HandESPNow* handInstance = nullptr;

// 广播MAC地址
uint8_t broadcast_mac[6] = BROADCAST_MAC;

HandESPNow::HandESPNow() : servoController(nullptr), feedbackManager(nullptr), 
                          brainOnline(false), lastCommandTime(0), lastHeartbeatResponse(0), 
                          feederId(255), registered(false),
                          registrationScheduledTime(0), registrationPending(false) {
    handInstance = this;
}

bool HandESPNow::begin(HandServo* servoCtrl, HandFeedbackManager* feedbackMgr) {
    servoController = servoCtrl;
    feedbackManager = feedbackMgr;
    
    // 初始化EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // 从EEPROM加载feeder ID
    loadFeederId();
    
    // 设置WiFi模式
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    // ESP8266的ESP-NOW初始化
    if (esp_now_init() != 0) {
        DEBUG_HAND_ESPNOW_PRINT("ESP-NOW init failed");
        return false;
    }
    
    // 设置角色为从设备
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    
    // 注册回调函数
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceived);
    
    // 添加广播对等设备用于接收发现消息
    if (!addBroadcastPeer()) {
        DEBUG_HAND_ESPNOW_PRINT("Failed to add broadcast peer");
        return false;
    }
    
    DEBUG_HAND_ESPNOW_PRINTF("Hand MAC: %s\n", WiFi.macAddress().c_str());
    DEBUG_HAND_ESPNOW_PRINTF("Feeder ID: %d\n", feederId);
    DEBUG_HAND_ESPNOW_PRINT("ESP-NOW Hand initialized, waiting for brain discovery");
    
    return true;
}

void HandESPNow::update() {
    checkBrainConnection();
    
    // 处理待发送的注册请求
    processScheduledRegistration();
    
    // 自动回缩逻辑
    if (feederRetractPending && millis() >= feederRetractTime) {
        feederRetractPending = false;
        DEBUG_HAND_ESPNOW_PRINT("Feeder auto retracting to 0 degrees");
        servoController->requestSetAngle(feederRetractAngle);
    }
    
    // 定期发送心跳响应（只要有配置的feeder ID就发送）
    if (feederId != 255 && millis() - lastHeartbeatResponse > HEARTBEAT_RESPONSE_INTERVAL) {
        sendStatus(STATUS_OK, "Hand alive");
        lastHeartbeatResponse = millis();
    }
}

bool HandESPNow::sendStatus(ESPNowStatusCode status, const char* message) {
    // 只要有配置的feeder ID就可以发送状态
    if (feederId == 255) {
        return false; // 没有配置feeder ID
    }
    
    ESPNowResponse response;
    memset(&response, 0, sizeof(response));
    
    response.magic = ESPNOW_MAGIC;
    response.handId = HAND_ID;
    response.commandType = CMD_RESPONSE;
    response.status = status;
    response.sequence = millis();
    response.timestamp = millis();
    
    if (message) {
        strncpy(response.message, message, sizeof(response.message) - 1);
        response.message[sizeof(response.message) - 1] = '\0';
    }
    
    int result = esp_now_send(broadcast_mac, (uint8_t*)&response, sizeof(response));
    
    DEBUG_HAND_ESPNOW_PRINTF("Sending status to brain - Status: %d, Message: %s, Result: %s\n", 
                             status, message ? message : "", result == 0 ? "OK" : "FAIL");
    
    return result == 0;
}

bool HandESPNow::sendResponse(const ESPNowPacket& originalPacket, ESPNowStatusCode status, const char* message) {
    if (!registered) {
        return false; // 只有注册后才能发送响应
    }
    
    ESPNowResponse response;
    memset(&response, 0, sizeof(response));
    
    response.magic = ESPNOW_MAGIC;
    response.handId = HAND_ID;
    response.commandType = originalPacket.commandType;
    response.status = status;
    response.sequence = originalPacket.sequence;
    response.timestamp = millis();
    
    if (message) {
        strncpy(response.message, message, sizeof(response.message) - 1);
        response.message[sizeof(response.message) - 1] = '\0';
    }
    
    int result = esp_now_send(broadcast_mac, (uint8_t*)&response, sizeof(response));
    
    DEBUG_HAND_ESPNOW_PRINTF("Sending response - CmdType: %d, Seq: %lu, Status: %d, Message: %s, Result: %s\n", 
                             originalPacket.commandType, originalPacket.sequence, status, 
                             message ? message : "", result == 0 ? "OK" : "FAIL");
    
    return result == 0;
}

bool HandESPNow::isBrainOnline() {
    return brainOnline;
}

unsigned long HandESPNow::getLastCommand() {
    return lastCommandTime;
}

// 静态回调函数
// ESP8266回调函数
void HandESPNow::onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
    #ifdef DEBUG_HAND
    DEBUG_HAND_ESPNOW_PRINTF("Send status: %s\n", sendStatus == 0 ? "Success" : "Fail");
    #endif
}

void HandESPNow::onDataReceived(uint8_t *mac_addr, uint8_t *data, uint8_t len) {
    // 添加接收日志
    #ifdef DEBUG_VERBOSE_HAND_ESPNOW
    DEBUG_HAND_ESPNOW_PRINT("ESP-NOW data received from: ");
    for(int i = 0; i < 6; i++) {
        if(mac_addr[i] < 16) DEBUG_PRINT("0");
        DEBUG_PRINT(String(mac_addr[i], HEX));
        if(i < 5) DEBUG_PRINT(":");
    }
    DEBUG_PRINTLN();
    #endif
    
    if (handInstance && len == sizeof(ESPNowPacket)) {
        ESPNowPacket packet;
        memcpy(&packet, data, sizeof(packet));
        
        #ifdef DEBUG_VERBOSE_HAND_ESPNOW
        // 添加数据包详细信息
        DEBUG_HAND_ESPNOW_PRINTF("Packet details - Magic: 0x%04X, HandID: %d, CmdType: %d, Seq: %lu\n", 
                                 packet.magic, packet.handId, packet.commandType, packet.sequence);
        #endif
        
        handInstance->handleCommand(mac_addr, packet);
    } else {
        DEBUG_HAND_ESPNOW_PRINTF("Invalid packet size or instance null. Expected: %d, Got: %d\n", 
                                 sizeof(ESPNowPacket), len);
    }
}

void HandESPNow::handleCommand(const uint8_t *mac_addr, const ESPNowPacket& packet) {
    #ifdef DEBUG_VERBOSE_HAND_ESPNOW
    DEBUG_HAND_ESPNOW_PRINT("Processing command from MAC: ");
    for(int i = 0; i < 6; i++) {
        if(mac_addr[i] < 16) Serial.print("0");
        Serial.print(mac_addr[i], HEX);
        if(i < 5) Serial.print(":");
    }
    Serial.println();
    #endif
    
    // 验证魔数和校验和
    if (packet.magic != ESPNOW_MAGIC) {
        DEBUG_HAND_ESPNOW_PRINTF("Invalid magic number. Expected: 0x%04X, Got: 0x%04X\n", 
                                 ESPNOW_MAGIC, packet.magic);
        return;
    }
    
    if (!verifyChecksum(&packet)) {
        DEBUG_HAND_ESPNOW_PRINT("Checksum verification failed");
        return;
    }
    
    // 处理广播命令（不需要验证handId）
    if (packet.commandType == CMD_BRAIN_DISCOVERY || packet.commandType == CMD_REQUEST_REGISTRATION) {
        lastCommandTime = millis();
        
        switch (packet.commandType) {
            case CMD_BRAIN_DISCOVERY:
                DEBUG_HAND_ESPNOW_PRINT("Processing CMD_BRAIN_DISCOVERY");
                processBrainDiscovery(packet);
                break;
            case CMD_REQUEST_REGISTRATION:
                DEBUG_HAND_ESPNOW_PRINT("Processing CMD_REQUEST_REGISTRATION");
                processRegistrationRequest(packet);
                break;
        }
        return;
    }
    
    // 对于其他命令，验证目标手部ID（或处理注册状态）
    if (packet.handId != HAND_ID && packet.handId != 0xFF) {
        DEBUG_HAND_ESPNOW_PRINTF("Packet for different hand. Expected: %d, Got: %d\n", 
                                 HAND_ID, packet.handId);
        return;
    }
    
    lastCommandTime = millis();
    brainOnline = true;
    
    DEBUG_HAND_ESPNOW_PRINTF("Valid command received - Type: %d, Sequence: %lu, Angle: %d, FeedLength: %d\n", 
                             packet.commandType, packet.sequence, packet.angle, packet.feedLength);
    
    // 处理命令
    switch (packet.commandType) {
        case CMD_SERVO_SET_ANGLE:
            DEBUG_HAND_ESPNOW_PRINT("Processing CMD_SERVO_SET_ANGLE");
            processServoAngle(packet);
            break;
        case CMD_FEEDER_ADVANCE:
            DEBUG_HAND_ESPNOW_PRINT("Processing CMD_FEEDER_ADVANCE");
            processFeederAdvance(packet);
            break;
        case CMD_STATUS_REQUEST:
            DEBUG_HAND_ESPNOW_PRINT("Processing CMD_STATUS_REQUEST");
            processStatusRequest(packet);
            break;
        case CMD_HEARTBEAT:
            DEBUG_HAND_ESPNOW_PRINT(F("Processing CMD_HEARTBEAT"));
            processHeartbeat(packet);
            break;
        case CMD_CHECK_FEEDBACK:
            DEBUG_HAND_ESPNOW_PRINT("Processing CMD_CHECK_FEEDBACK");
            processCheckFeedback(packet);
            break;
        case CMD_ENABLE_FEEDBACK:
            DEBUG_HAND_ESPNOW_PRINT("Processing CMD_ENABLE_FEEDBACK");
            processEnableFeedback(packet);
            break;
        default:
            DEBUG_HAND_ESPNOW_PRINTF("Unknown command type: %d\n", packet.commandType);
            sendResponse(packet, STATUS_ERROR, "Unknown command");
            break;
    }
}

void HandESPNow::processServoAngle(const ESPNowPacket& packet) {
    DEBUG_HAND_ESPNOW_PRINTF("Setting servo angle to: %d° with pulse range: %d-%d\n", 
                             packet.angle, packet.pulseMin, packet.pulseMax);
    
    // 只在舵机未连接或需要更改脉宽时才重新连接
    if (!servoController->isAttached() && packet.pulseMin > 0 && packet.pulseMax > 0) {
        DEBUG_HAND_ESPNOW_PRINT("Requesting servo attach with custom pulse range");
        servoController->requestAttach(packet.pulseMin, packet.pulseMax);
    }
    
    // 异步设置角度
    DEBUG_HAND_ESPNOW_PRINT("Requesting servo angle change");
    servoController->requestSetAngle(packet.angle);
    
    sendResponse(packet, STATUS_OK, "Angle command queued");
    DEBUG_HAND_ESPNOW_PRINT("Servo angle command response sent");
}

void HandESPNow::processFeederAdvance(const ESPNowPacket& packet) {
    DEBUG_HAND_ESPNOW_PRINTF("Feeder advance request - Length: %d\n", packet.feedLength);
    uint8_t feedLength = packet.feedLength;
    if (feedLength > 0) {
        DEBUG_HAND_ESPNOW_PRINT("Setting servo to 80 degrees for advance");
        // 推进到指定角度
        servoController->requestSetAngle(feederAdvanceAngle);
        // 设置回缩任务
        feederRetractPending = true;
        feederRetractTime = millis() + feederRetractDelay;
        sendResponse(packet, STATUS_OK, "Advance command queued");
        DEBUG_HAND_ESPNOW_PRINT("Advance response sent, retract scheduled");
    } else {
        DEBUG_HAND_ESPNOW_PRINT("No feed needed (length = 0)");
        sendResponse(packet, STATUS_OK, "No feed needed");
    }
}

void HandESPNow::processStatusRequest(const ESPNowPacket& packet) {
    DEBUG_HAND_ESPNOW_PRINT("Status request received");
    
    String status = F("Hand ");
    status += HAND_ID;
    status += F(" OK, servo: ");
    status += servoController->isAttached() ? F("attached") : F("detached");
    status += F(", angle: ");
    status += servoController->getCurrentAngle();
    
    DEBUG_HAND_ESPNOW_PRINTF("Sending status: %s\n", status.c_str());
    
    sendResponse(packet, STATUS_OK, status.c_str());
}

void HandESPNow::processHeartbeat(const ESPNowPacket& packet) {
    DEBUG_HAND_ESPNOW_PRINTF("Heartbeat received from brain, seq: %lu\n", packet.sequence);
    
    sendResponse(packet, STATUS_OK, "Heartbeat ACK");
    DEBUG_HAND_ESPNOW_PRINT("Heartbeat response sent");
}

bool HandESPNow::addBroadcastPeer() {
    int result = esp_now_add_peer(broadcast_mac, ESP_NOW_ROLE_CONTROLLER, 1, NULL, 0);
    return result == 0;
}

bool HandESPNow::sendRegistration() {
    ESPNowPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = ESPNOW_MAGIC;
    packet.handId = HAND_ID;
    packet.commandType = CMD_HAND_REGISTER;
    packet.feederId = feederId;
    packet.sequence = millis();
    setPacketChecksum(&packet);
    
    int result = esp_now_send(broadcast_mac, (uint8_t*)&packet, sizeof(packet));
    
    DEBUG_HAND_ESPNOW_PRINTF("Sending registration - Feeder ID: %d, Result: %s\n", 
                             feederId, result == 0 ? "OK" : "FAIL");
    
    if (result == 0) {
        registered = true;
        brainOnline = true;
    }
    
    return result == 0;
}

void HandESPNow::processBrainDiscovery(const ESPNowPacket& packet) {
    DEBUG_HAND_ESPNOW_PRINT("Brain discovery received");
    
    if (feederId == 255) {
        DEBUG_HAND_ESPNOW_PRINT("Feeder ID not configured, ignoring discovery");
        return;
    }
    
    // 发送注册响应（带随机延迟）
    scheduleRegistration();
}

void HandESPNow::processRegistrationRequest(const ESPNowPacket& packet) {
    DEBUG_HAND_ESPNOW_PRINT("Registration request received");
    
    if (feederId == 255) {
        DEBUG_HAND_ESPNOW_PRINT("Feeder ID not configured, ignoring request");
        return;
    }
    
    // 发送注册响应（带随机延迟）
    scheduleRegistration();
}

void HandESPNow::scheduleRegistration() {
    // 生成0-2000ms的随机延迟
    unsigned long delay_ms = random(0, 2001);
    
    DEBUG_HAND_ESPNOW_PRINTF("Scheduling registration with %lums delay\n", delay_ms);
    
    // 非阻塞延迟实现
    registrationScheduledTime = millis() + delay_ms;
    registrationPending = true;
}

void HandESPNow::processScheduledRegistration() {
    if (registrationPending && millis() >= registrationScheduledTime) {
        registrationPending = false;
        sendRegistration();
    }
}

void HandESPNow::setFeederId(uint8_t id) {
    feederId = id;
    registered = false; // 重置注册状态
    saveFeederId(); // 保存到EEPROM
    DEBUG_HAND_ESPNOW_PRINTF("Feeder ID set to: %d\n", feederId);
}

uint8_t HandESPNow::getFeederId() {
    return feederId;
}

void HandESPNow::saveFeederId() {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.write(FEEDER_ID_ADDR, feederId);
    EEPROM.commit();
    DEBUG_HAND_ESPNOW_PRINTF("Feeder ID %d saved to EEPROM\n", feederId);
}

void HandESPNow::loadFeederId() {
    uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);
    if (magic == EEPROM_MAGIC_VALUE) {
        feederId = EEPROM.read(FEEDER_ID_ADDR);
        DEBUG_HAND_ESPNOW_PRINTF("Loaded feeder ID from EEPROM: %d\n", feederId);
    } else {
        feederId = 255; // 未配置状态
        DEBUG_HAND_ESPNOW_PRINT("No valid feeder ID in EEPROM, using default (255)");
    }
}

void HandESPNow::checkBrainConnection() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 5000) return; // 每5秒检查一次
    
    lastCheck = millis();
    
    if (millis() - lastCommandTime > 10000) { // 10秒没有收到命令
        if (brainOnline) {
            brainOnline = false;
            DEBUG_HAND_ESPNOW_PRINT("Brain connection lost");
        }
    }
}

// 反馈系统相关处理函数
void HandESPNow::processCheckFeedback(const ESPNowPacket& packet) {
    DEBUG_HAND_ESPNOW_PRINT("Processing feedback check request");
    
    if (!feedbackManager) {
        DEBUG_HAND_ESPNOW_PRINT("Feedback manager not available");
        sendResponse(packet, STATUS_ERROR, "Feedback manager not available");
        return;
    }
    
    // 发送当前反馈状态
    if (sendFeedbackStatus()) {
        sendResponse(packet, STATUS_OK, "Feedback status sent");
        DEBUG_HAND_ESPNOW_PRINT("Feedback status response sent");
    } else {
        sendResponse(packet, STATUS_ERROR, "Failed to send feedback status");
        DEBUG_HAND_ESPNOW_PRINT("Failed to send feedback status");
    }
}

void HandESPNow::processEnableFeedback(const ESPNowPacket& packet) {
    DEBUG_HAND_ESPNOW_PRINTF("Processing feedback enable/disable: %d\n", packet.feedLength);
    
    if (!feedbackManager) {
        DEBUG_HAND_ESPNOW_PRINT("Feedback manager not available");
        sendResponse(packet, STATUS_ERROR, "Feedback manager not available");
        return;
    }
    
    bool enable = (packet.feedLength == 1);
    bool clearFlags = (packet.angle == 1);
    
    if (clearFlags) {
        // 清除手动进料标志
        feedbackManager->clearManualFeedFlag();
        sendResponse(packet, STATUS_OK, "Manual feed flag cleared");
        DEBUG_HAND_ESPNOW_PRINT("Manual feed flag cleared");
    } else {
        // 启用/禁用反馈
        feedbackManager->enableFeedback(enable);
        const char* action = enable ? "enabled" : "disabled";
        String message = String("Feedback ") + action;
        sendResponse(packet, STATUS_OK, message.c_str());
        DEBUG_HAND_ESPNOW_PRINTF("Feedback %s\n", action);
    }
}

bool HandESPNow::sendFeedbackStatus() {
    if (!feedbackManager) {
        DEBUG_HAND_ESPNOW_PRINT("Cannot send feedback status - manager not available");
        return false;
    }
    
    ESPNowFeedbackPacket feedbackPacket;
    memset(&feedbackPacket, 0, sizeof(feedbackPacket));
    
    // 填充数据包
    feedbackPacket.magic = ESPNOW_MAGIC;
    feedbackPacket.handId = HAND_ID;
    feedbackPacket.commandType = CMD_FEEDBACK_STATUS;
    feedbackPacket.feederId = feederId;
    feedbackPacket.sequence = millis();
    
    // 获取反馈状态
    HandFeedbackManager::FeedbackStatus status = feedbackManager->getStatus();
    feedbackPacket.tapeLoaded = status.tapeLoaded ? 1 : 0;
    feedbackPacket.feedbackEnabled = status.feedbackEnabled ? 1 : 0;
    feedbackPacket.manualFeedDetected = status.manualFeedDetected ? 1 : 0;
    feedbackPacket.errorCount = status.errorCount;
    feedbackPacket.lastCheckTime = status.lastCheckTime;
    
    // 计算校验和
    feedbackPacket.checksum = 0;
    uint8_t* data = (uint8_t*)&feedbackPacket;
    uint16_t checksum = 0;
    for (size_t i = 0; i < sizeof(feedbackPacket) - sizeof(feedbackPacket.checksum); i++) {
        checksum += data[i];
    }
    feedbackPacket.checksum = checksum;
    
    // 发送数据包
    int result = esp_now_send(broadcast_mac, (uint8_t*)&feedbackPacket, sizeof(feedbackPacket));
    
    if (result == 0) {
        DEBUG_HAND_ESPNOW_PRINTF("Feedback status sent - Tape: %s, Enabled: %s, Manual: %s, Errors: %d\n",
                                 feedbackPacket.tapeLoaded ? "LOADED" : "NOT_LOADED",
                                 feedbackPacket.feedbackEnabled ? "YES" : "NO", 
                                 feedbackPacket.manualFeedDetected ? "YES" : "NO",
                                 feedbackPacket.errorCount);
        return true;
    } else {
        DEBUG_HAND_ESPNOW_PRINTF("Failed to send feedback status: %d\n", result);
        return false;
    }
}
