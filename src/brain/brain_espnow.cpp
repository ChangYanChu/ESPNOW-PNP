#include "brain_espnow.h"
#include "brain_config.h"
#include "gcode.h"
#include "lcd.h"
#include "../common/espnow_protocol.h"
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

// 全局变量用于存储接收到的响应
volatile bool hasNewResponse = false;
volatile uint8_t receivedHandID = 0;
volatile uint8_t receivedCommandType = 0;
volatile uint8_t receivedStatus = 0;
volatile uint32_t responseTimestamp = 0;
volatile char receivedMessage[16] = {0};

// 超时管理变量
volatile bool waitingForResponse = false;
volatile uint32_t commandSentTime = 0;
volatile uint8_t pendingFeederID = 0;
#define COMMAND_TIMEOUT_MS 5000  // 5秒超时

// Hand在线状态管理（最简单实现）
#define HAND_OFFLINE_TIMEOUT 30000  // 30秒无响应视为离线
#define HEARTBEAT_INTERVAL 10000    // 10秒发送一次心跳
static uint32_t lastHandResponse[TOTAL_FEEDERS] = {0}; // 记录每个Hand最后响应时间
static uint32_t lastHeartbeatTime = 0;              // 最后心跳时间

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
    // Serial.print("Brain Received ESP-NOW data: ");
    // Serial.printf("Length=%d bytes, RSSI=%d dBm\n", len, rssi);
    // Serial.printf("From: " MACSTR "\n", MAC2STR(address));
    // Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");

    // 检查数据长度是否符合响应包大小
    if (len == sizeof(ESPNowResponse))
    {
        ESPNowResponse *response = (ESPNowResponse *)data;
        
        // Serial.printf("Brain received ESPNowResponse:\n");
        // Serial.printf("  Command Type: 0x%02X\n", response->commandType);
        // Serial.printf("  Hand ID: %d\n", response->handId);
        // Serial.printf("  Status: 0x%02X\n", response->status);
        // Serial.printf("  Message: %.16s\n", response->message);

        // 将接收到的响应数据存储到全局变量
        receivedHandID = response->handId;
        receivedCommandType = response->commandType;
        receivedStatus = response->status;
        responseTimestamp = millis();
        strncpy((char*)receivedMessage, response->message, sizeof(receivedMessage) - 1);
        receivedMessage[sizeof(receivedMessage) - 1] = '\0';
        hasNewResponse = true; // 设置新响应标志
        
        // 更新Hand在线状态 - 记录响应时间
        if (response->handId < TOTAL_FEEDERS) {
            lastHandResponse[response->handId] = millis();
        }
        
        // Serial.printf("Brain stored response data, hasNewResponse=true\n");
    }
    else
    {
        // Serial.printf("Brain: Unknown packet size: %d bytes (expected %d for ESPNowResponse)\n", 
                    //  len, sizeof(ESPNowResponse));
    }
}

// 处理接收到的响应
void processReceivedResponse()
{
    if (!hasNewResponse)
    {
        return; // 没有新响应需要处理
    }

    // 清除新响应标志
    hasNewResponse = false;

    // Serial.printf("Processing response: HandID=%d, Type=0x%02X, Status=0x%02X, Message=%s\n",
    //               receivedHandID, receivedCommandType, receivedStatus, receivedMessage);

    // 处理CMD_RESPONSE类型的响应
    if (receivedCommandType == CMD_RESPONSE)
    {
        // 检查是否为心跳响应（通过消息内容判断）
        if (strcmp((char*)receivedMessage, "Online") == 0)
        {
            // 心跳响应，只记录时间，不发送G-code响应，不清除等待状态
            // Serial.printf("Heartbeat response from Hand %d\n", receivedHandID);
            // 触发心跳动画更新
            triggerHeartbeatAnimation();
        }
        else
        {
            // 喂料命令响应，清除等待状态
            if (waitingForResponse) {
                waitingForResponse = false;
            }
            
            if (receivedStatus == STATUS_OK)
            {
                // 喂料完成成功，发送OK响应
                sendAnswer(0, F("Feed completed"));
            }
            else
            {
                // 喂料失败，发送错误响应
                sendAnswer(1, String("Feed error: ") + String((char*)receivedMessage));
            }
        }
    }
    else if (receivedCommandType == CMD_HEARTBEAT)
    {
        // 心跳响应（备用方案，如果直接使用CMD_HEARTBEAT响应）
        // Serial.printf("Direct heartbeat response from Hand %d\n", receivedHandID);
        triggerHeartbeatAnimation();
    }
    else
    {
        // Serial.printf("Unknown response command type: 0x%02X\n", receivedCommandType);
    }
}

// 检查命令超时
void checkCommandTimeout()
{
    if (!waitingForResponse) {
        return; // 没有等待中的命令
    }
    
    uint32_t now = millis();
    if (now - commandSentTime > COMMAND_TIMEOUT_MS) {
        // 命令超时
        waitingForResponse = false;
        // Serial.printf("Command timeout for feeder ID %d\n", pendingFeederID);
        sendAnswer(1, F("Feed timeout"));
    }
}

// 获取在线Hand数量（最简单实现）
int getOnlineHandCount()
{
    uint32_t now = millis();
    int onlineCount = 0;
    
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        if (lastHandResponse[i] > 0 && (now - lastHandResponse[i] < HAND_OFFLINE_TIMEOUT)) {
            onlineCount++;
        }
    }
    
    return onlineCount;
}

// 发送心跳包检测Hand在线状态
void sendHeartbeat()
{
    uint32_t now = millis();
    if (now - lastHeartbeatTime < HEARTBEAT_INTERVAL) {
        return; // 还没到发送时间
    }
    
    lastHeartbeatTime = now;
    
    // 创建心跳包
    ESPNowPacket heartbeat;
    heartbeat.commandType = CMD_HEARTBEAT;
    heartbeat.feederId = 0xFF;      // 广播给所有Hand
    heartbeat.feedLength = 0;
    memset(heartbeat.reserved, 0, sizeof(heartbeat.reserved));
    
    // 广播心跳包
    if (!quickEspNow.send(DEST_ADDR, (uint8_t*)&heartbeat, sizeof(heartbeat))) {
        // Serial.println("Heartbeat sent to all hands");
    } else {
        // Serial.println("Failed to send heartbeat");
    }
}

void espnow_setup()
{
    // 设置为Station模式但不连接WiFi，仅用于ESP-NOW通信
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    
    // Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
    // Serial.println("ESP-NOW initializing without WiFi connection...");
    
    // 直接更新LCD显示ESP-NOW就绪状态
    lcd_update_system_status(SYSTEM_ESPNOW_READY);
    
    quickEspNow.onDataRcvd(dataReceived);
    quickEspNow.begin(6); // 使用固定频道6启动ESP-NOW
    
    // Serial.println("ESP-NOW initialized on channel 6");
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
            // 发送成功 直接调用 Gcode  sendAnswer(0, message);
            sendAnswer(0, F("Feeder advance command sent"));
        }
        else
        {
            // Serial.printf(">>>>>>>>>> Message not sent\n");
        }
    }
}

// 增加发送喂料推进命令的函数
bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength)
{
    // 如果已经有命令在等待响应，拒绝新命令
    if (waitingForResponse) {
        // Serial.printf("Cannot send new command - already waiting for response from feeder %d\n", pendingFeederID);
        sendAnswer(1, F("Feed busy"));
        return false;
    }

    // 打印
    // Serial.printf("Sending feeder advance command: Feeder ID: %d, Feed Length: %d\n", feederId, feedLength);
    // 创建ESP-NOW数据包
    ESPNowPacket packet;
    packet.commandType = CMD_FEEDER_ADVANCE;
    packet.feederId = feederId;
    packet.feedLength = feedLength;
    // 清零保留字段
    memset(packet.reserved, 0, sizeof(packet.reserved));

    // 发送数据包
    bool result = quickEspNow.send(DEST_ADDR, (uint8_t *)&packet, sizeof(packet));

    if (!result)
    {
        // ESP-NOW发送成功，开始等待响应
        waitingForResponse = true;
        commandSentTime = millis();
        pendingFeederID = feederId;
        // Serial.printf("Feeder advance command sent successfully, waiting for response...\n");
        return true;
    }
    else
    {
        // ESP-NOW发送失败，立即返回错误
        sendAnswer(1, F("Send failed"));
        return false;
    }
}