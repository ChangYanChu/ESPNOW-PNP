#include "brain_espnow.h"
#include "brain_config.h"
#include "brain_web.h"
#include "gcode.h"
#include "lcd.h"
#include "../common/espnow_protocol.h"
#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
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
volatile uint32_t currentTimeoutMs = 5000; // 添加动态超时变量

// Hand在线状态管理（最简单实现）
#define HAND_OFFLINE_TIMEOUT 30000                     // 30秒无响应视为离线
#define HEARTBEAT_INTERVAL 10000                       // 10秒发送一次心跳
uint32_t lastHandResponse[TOTAL_FEEDERS] = {0}; // 记录每个Hand最后响应时间
static uint32_t lastHeartbeatTime = 0;                 // 最后心跳时间

static const String msg = "Hello esp-now!";

#define USE_BROADCAST 1 // Set this to 1 to use broadcast communication

#if USE_BROADCAST != 1
// set the MAC address of the receiver for unicast
static uint8_t receiver[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12};
#define DEST_ADDR receiver
#else // USE_BROADCAST != 1
#define DEST_ADDR ESPNOW_BROADCAST_ADDRESS
#endif // USE_BROADCAST != 1

// 超时管理变量 - 改为按设备管理
FeederStatus feederStatusArray[NUMBER_OF_FEEDER];

// 全局统计变量
uint32_t totalSessionFeeds = 0;    // 本次开机总送料次数
uint32_t totalWorkCount = 0;       // 总作业次数

// 未分配Hand管理（简化版，避免性能占用）
struct UnassignedHand {
    uint8_t macAddr[6];
    uint32_t lastSeen;
    bool active;
};

#define MAX_UNASSIGNED_HANDS 10
UnassignedHand unassignedHands[MAX_UNASSIGNED_HANDS];

// 添加或更新未分配Hand记录
void addUnassignedHand(uint8_t* macAddr) {
    // 先查找是否已存在
    for (int i = 0; i < MAX_UNASSIGNED_HANDS; i++) {
        if (unassignedHands[i].active && 
            memcmp(unassignedHands[i].macAddr, macAddr, 6) == 0) {
            unassignedHands[i].lastSeen = millis();
            return;
        }
    }
    
    // 查找空槽位
    for (int i = 0; i < MAX_UNASSIGNED_HANDS; i++) {
        if (!unassignedHands[i].active) {
            memcpy(unassignedHands[i].macAddr, macAddr, 6);
            unassignedHands[i].lastSeen = millis();
            unassignedHands[i].active = true;
            return;
        }
    }
    
    // 如果没有空槽位，替换最老的记录
    int oldestIndex = 0;
    uint32_t oldestTime = unassignedHands[0].lastSeen;
    for (int i = 1; i < MAX_UNASSIGNED_HANDS; i++) {
        if (unassignedHands[i].lastSeen < oldestTime) {
            oldestTime = unassignedHands[i].lastSeen;
            oldestIndex = i;
        }
    }
    
    memcpy(unassignedHands[oldestIndex].macAddr, macAddr, 6);
    unassignedHands[oldestIndex].lastSeen = millis();
    unassignedHands[oldestIndex].active = true;
}

void initFeederStatus()
{
    for (int i = 0; i < NUMBER_OF_FEEDER; i++)
    {
        feederStatusArray[i].waitingForResponse = false;
        feederStatusArray[i].commandSentTime = 0;
        feederStatusArray[i].timeoutMs = 5000;
        // 初始化新增字段
        feederStatusArray[i].totalFeedCount = 0;
        feederStatusArray[i].sessionFeedCount = 0;
        feederStatusArray[i].totalPartCount = 0;
        feederStatusArray[i].remainingPartCount = 0;
        snprintf(feederStatusArray[i].componentName, sizeof(feederStatusArray[i].componentName), "N%d", i);
        strcpy(feederStatusArray[i].packageType, "Unknown");
    }
    
    // 初始化未分配Hand列表
    for (int i = 0; i < MAX_UNASSIGNED_HANDS; i++) {
        unassignedHands[i].active = false;
        unassignedHands[i].lastSeen = 0;
        memset(unassignedHands[i].macAddr, 0, 6);
    }
    
    // 加载配置
    loadFeederConfig();
}

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
        strncpy((char *)receivedMessage, response->message, sizeof(receivedMessage) - 1);
        receivedMessage[sizeof(receivedMessage) - 1] = '\0';
        hasNewResponse = true; // 设置新响应标志

        // 更新Hand在线状态 - 记录响应时间
        if (response->handId < TOTAL_FEEDERS)
        {
            // 检查是否是新上线的Hand
            bool wasOffline = (lastHandResponse[response->handId] == 0 || 
                             (millis() - lastHandResponse[response->handId] >= HAND_OFFLINE_TIMEOUT));
            
            lastHandResponse[response->handId] = millis();
            
            // 如果是新上线，通知Web界面
            if (wasOffline) {
                notifyHandOnline(response->handId);
            }
        }

        // Serial.printf("Brain stored response data, hasNewResponse=true\n");
    }
    else if (len == sizeof(ESPNowPacket))
    {
        // 处理来自Hand的命令包（如发现请求）
        ESPNowPacket *packet = (ESPNowPacket *)data;
        
        // Serial.printf("Brain received ESPNowPacket:\n");
        // Serial.printf("  Command Type: 0x%02X\n", packet->commandType);
        // Serial.printf("  Feeder ID: %d\n", packet->feederId);
        
        if (packet->commandType == CMD_DISCOVERY)
        {
            // 检查是否为未分配的Hand（ID=255）
            if (packet->feederId == 255) {
                // 未分配的Hand，记录其MAC地址
                addUnassignedHand(address);
                Serial.printf("Unassigned Hand discovered: " MACSTR "\n", MAC2STR(address));
            }
            
            // 处理发现请求，立即发送响应
            handleDiscoveryRequest(packet->feederId);
        }
    }
    else
    {
        // Serial.printf("Brain: Unknown packet size: %d bytes\n", len);
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
        if (strcmp((char *)receivedMessage, "Online") == 0)
        {
// 心跳响应，只记录时间，不发送G-code 响应，不清除等待状态
// Serial.printf("Heartbeat response from Hand %d\n", receivedHandID);
// 触发心跳动画更新
#if HAS_LCD
            triggerHeartbeatAnimation();
#endif
        }
        else
        {
            // 喂料命令响应 - 使用新的按设备管理方式
            uint8_t feederId = receivedHandID;

            // 检查feederId是否有效且在等待响应
            if (feederId < NUMBER_OF_FEEDER && feederStatusArray[feederId].waitingForResponse)
            {
                // 清除该设备的等待状态
                feederStatusArray[feederId].waitingForResponse = false;

                if (receivedStatus == STATUS_OK)
                {
                    // 喂料完成成功，发送带飞达编号的OK响应
                    String response = "Feed N" + String(feederId) + " completed";
                    sendAnswer(0, response);
                    updateFeederStats(feederId, true);  // 更新统计
                    notifyCommandCompleted(feederId, true, "completed");
                }
                else
                {
                    // 喂料失败，发送带飞达编号的错误响应
                    String errorResponse = "Feed N" + String(feederId) + " error: " + String((char *)receivedMessage);
                    sendAnswer(1, errorResponse);
                    updateFeederStats(feederId, false); // 更新统计
                    notifyCommandCompleted(feederId, false, (char *)receivedMessage);
                }
            }
            else
            {
                // Serial.printf("Unexpected response from feeder %d\n", feederId);
            }
        }
    }
    else if (receivedCommandType == CMD_HEARTBEAT)
    {
        // 心跳响应（备用方案，如果直接使用CMD_HEARTBEAT响应）
        // Serial.printf("Direct heartbeat response from Hand %d\n", receivedHandID);
#if HAS_LCD
        triggerHeartbeatAnimation();
#endif
    }
    else
    {
        // Serial.printf("Unknown response command type: 0x%02X\n", receivedCommandType);
    }
}

// 检查命令超时
void checkCommandTimeout()
{
    uint32_t now = millis();

    for (int i = 0; i < NUMBER_OF_FEEDER; i++)
    {
        if (feederStatusArray[i].waitingForResponse)
        {
            if (now - feederStatusArray[i].commandSentTime > feederStatusArray[i].timeoutMs)
            {
                // 命令超时
                feederStatusArray[i].waitingForResponse = false;
                Serial.printf("Feed timeout for feeder %d\n", i);

                // 发送带飞达编号的超时错误响应
                String timeoutResponse = "Feed N" + String(i) + " timeout";
                sendAnswer(1, timeoutResponse);
                notifyCommandCompleted(i, false, "timeout");
            }
        }
    }
}

// 获取在线Hand数量（最简单实现）
int getOnlineHandCount()
{
    uint32_t now = millis();
    int onlineCount = 0;
    static bool lastOnlineStatus[TOTAL_FEEDERS] = {false}; // 记录上次在线状态

    for (int i = 0; i < TOTAL_FEEDERS; i++)
    {
        bool isOnline = (lastHandResponse[i] > 0 && (now - lastHandResponse[i] < HAND_OFFLINE_TIMEOUT));
        
        // 检测状态变化，通知Web界面
        if (isOnline && !lastOnlineStatus[i]) {
            // 从离线变为在线（已在dataReceived中处理）
        } else if (!isOnline && lastOnlineStatus[i]) {
            // 从在线变为离线
            notifyHandOffline(i);
        }
        
        lastOnlineStatus[i] = isOnline;
        if (isOnline) {
            onlineCount++;
        }
    }

    return onlineCount;
}

// 发送心跳包检测Hand在线状态
void sendHeartbeat()
{
    uint32_t now = millis();
    if (now - lastHeartbeatTime < HEARTBEAT_INTERVAL)
    {
        return; // 还没到发送时间
    }

    lastHeartbeatTime = now;

    // 创建心跳包
    ESPNowPacket heartbeat;
    heartbeat.commandType = CMD_HEARTBEAT;
    heartbeat.feederId = 0xFF; // 广播给所有Hand
    heartbeat.feedLength = 0;
    memset(heartbeat.reserved, 0, sizeof(heartbeat.reserved));

    // 广播心跳包
    if (!quickEspNow.send(DEST_ADDR, (uint8_t *)&heartbeat, sizeof(heartbeat)))
    {
        // Serial.println("Heartbeat sent to all hands");
    }
    else
    {
        // Serial.println("Failed to send heartbeat");
    }
}

void espnow_setup()
{
    // 设置为Station模式但不连接WiFi，仅用于ESP-NOW通信
    WiFi.mode(WIFI_MODE_STA);
    // WiFi.disconnect();
    WiFi.begin("HUAWEI-P99", "12345678");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("Connected to %s in channel %d\n", WiFi.SSID().c_str(), WiFi.channel());
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());

// Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
// Serial.println("ESP-NOW initializing without WiFi connection...");

// 直接更新LCD显示ESP-NOW就绪状态
#if HAS_LCD
    lcd_update_system_status(SYSTEM_ESPNOW_READY);
#endif
    quickEspNow.onDataRcvd(dataReceived);
    // quickEspNow.begin(6); // 使用固定频道6启动ESP-NOW
    // quickEspNow.begin(); // 使用固定频道6启动ESP-NOW

     // 确保ESP-NOW使用与WiFi相同的频道
    int wifiChannel = WiFi.channel();
    quickEspNow.begin(wifiChannel);
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

bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength, uint32_t timeoutMs)
{

    // 检查feederId是否有效
    if (feederId >= NUMBER_OF_FEEDER)
    {
        String errorMsg = "Invalid feeder N" + String(feederId);
        sendAnswer(1, errorMsg);
        return false;
    }

    // 如果已经有命令在等待响应，拒绝新命令
    if (feederStatusArray[feederId].waitingForResponse)
    {
        String busyMsg = "Feed N" + String(feederId) + " busy";
        sendAnswer(1, busyMsg);
        return false;
    }

    // 计算动态超时时间
    if (timeoutMs == 0)
    {
        // 基础超时3秒 + 每mm喂料长度额外200ms
        feederStatusArray[feederId].timeoutMs = 3000 + (feedLength * 200);

        // 最小超时3秒，最大超时15秒
        if (feederStatusArray[feederId].timeoutMs < 3000)
            feederStatusArray[feederId].timeoutMs = 3000;
        if (feederStatusArray[feederId].timeoutMs > 15000)
            feederStatusArray[feederId].timeoutMs = 15000;
    }
    else
    {
        feederStatusArray[feederId].timeoutMs = timeoutMs;
    }

    // Serial.printf("Sending feeder advance command: Feeder ID: %d, Feed Length: %d, Timeout: %dms\n",
    //               feederId, feedLength, currentTimeoutMs);

    // 创建ESP-NOW数据包
    ESPNowPacket packet;
    packet.commandType = CMD_FEEDER_ADVANCE;
    packet.feederId = feederId;
    packet.feedLength = feedLength;
    memset(packet.reserved, 0, sizeof(packet.reserved));

    // 发送数据包
    bool result = quickEspNow.send(DEST_ADDR, (uint8_t *)&packet, sizeof(packet));

    if (!result)
    {
        // ESP-NOW发送成功，开始等待该设备的响应
        feederStatusArray[feederId].waitingForResponse = true;
        feederStatusArray[feederId].commandSentTime = millis();
        notifyCommandReceived(feederId, feedLength);
        return true;
    }
    else
    {
        // ESP-NOW发送失败，立即返回错误
        String sendFailMsg = "Send N" + String(feederId) + " failed";
        sendAnswer(1, sendFailMsg);
        return false;
    }
}

// 获取在线Hand详细信息
void getOnlineHandDetails(String &response)
{
    uint32_t now = millis();
    int onlineCount = 0;
    response = "Online Hands: ";

    for (int i = 0; i < TOTAL_FEEDERS; i++)
    {
        if (lastHandResponse[i] > 0 && (now - lastHandResponse[i] < HAND_OFFLINE_TIMEOUT))
        {
            if (onlineCount > 0)
            {
                response += ", ";
            }
            response += "N" + String(i);
            onlineCount++;
        }
    }

    response += " (Total: " + String(onlineCount) + ")";
}

// 处理来自Hand的发现请求
void handleDiscoveryRequest(uint8_t feederID)
{
    // Serial.printf("Received discovery request from Feeder ID: %d\n", feederID);
    
    // 创建发现响应
    ESPNowResponse discoveryResponse;
    discoveryResponse.handId = feederID;
    discoveryResponse.commandType = CMD_RESPONSE;
    discoveryResponse.status = STATUS_OK;
    discoveryResponse.sequence = 0;
    discoveryResponse.timestamp = millis();
    strncpy(discoveryResponse.message, "Brain Found", sizeof(discoveryResponse.message) - 1);
    discoveryResponse.message[sizeof(discoveryResponse.message) - 1] = '\0';
    
    // 立即发送响应
    bool result = quickEspNow.send(DEST_ADDR, (uint8_t *)&discoveryResponse, sizeof(discoveryResponse));
    if (!result) {
        // Serial.printf("Discovery response sent to Feeder ID: %d\n", feederID);
        
        // 更新Hand在线状态
        if (feederID < TOTAL_FEEDERS) {
            lastHandResponse[feederID] = millis();
        }
    } else {
        // Serial.printf("Failed to send discovery response to Feeder ID: %d\n", feederID);
    }
}

// 发送设置Feeder ID命令
bool sendSetFeederIDCommand(uint8_t targetMAC[6], uint8_t newFeederID) {
    ESPNowPacket setIDPacket;
    setIDPacket.commandType = CMD_SET_FEEDER_ID;
    setIDPacket.feederId = 255; // 广播给未分配的Hand
    setIDPacket.feedLength = newFeederID; // 使用feedLength字段传递新ID
    memset(setIDPacket.reserved, 0, sizeof(setIDPacket.reserved));
    
    Serial.printf("Sending ID assignment: ID=%d to " MACSTR "\n", 
                  newFeederID, MAC2STR(targetMAC));
    
    // 发送到指定MAC地址
    bool result = quickEspNow.send(targetMAC, (uint8_t*)&setIDPacket, sizeof(setIDPacket));
    return !result; // QuickEspNow返回0表示成功
}

// 列出未分配的Hand
void listUnassignedHands(String &response) {
    response = "Unassigned Hands:\n";
    uint32_t now = millis();
    int count = 0;
    
    for (int i = 0; i < MAX_UNASSIGNED_HANDS; i++) {
        if (unassignedHands[i].active && 
            (now - unassignedHands[i].lastSeen) < 60000) { // 1分钟内活跃
            
            // 格式化MAC地址
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                   unassignedHands[i].macAddr[0], unassignedHands[i].macAddr[1],
                   unassignedHands[i].macAddr[2], unassignedHands[i].macAddr[3],
                   unassignedHands[i].macAddr[4], unassignedHands[i].macAddr[5]);
            
            response += String(count) + ": " + String(macStr) + "\n";
            count++;
        }
    }
    
    if (count == 0) {
        response += "No unassigned hands found.\n";
    }
}

// =============================================================================
// 配置管理函数（使用Preferences）
// =============================================================================

// 加载Feeder配置
void loadFeederConfig() {
    Preferences prefs;
    if (!prefs.begin("feeder_cfg", true)) { // 只读模式
        Serial.println("Failed to open Preferences, using defaults");
        return;
    }
    
    // 加载全局统计
    totalWorkCount = prefs.getULong("totalWork", 0);
    
    // 加载每个Feeder配置（压缩存储）
    for (int i = 0; i < NUMBER_OF_FEEDER; i++) {
        char key[16];
        
        // 加载基本统计数据
        snprintf(key, sizeof(key), "feed_%d", i);
        feederStatusArray[i].totalFeedCount = prefs.getULong(key, 0);
        
        snprintf(key, sizeof(key), "total_%d", i);
        feederStatusArray[i].totalPartCount = prefs.getUShort(key, 0);
        
        snprintf(key, sizeof(key), "remain_%d", i);
        feederStatusArray[i].remainingPartCount = prefs.getUShort(key, 0);
        
        // 加载字符串数据（仅加载有意义的数据）
        if (feederStatusArray[i].totalFeedCount > 0 || feederStatusArray[i].totalPartCount > 0) {
            snprintf(key, sizeof(key), "name_%d", i);
            String name = prefs.getString(key, "");
            if (name.length() > 0) {
                strncpy(feederStatusArray[i].componentName, name.c_str(), sizeof(feederStatusArray[i].componentName) - 1);
            }
            
            snprintf(key, sizeof(key), "pkg_%d", i);
            String pkg = prefs.getString(key, "");
            if (pkg.length() > 0) {
                strncpy(feederStatusArray[i].packageType, pkg.c_str(), sizeof(feederStatusArray[i].packageType) - 1);
            }
        }
    }
    
    prefs.end();
    Serial.println("Feeder config loaded from Preferences");
}

// 保存Feeder配置（轻量级实现）
void saveFeederConfig() {
    static uint32_t lastSaveTime = 0;
    uint32_t now = millis();
    
    // 限制保存频率，避免频繁写入
    if (now - lastSaveTime < 10000) return; // 10秒最多保存一次
    lastSaveTime = now;
    
    Preferences prefs;
    if (!prefs.begin("feeder_cfg", false)) { // 读写模式
        Serial.println("Failed to open Preferences for saving");
        return;
    }
    
    // 保存全局统计
    prefs.putULong("totalWork", totalWorkCount);
    
    // 保存每个Feeder配置（只保存有意义的数据）
    for (int i = 0; i < NUMBER_OF_FEEDER; i++) {
        if (feederStatusArray[i].totalFeedCount > 0 || 
            feederStatusArray[i].totalPartCount > 0 ||
            strlen(feederStatusArray[i].componentName) > 0) {
            
            char key[16];
            
            snprintf(key, sizeof(key), "feed_%d", i);
            prefs.putULong(key, feederStatusArray[i].totalFeedCount);
            
            snprintf(key, sizeof(key), "total_%d", i);
            prefs.putUShort(key, feederStatusArray[i].totalPartCount);
            
            snprintf(key, sizeof(key), "remain_%d", i);
            prefs.putUShort(key, feederStatusArray[i].remainingPartCount);
            
            snprintf(key, sizeof(key), "name_%d", i);
            prefs.putString(key, String(feederStatusArray[i].componentName));
            
            snprintf(key, sizeof(key), "pkg_%d", i);
            prefs.putString(key, String(feederStatusArray[i].packageType));
        }
    }
    
    prefs.end();
    Serial.println("Feeder config saved to Preferences");
}

// 更新Feeder统计信息
void updateFeederStats(uint8_t feederId, bool success) {
    if (feederId >= NUMBER_OF_FEEDER) return;
    
    if (success) {
        feederStatusArray[feederId].totalFeedCount++;
        feederStatusArray[feederId].sessionFeedCount++;
        totalSessionFeeds++;
        totalWorkCount++;
        
        // 减少剩余零件数量
        if (feederStatusArray[feederId].remainingPartCount > 0) {
            feederStatusArray[feederId].remainingPartCount--;
        }
        
        // 每10次操作保存一次配置
        if (totalWorkCount % 10 == 0) {
            saveFeederConfig();
        }
    }
}