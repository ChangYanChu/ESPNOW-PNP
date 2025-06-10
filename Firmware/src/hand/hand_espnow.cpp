#include "hand_espnow.h"
#include "feeder_id_manager.h"
#include <Arduino.h>
#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#define WIFI_MODE_STA WIFI_STA
#else
#error "Unsupported platform"
#endif // ESP32
#include <QuickEspNow.h>
#include "common/espnow_protocol.h"
#include "hand_servo.h"

// 全局变量用于存储接收到的命令
volatile bool hasNewCommand = false;
volatile uint8_t receivedCommandType = 0;
volatile uint8_t receivedFeederID = 0;
volatile uint8_t receivedFeedLength = 0;
volatile uint32_t commandTimestamp = 0;

// 全局变量用于响应状态管理
volatile bool hasPendingResponse = false;
volatile uint8_t pendingResponseFeederID = 0;
volatile uint8_t pendingResponseStatus = 0;
volatile char pendingResponseMessage[16] = {0};

static const String msg = "Hello esp-now!";

#define USE_BROADCAST 1 // Set this to 1 to use broadcast communication

#if USE_BROADCAST != 1
// set the MAC address of the receiver for unicast
static uint8_t receiver[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12};
#define DEST_ADDR receiver
#else // USE_BROADCAST != 1
#define DEST_ADDR ESPNOW_BROADCAST_ADDRESS
#endif // USE_BROADCAST != 1

void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast)
{
    Serial.print("Received: ");
    Serial.printf("%.*s\n", len, data);
    Serial.printf("RSSI: %d dBm\n", rssi);
    Serial.printf("From: " MACSTR "\n", MAC2STR(address));
    Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");

    // 检查数据长度是否符合协议包大小
    if (len == sizeof(ESPNowResponse))
    {
        ESPNowResponse *response = (ESPNowResponse *)data;

        Serial.printf("Command Type: 0x%02X\n", response->commandType);
        Serial.printf("Hand ID: %d\n", response->handId);
        Serial.printf("Status: 0x%02X\n", response->status);
        Serial.printf("Message: %.16s\n", response->message);
    }
    else if (len == sizeof(ESPNowPacket))
    {
        // 如果是命令包，存储到全局变量中
        ESPNowPacket *packet = (ESPNowPacket *)data;
        Serial.printf("Received command: Type=0x%02X, FeederID=%d, Length=%d\n",
                      packet->commandType, packet->feederId, packet->feedLength);

        // 将接收到的命令数据存储到全局变量
        receivedCommandType = packet->commandType;
        receivedFeederID = packet->feederId;
        receivedFeedLength = packet->feedLength;
        commandTimestamp = millis();
        hasNewCommand = true; // 设置新命令标志
    }
    else
    {
        Serial.printf("Unknown packet size: %d bytes\n", len);
    }
}

void espnow_setup()
{
    // 设置为Station模式但不连接WiFi，仅用于ESP-NOW通信
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    
    Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
    Serial.println("ESP-NOW initializing without WiFi connection...");
    
    quickEspNow.onDataRcvd(dataReceived);
    quickEspNow.begin(6); // 使用固定频道6启动ESP-NOW，与Brain端保持一致
    
    Serial.println("ESP-NOW initialized on channel 6");
}

void esp_update()
{
    static time_t lastSend = 60000;
    static unsigned int counter = 0;

    if (millis() - lastSend >= 1000)
    {
        lastSend = millis();
        String message = String(msg) + " " + String(counter++);
        if (!quickEspNow.send(DEST_ADDR, (uint8_t *)message.c_str(), message.length()))
        {
            Serial.printf(">>>>>>>>>> Message sent\n");
        }
        else
        {
            Serial.printf(">>>>>>>>>> Message not sent\n");
        }
    }
}

// 处理接收到的命令
void processReceivedCommand()
{
    if (!hasNewCommand)
    {
        return; // 没有新命令需要处理
    }

    // 清除新命令标志
    hasNewCommand = false;

    // 检查命令是否针对本设备
    uint8_t myFeederID = getCurrentFeederID();
    if (receivedFeederID != myFeederID && receivedFeederID != 0xFF)
    { // 0xFF为广播ID
        Serial.printf("Command not for this feeder (ID:%d, received:%d)\n",
                      myFeederID, receivedFeederID);
        return;
    }

    Serial.printf("Processing command: Type=0x%02X, FeederID=%d, Length=%d\n",
                  receivedCommandType, receivedFeederID, receivedFeedLength);

    switch (receivedCommandType)
    {
    case CMD_FEEDER_ADVANCE:
        handleFeederAdvanceCommand(receivedFeederID, receivedFeedLength);
        break;

    case CMD_HEARTBEAT:
        handleHeartbeatCommand();
        break;

    default:
        Serial.printf("Unknown command type: 0x%02X\n", receivedCommandType);
        // sendErrorResponse(receivedFeederID, STATUS_INVALID_PARAM, "Unknown command");
        break;
    }
}

// 处理喂料推进命令
void handleFeederAdvanceCommand(uint8_t feederID, uint8_t feedLength)
{
    Serial.printf("Handling feeder advance: ID=%d, Length=%d\n", feederID, feedLength);

    // 这里添加实际的喂料逻辑
    // 例如：控制步进电机，检查传感器等
    feedTapeAction(feedLength);
    
    // 模拟处理时间
    delay(100);

    // 不直接发送响应，而是设置待发送的响应状态
    schedulePendingResponse(feederID, STATUS_OK, "Feed OK");
}

// 处理心跳命令
void handleHeartbeatCommand()
{
    Serial.println("Received heartbeat, sending response");
    
    uint8_t myFeederID = getCurrentFeederID();
    schedulePendingResponse(myFeederID, STATUS_OK, "Online");
}

// 调度待发送的响应
void schedulePendingResponse(uint8_t feederID, uint8_t status, const char *message)
{
    pendingResponseFeederID = feederID;
    pendingResponseStatus = status;
    strncpy((char*)pendingResponseMessage, message, sizeof(pendingResponseMessage) - 1);
    pendingResponseMessage[sizeof(pendingResponseMessage) - 1] = '\0';
    hasPendingResponse = true;
    
    Serial.printf("Scheduled response: FeederID=%d, Status=0x%02X, Message=%s\n", 
                  feederID, status, message);
}

// 处理待发送的响应（在主循环中调用）
void processPendingResponse()
{
    if (!hasPendingResponse)
    {
        return; // 没有待发送的响应
    }

    // 清除待发送标志
    hasPendingResponse = false;

    Serial.printf("Processing pending response: FeederID=%d, Status=0x%02X, Message=%s\n",
                  pendingResponseFeederID, pendingResponseStatus, (char*)pendingResponseMessage);

    // 根据状态发送相应的响应
    if (pendingResponseStatus == STATUS_OK)
    {
        sendSuccessResponse(pendingResponseFeederID, (char*)pendingResponseMessage);
    }
    else
    {
        sendErrorResponse(pendingResponseFeederID, pendingResponseStatus, (char*)pendingResponseMessage);
    }
}

// 发送成功响应
void sendSuccessResponse(uint8_t feederID, const char *message)
{
    ESPNowResponse response;
    response.handId = getCurrentFeederID(); // 使用当前设备的Feeder ID
    response.commandType = CMD_RESPONSE;
    response.status = STATUS_OK;
    response.sequence = 0;
    response.timestamp = millis();
    strncpy(response.message, message, sizeof(response.message) - 1);
    response.message[sizeof(response.message) - 1] = '\0';

    Serial.printf("Hand sending ESPNowResponse:\n");
    Serial.printf("  Size: %d bytes\n", sizeof(response));
    Serial.printf("  Command Type: 0x%02X\n", response.commandType);
    Serial.printf("  Hand ID: %d\n", response.handId);
    Serial.printf("  Status: 0x%02X\n", response.status);
    Serial.printf("  Message: %s\n", response.message);

    bool result = quickEspNow.send(DEST_ADDR, (uint8_t *)&response, sizeof(response));
    if (!result)
    {
        Serial.printf("Success response sent from feeder %d\n", getCurrentFeederID());
    }
    else
    {
        Serial.printf("Failed to send success response from feeder %d\n", getCurrentFeederID());
    }
}

// 发送错误响应
void sendErrorResponse(uint8_t feederID, uint8_t errorCode, const char *message)
{
    ESPNowResponse response;
    response.handId = feederID;
    response.commandType = CMD_RESPONSE;
    response.status = errorCode;
    response.sequence = 0;
    response.timestamp = millis();
    strncpy(response.message, message, sizeof(response.message) - 1);
    response.message[sizeof(response.message) - 1] = '\0';

    bool result = quickEspNow.send(DEST_ADDR, (uint8_t *)&response, sizeof(response));
    if (!result)
    {
        Serial.printf("Error response sent for feeder %d\n", feederID);
    }
    else
    {
        Serial.printf("Failed to send error response for feeder %d\n", feederID);
    }
}