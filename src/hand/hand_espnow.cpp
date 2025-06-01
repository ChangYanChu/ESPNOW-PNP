#include "hand_espnow.h"
#include "hand_servo.h"

// 静态实例指针用于回调
static HandESPNow* handInstance = nullptr;

// Brain控制器MAC地址 (需要根据实际ESP32C3填写)
uint8_t brain_mac_address[6] = {0xDC, 0x06, 0x75, 0xF7, 0x77, 0x98}; // 修改为实际ESP32C3 MAC

HandESPNow::HandESPNow() : servoController(nullptr), brainOnline(false), 
                          lastCommandTime(0), lastHeartbeatResponse(0) {
    handInstance = this;
}

bool HandESPNow::begin(HandServo* servoCtrl) {
    servoController = servoCtrl;
    
    // 设置WiFi模式
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    // ESP8266的ESP-NOW初始化
    if (esp_now_init() != 0) {
        Serial.println(F("ESP-NOW init failed"));
        return false;
    }
    
    // 设置角色为从设备
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    
    // 注册回调函数
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceived);
    
    // 添加Brain为对等设备
    if (!addBrainPeer()) {
        Serial.println(F("Failed to add brain peer"));
        return false;
    }
    
    Serial.print(F("Hand MAC: "));
    Serial.println(WiFi.macAddress());
    Serial.println(F("ESP-NOW Hand initialized"));
    
    return true;
}

void HandESPNow::update() {
    checkBrainConnection();
    
    // 定期发送心跳响应
    if (millis() - lastHeartbeatResponse > HEARTBEAT_RESPONSE_INTERVAL) {
        sendStatus(STATUS_OK, "Hand alive");
        lastHeartbeatResponse = millis();
    }
}

bool HandESPNow::sendStatus(ESPNowStatusCode status, const char* message) {
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
    
    int result = esp_now_send(brain_mac_address, (uint8_t*)&response, sizeof(response));
    
    Serial.print(F("Sending status to brain - Status: "));
    Serial.print(status);
    Serial.print(F(", Message: "));
    Serial.print(message ? message : "");
    Serial.print(F(", Result: "));
    Serial.println(result == 0 ? "OK" : "FAIL");
    
    return result == 0;
}

bool HandESPNow::sendResponse(const ESPNowPacket& originalPacket, ESPNowStatusCode status, const char* message) {
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
    
    int result = esp_now_send(brain_mac_address, (uint8_t*)&response, sizeof(response));
    
    Serial.print(F("Sending response - CmdType: "));
    Serial.print(originalPacket.commandType);
    Serial.print(F(", Seq: "));
    Serial.print(originalPacket.sequence);
    Serial.print(F(", Status: "));
    Serial.print(status);
    Serial.print(F(", Message: "));
    Serial.print(message ? message : "");
    Serial.print(F(", Result: "));
    Serial.println(result == 0 ? "OK" : "FAIL");
    
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
    Serial.print(F("Send status: "));
    Serial.println(sendStatus == 0 ? "Success" : "Fail");
    #endif
}

void HandESPNow::onDataReceived(uint8_t *mac_addr, uint8_t *data, uint8_t len) {
    // 添加接收日志
    Serial.print(F("ESP-NOW data received from: "));
    for(int i = 0; i < 6; i++) {
        if(mac_addr[i] < 16) Serial.print("0");
        Serial.print(mac_addr[i], HEX);
        if(i < 5) Serial.print(":");
    }
    Serial.print(F(", length: "));
    Serial.println(len);
    
    if (handInstance && len == sizeof(ESPNowPacket)) {
        ESPNowPacket packet;
        memcpy(&packet, data, sizeof(packet));
        
        // 添加数据包详细信息
        Serial.print(F("Packet details - Magic: 0x"));
        Serial.print(packet.magic, HEX);
        Serial.print(F(", HandID: "));
        Serial.print(packet.handId);
        Serial.print(F(", CmdType: "));
        Serial.print(packet.commandType);
        Serial.print(F(", Seq: "));
        Serial.println(packet.sequence);
        
        handInstance->handleCommand(mac_addr, packet);
    } else {
        Serial.print(F("Invalid packet size or instance null. Expected: "));
        Serial.print(sizeof(ESPNowPacket));
        Serial.print(F(", Got: "));
        Serial.println(len);
    }
}

void HandESPNow::handleCommand(const uint8_t *mac_addr, const ESPNowPacket& packet) {
    Serial.print(F("Processing command from MAC: "));
    for(int i = 0; i < 6; i++) {
        if(mac_addr[i] < 16) Serial.print("0");
        Serial.print(mac_addr[i], HEX);
        if(i < 5) Serial.print(":");
    }
    Serial.println();
    
    // 验证魔数和校验和
    if (packet.magic != ESPNOW_MAGIC) {
        Serial.print(F("Invalid magic number. Expected: 0x"));
        Serial.print(ESPNOW_MAGIC, HEX);
        Serial.print(F(", Got: 0x"));
        Serial.println(packet.magic, HEX);
        return;
    }
    
    if (!verifyChecksum(&packet)) {
        Serial.println(F("Checksum verification failed"));
        return;
    }
    
    // 验证目标手部ID
    if (packet.handId != HAND_ID) {
        Serial.print(F("Packet for different hand. Expected: "));
        Serial.print(HAND_ID);
        Serial.print(F(", Got: "));
        Serial.println(packet.handId);
        return;
    }
    
    lastCommandTime = millis();
    brainOnline = true;
    
    Serial.print(F("Valid command received - Type: "));
    Serial.print(packet.commandType);
    Serial.print(F(", Sequence: "));
    Serial.print(packet.sequence);
    Serial.print(F(", Angle: "));
    Serial.print(packet.angle);
    Serial.print(F(", FeedLength: "));
    Serial.println(packet.feedLength);
    
    // 处理命令
    switch (packet.commandType) {
        case CMD_SERVO_SET_ANGLE:
            Serial.println(F("Processing CMD_SERVO_SET_ANGLE"));
            processServoAngle(packet);
            break;
        case CMD_FEEDER_ADVANCE:
            Serial.println(F("Processing CMD_FEEDER_ADVANCE"));
            processFeederAdvance(packet);
            break;
        case CMD_STATUS_REQUEST:
            Serial.println(F("Processing CMD_STATUS_REQUEST"));
            processStatusRequest(packet);
            break;
        case CMD_HEARTBEAT:
            Serial.println(F("Processing CMD_HEARTBEAT"));
            processHeartbeat(packet);
            break;
        default:
            Serial.print(F("Unknown command type: "));
            Serial.println(packet.commandType);
            sendResponse(packet, STATUS_ERROR, "Unknown command");
            break;
    }
}

void HandESPNow::processServoAngle(const ESPNowPacket& packet) {
    Serial.print(F("Setting servo angle to: "));
    Serial.print(packet.angle);
    Serial.print(F("° with pulse range: "));
    Serial.print(packet.pulseMin);
    Serial.print(F("-"));
    Serial.println(packet.pulseMax);
    
    // 只在舵机未连接或需要更改脉宽时才重新连接
    if (!servoController->isAttached() && packet.pulseMin > 0 && packet.pulseMax > 0) {
        Serial.println(F("Requesting servo attach with custom pulse range"));
        servoController->requestAttach(packet.pulseMin, packet.pulseMax);
    }
    
    // 异步设置角度
    Serial.println(F("Requesting servo angle change"));
    servoController->requestSetAngle(packet.angle);
    
    sendResponse(packet, STATUS_OK, "Angle command queued");
    Serial.println(F("Servo angle command response sent"));
}

void HandESPNow::processFeederAdvance(const ESPNowPacket& packet) {
    Serial.print(F("Feed advance request - Length: "));
    Serial.println(packet.feedLength);
    
    // 实现喂料推进逻辑，使用异步操作
    uint8_t feedLength = packet.feedLength;
    
    if (feedLength > 0) {
        Serial.println(F("Starting feed advance to 80°"));
        // 异步推进到80度
        servoController->requestSetAngle(80);
        sendResponse(packet, STATUS_OK, "Feed advance started");
        Serial.println(F("Feed advance response sent"));
    } else {
        Serial.println(F("No feed needed (length = 0)"));
        sendResponse(packet, STATUS_OK, "No feed needed");
    }
}

void HandESPNow::processStatusRequest(const ESPNowPacket& packet) {
    Serial.println(F("Status request received"));
    
    String status = F("Hand ");
    status += HAND_ID;
    status += F(" OK, servo: ");
    status += servoController->isAttached() ? F("attached") : F("detached");
    status += F(", angle: ");
    status += servoController->getCurrentAngle();
    
    Serial.print(F("Sending status: "));
    Serial.println(status);
    
    sendResponse(packet, STATUS_OK, status.c_str());
}

void HandESPNow::processHeartbeat(const ESPNowPacket& packet) {
    Serial.print(F("Heartbeat received from brain, seq: "));
    Serial.println(packet.sequence);
    
    sendResponse(packet, STATUS_OK, "Heartbeat ACK");
    Serial.println(F("Heartbeat response sent"));
}

bool HandESPNow::addBrainPeer() {
    int result = esp_now_add_peer(brain_mac_address, ESP_NOW_ROLE_CONTROLLER, 1, NULL, 0);
    return result == 0;
}

void HandESPNow::checkBrainConnection() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 5000) return; // 每5秒检查一次
    
    lastCheck = millis();
    
    if (millis() - lastCommandTime > 10000) { // 10秒没有收到命令
        if (brainOnline) {
            brainOnline = false;
            Serial.println(F("Brain connection lost"));
        }
    }
}
