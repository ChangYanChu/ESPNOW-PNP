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

    WiFi.mode(WIFI_MODE_STA);
    // WiFi.begin("HUAWEI-P61", "12345678");
    // while (WiFi.status() != WL_CONNECTED)
    // {
    //     delay(500);
    //     Serial.print(".");
    // }
    // Serial.printf("Connected to %s in channel %d\n", WiFi.SSID().c_str(), WiFi.channel());
    // Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
    quickEspNow.onDataRcvd(dataReceived);
    quickEspNow.begin(); // 在STA模式和同步发送模式下，不使用任何参数在与WiFi相同的频道启动ESP-NOW
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

    // 发送成功响应
    // sendSuccessResponse(feederID, "Feed OK");
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