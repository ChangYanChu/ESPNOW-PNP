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
    DEBUG_PRINT("Received: ");
    DEBUG_PRINTF("%.*s\n", len, data);
    DEBUG_PRINTF("RSSI: %d dBm\n", rssi);
    DEBUG_PRINTF("From: " MACSTR "\n", MAC2STR(address));
    DEBUG_PRINTF("%s\n", broadcast ? "Broadcast" : "Unicast");

    // 检查数据长度是否符合协议包大小
    if (len == sizeof(ESPNowResponse))
    {
        ESPNowResponse *response = (ESPNowResponse *)data;

        DEBUG_PRINTF("Command Type: 0x%02X\n", response->commandType);
        DEBUG_PRINTF("Hand ID: %d\n", response->handId);
        DEBUG_PRINTF("Status: 0x%02X\n", response->status);
        DEBUG_PRINTF("Message: %.16s\n", response->message);

        // 这里可以处理其他类型的响应，目前暂时保留
        (void)response; // 避免未使用变量警告
    }
    else if (len == sizeof(ESPNowPacket))
    {
        // 如果是命令包，存储到全局变量中
        ESPNowPacket *packet = (ESPNowPacket *)data;
        DEBUG_PRINTF("Received command: Type=0x%02X, FeederID=%d, Length=%d\n",
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
        DEBUG_PRINTF("Unknown packet size: %d bytes\n", len);
    }
}

void espnow_setup()
{
    // 简单WiFi连接设置
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    DEBUG_PRINTLN("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG_PRINT(".");
    }
    
    DEBUG_PRINTF("WiFi Connected: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("MAC: %s\n", WiFi.macAddress().c_str());
    
    // 初始化ESP-NOW
    quickEspNow.onDataRcvd(dataReceived);
    quickEspNow.begin(WiFi.channel());
    
    DEBUG_PRINTF("ESP-NOW initialized on channel %d\n", WiFi.channel());
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
            DEBUG_PRINTF(">>>>>>>>>> Message sent\n");
        }
        else
        {
            DEBUG_PRINTF(">>>>>>>>>> Message not sent\n");
        }
    }
}

// 处理接收到的命令 - 简化版本
void processReceivedCommand()
{
    if (!hasNewCommand) return;
    
    hasNewCommand = false;
    uint8_t myFeederID = getCurrentFeederID();
    
    // 检查命令是否针对本设备（广播255或匹配ID）
    if (receivedFeederID != myFeederID && receivedFeederID != 255) {
        return;
    }
    
    DEBUG_PRINTF("Processing cmd: Type=0x%02X, ID=%d, Len=%d\n",
                 receivedCommandType, receivedFeederID, receivedFeedLength);
    
    switch (receivedCommandType) {
        case CMD_FEEDER_ADVANCE:
            DEBUG_PRINTF("Feed command: %d mm\n", receivedFeedLength);
            feedTapeAction(receivedFeedLength);
            schedulePendingResponse(myFeederID, STATUS_OK, "Feed OK");
            break;
            
        case CMD_HEARTBEAT:
            DEBUG_PRINTLN("Heartbeat received");
            schedulePendingResponse(myFeederID, STATUS_OK, "Online");
            break;
            
        case CMD_SET_FEEDER_ID:
            DEBUG_PRINTF("Set ID command: %d\n", receivedFeedLength);
            if (setFeederIDRemotely(receivedFeedLength)) {
                schedulePendingResponse(receivedFeedLength, STATUS_OK, "ID Set");
            } else {
                schedulePendingResponse(myFeederID, STATUS_ERROR, "ID Failed");
            }
            break;
            
        default:
            DEBUG_PRINTF("Unknown command: 0x%02X\n", receivedCommandType);
            break;
    }
}

// 简化的响应调度
void schedulePendingResponse(uint8_t feederID, uint8_t status, const char *message)
{
    pendingResponseFeederID = feederID;
    pendingResponseStatus = status;
    strncpy((char *)pendingResponseMessage, message, sizeof(pendingResponseMessage) - 1);
    pendingResponseMessage[sizeof(pendingResponseMessage) - 1] = '\0';
    hasPendingResponse = true;
}

// 简化的响应处理
void processPendingResponse()
{
    if (!hasPendingResponse) return;
    
    hasPendingResponse = false;
    
    DEBUG_PRINTF("Sending response: ID=%d, Status=%d, Msg=%s\n",
                 pendingResponseFeederID, pendingResponseStatus, (char *)pendingResponseMessage);
    
    // 创建响应包
    ESPNowResponse response;
    response.handId = pendingResponseFeederID;
    response.commandType = CMD_RESPONSE;
    response.status = pendingResponseStatus;
    response.sequence = 0;
    response.timestamp = millis();
    strncpy(response.message, (char *)pendingResponseMessage, sizeof(response.message) - 1);
    response.message[sizeof(response.message) - 1] = '\0';
    
    // 发送响应
    quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, (uint8_t *)&response, sizeof(response));
}