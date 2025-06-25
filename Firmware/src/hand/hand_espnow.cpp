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

// 信道发现相关全局变量
volatile bool discoveryResponseReceived = false;
volatile uint8_t discoveredChannel = 0;
uint8_t currentScanChannel = 1;

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

        // 检查是否为发现响应
        if (response->commandType == CMD_RESPONSE &&
            strcmp(response->message, "Brain Found") == 0)
        {
            discoveryResponseReceived = true;
            discoveredChannel = currentScanChannel;
            DEBUG_PRINTF("Discovery response received on channel %d\n", currentScanChannel);
        }
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
    // 设置为Station模式但不连接WiFi，仅用于ESP-NOW通信
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();

    DEBUG_PRINTF("MAC address: %s\n", WiFi.macAddress().c_str());
    DEBUG_PRINTLN("ESP-NOW initializing without WiFi connection...");

    quickEspNow.onDataRcvd(dataReceived);

    // 自动扫描并找到Brain所在的信道
    DEBUG_PRINTLN("Starting channel discovery...");
    if (scanForBrainChannel())
    {
        DEBUG_PRINTF("Brain found on channel %d\n", discoveredChannel);
        DEBUG_PRINTLN("ESP-NOW initialized successfully");
    }
    else
    {
        DEBUG_PRINTLN("Failed to find Brain, using default channel 6");
        quickEspNow.begin(6); // 失败时使用默认频道6
    }

    DEBUG_PRINTLN("ESP-NOW initialized on channel 6");
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
        DEBUG_PRINTF("Command not for this feeder (ID:%d, received:%d)\n",
                     myFeederID, receivedFeederID);
        return;
    }

    DEBUG_PRINTF("Processing command: Type=0x%02X, FeederID=%d, Length=%d\n",
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
        DEBUG_PRINTF("Unknown command type: 0x%02X\n", receivedCommandType);
        // sendErrorResponse(receivedFeederID, STATUS_INVALID_PARAM, "Unknown command");
        break;
    }
}

// 处理喂料推进命令
void handleFeederAdvanceCommand(uint8_t feederID, uint8_t feedLength)
{
    DEBUG_PRINTF("Handling feeder advance: ID=%d, Length=%d\n", feederID, feedLength);

    // 这里添加实际的喂料逻辑
    // 例如：控制步进电机，检查传感器等
    feedTapeAction(feedLength);

    // 不直接发送响应，而是设置待发送的响应状态
    schedulePendingResponse(feederID, STATUS_OK, "Feed OK");
}

// 处理心跳命令
void handleHeartbeatCommand()
{
    DEBUG_PRINTLN("Received heartbeat, sending response");

    uint8_t myFeederID = getCurrentFeederID();
    schedulePendingResponse(myFeederID, STATUS_OK, "Online");
}

// 调度待发送的响应
void schedulePendingResponse(uint8_t feederID, uint8_t status, const char *message)
{
    pendingResponseFeederID = feederID;
    pendingResponseStatus = status;
    strncpy((char *)pendingResponseMessage, message, sizeof(pendingResponseMessage) - 1);
    pendingResponseMessage[sizeof(pendingResponseMessage) - 1] = '\0';
    hasPendingResponse = true;

    DEBUG_PRINTF("Scheduled response: FeederID=%d, Status=0x%02X, Message=%s\n",
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

    DEBUG_PRINTF("Processing pending response: FeederID=%d, Status=0x%02X, Message=%s\n",
                 pendingResponseFeederID, pendingResponseStatus, (char *)pendingResponseMessage);

    // 根据状态发送相应的响应
    if (pendingResponseStatus == STATUS_OK)
    {
        sendSuccessResponse(pendingResponseFeederID, (char *)pendingResponseMessage);
    }
    else
    {
        sendErrorResponse(pendingResponseFeederID, pendingResponseStatus, (char *)pendingResponseMessage);
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

    DEBUG_PRINTF("Hand sending ESPNowResponse:\n");
    DEBUG_PRINTF("  Size: %d bytes\n", sizeof(response));
    DEBUG_PRINTF("  Command Type: 0x%02X\n", response.commandType);
    DEBUG_PRINTF("  Hand ID: %d\n", response.handId);
    DEBUG_PRINTF("  Status: 0x%02X\n", response.status);
    DEBUG_PRINTF("  Message: %s\n", response.message);

    bool result = quickEspNow.send(DEST_ADDR, (uint8_t *)&response, sizeof(response));
    if (!result)
    {
        DEBUG_PRINTF("Success response sent from feeder %d\n", getCurrentFeederID());
    }
    else
    {
        DEBUG_PRINTF("Failed to send success response from feeder %d\n", getCurrentFeederID());
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
        DEBUG_PRINTF("Error response sent for feeder %d\n", feederID);
    }
    else
    {
        DEBUG_PRINTF("Failed to send error response for feeder %d\n", feederID);
    }
}

// =============================================================================
// 信道发现功能实现
// =============================================================================

// 扫描所有信道寻找Brain
bool scanForBrainChannel()
{
    // 扫描信道1-13（2.4GHz WiFi信道）
    for (uint8_t channel = 1; channel <= 13; channel++)
    {
        DEBUG_PRINTF("Scanning channel %d...\n", channel);

        if (tryChannelDiscovery(channel))
        {
            discoveredChannel = channel;
            return true;
        }

        // 每个信道间隔一点时间，避免射频冲突
        delay(100);
    }

    return false; // 未找到Brain
}

// 在指定信道上尝试发现Brain
bool tryChannelDiscovery(uint8_t channel)
{
    currentScanChannel = channel;
    discoveryResponseReceived = false;

    // 在指定信道上初始化ESP-NOW
    quickEspNow.begin(channel);
    delay(50); // 等待ESP-NOW初始化完成

    // 发送发现请求
    sendDiscoveryRequest();

    // 等待响应
    return waitForDiscoveryResponse(500); // 500ms超时
}

// 发送发现请求
void sendDiscoveryRequest()
{
    ESPNowPacket discoveryPacket;
    discoveryPacket.commandType = CMD_DISCOVERY;
    discoveryPacket.feederId = getCurrentFeederID();
    discoveryPacket.feedLength = 0;
    memset(discoveryPacket.reserved, 0, sizeof(discoveryPacket.reserved));

    DEBUG_PRINTF("Sending discovery request on channel %d\n", currentScanChannel);

    bool result = quickEspNow.send(DEST_ADDR, (uint8_t *)&discoveryPacket, sizeof(discoveryPacket));
    if (result)
    {
        DEBUG_PRINTF("Failed to send discovery request on channel %d\n", currentScanChannel);
    }
}

// 等待发现响应
bool waitForDiscoveryResponse(uint32_t timeoutMs)
{
    uint32_t startTime = millis();

    while (millis() - startTime < timeoutMs)
    {
        if (discoveryResponseReceived)
        {
            return true;
        }
        delay(10); // 短暂延迟，让系统处理其他任务
    }

    return false; // 超时
}