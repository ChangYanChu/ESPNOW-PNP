#include "hand_udp.h"
#include "feeder_id_manager.h"
#include "hand_servo.h"
#include "hand_config.h"
#include "hand_led.h"

#if defined ESP32
#include <WiFi.h>
#include <WiFiUdp.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define WIFI_MODE_STA WIFI_STA
#else
#error "Unsupported platform"
#endif

// =============================================================================
// 全局变量
// =============================================================================

UDPConnectionState udpState = UDP_STATE_DISCONNECTED;
BrainInfo connectedBrain = {IPAddress(0, 0, 0, 0), 0, 0, false, ""};
UDPStats udpStats = {0};

// UDP对象
WiFiUDP udp;
WiFiUDP discoveryUdp;

// 时间戳变量
uint32_t lastDiscoveryTime = 0;
uint32_t lastHeartbeatTime = 0;
uint32_t lastBrainCheckTime = 0;

// 接收缓冲区 - 优化大小
uint8_t udpBuffer[UDP_BUFFER_SIZE];

// 全局变量用于存储接收到的命令（保持与原ESP-NOW兼容）
volatile bool hasNewCommand = false;
volatile uint8_t receivedCommandType = 0;
volatile uint8_t receivedFeederID = 0;
volatile uint8_t receivedFeedLength = 0;
volatile uint32_t commandTimestamp = 0;
volatile uint32_t receivedSequence = 0;  // 保存收到的命令序列号

// 全局变量用于响应状态管理
volatile bool hasPendingResponse = false;
volatile uint8_t pendingResponseFeederID = 0;
volatile uint8_t pendingResponseStatus = 0;
volatile char pendingResponseMessage[16] = {0};

// =============================================================================
// 核心UDP函数实现
// =============================================================================

void udp_setup() {
    // 初始化LED
    initLED();
    setLEDStatus(LED_STATUS_WIFI_CONNECTING);
    
    // WiFi连接设置
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    DEBUG_PRINTLN("UDP: 连接WiFi中...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG_PRINT(".");
        handleLED(); // 处理连接中的LED闪烁
    }

    DEBUG_PRINTF("UDP: WiFi已连接: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("UDP: MAC地址: %s\n", WiFi.macAddress().c_str());

    // WiFi连接成功，根据是否分配ID设置状态
    uint8_t currentFeederID = getCurrentFeederID();
    if (currentFeederID == 255) {
        setLEDStatus(LED_STATUS_WIFI_CONNECTED); // 未分配ID
    } else {
        setLEDStatus(LED_STATUS_READY); // 已分配ID，就绪状态
    }

    // 优化WiFi性能设置
    optimizeWiFiSettings();

    // 初始化UDP
    if (udp.begin(UDP_HAND_PORT)) {
        DEBUG_PRINTF("UDP: Hand端监听端口 %d\n", UDP_HAND_PORT);
    } else {
        DEBUG_PRINTLN("UDP: Hand端端口初始化失败");
    }

    if (discoveryUdp.begin(UDP_DISCOVERY_PORT)) {
        DEBUG_PRINTF("UDP: 发现服务监听端口 %d\n", UDP_DISCOVERY_PORT);
    } else {
        DEBUG_PRINTLN("UDP: 发现端口初始化失败");
    }

    udpState = UDP_STATE_DISCONNECTED;
    resetUDPStats();

    DEBUG_PRINTLN("UDP: 初始化完成");
}

void udp_update() {
    uint32_t now = millis();

    // 处理接收到的UDP数据
    processUDPData();

    // 状态机处理
    switch (udpState) {
        case UDP_STATE_DISCONNECTED:
            // 尝试发现Brain
            if (now - lastDiscoveryTime > UDP_DISCOVERY_INTERVAL_MS) {
                DEBUG_PRINTLN("UDP: 开始发现Brain...");
                udpState = UDP_STATE_DISCOVERING;
                lastDiscoveryTime = now;
                sendDiscoveryRequest();
            }
            break;

        case UDP_STATE_DISCOVERING:
            // 发现超时检查
            if (now - lastDiscoveryTime > UDP_DISCOVERY_TIMEOUT_MS) {
                DEBUG_PRINTLN("UDP: 发现超时，重新尝试");
                udpState = UDP_STATE_DISCONNECTED;
            }
            break;

        case UDP_STATE_CONNECTED:
            // 发送心跳
            if (now - lastHeartbeatTime > UDP_HEARTBEAT_INTERVAL_MS) {
                sendHeartbeat();
                lastHeartbeatTime = now;
            }
            
            // 检查Brain连接状态
            if (now - lastBrainCheckTime > 30000) { // 30秒检查一次
                checkBrainConnection();
                lastBrainCheckTime = now;
            }
            break;

        case UDP_STATE_ERROR:
            // 错误状态，回到断开状态重新发现
            DEBUG_PRINTLN("UDP: 错误状态，重置连接");
            udpState = UDP_STATE_DISCONNECTED;
            connectedBrain.isActive = false;
            break;
    }
}

bool discoverBrain() {
    return sendDiscoveryRequest();
}

bool sendCommandAndWaitResponse(const ESPNowPacket& command, ESPNowResponse& response, uint32_t timeoutMs) {
    if (udpState != UDP_STATE_CONNECTED || !connectedBrain.isActive) {
        DEBUG_PRINTLN("UDP: Brain未连接，无法发送命令");
        return false;
    }

    // 创建UDP命令包
    UDPCommandPacket udpCommand;
    udpCommand.packetType = UDP_PKT_COMMAND;
    udpCommand.sequence = generateSequence();
    udpCommand.timestamp = getCurrentTimestamp();
    udpCommand.command = command;

    // 发送命令
    udp.beginPacket(connectedBrain.ip, connectedBrain.port);
    udp.write((uint8_t*)&udpCommand, sizeof(udpCommand));
    bool sent = udp.endPacket();

    if (!sent) {
        DEBUG_PRINTLN("UDP: 命令发送失败");
        udpStats.errors++;
        return false;
    }

    udpStats.commandsSent++;
    DEBUG_PRINTF("UDP: 命令已发送 seq=%u cmd=0x%02X\n", udpCommand.sequence, command.commandType);

    // 等待响应
    uint32_t startTime = millis();
    while (millis() - startTime < timeoutMs) {
        processUDPData();
        delay(10);
        
        // 这里需要检查是否收到了对应sequence的响应
        // 简化处理：如果有任何响应就返回
        if (hasNewCommand) {
            hasNewCommand = false;
            // 构造响应（简化处理）
            response.handId = getCurrentFeederID();
            response.commandType = CMD_RESPONSE;
            response.status = STATUS_OK;
            response.sequence = udpCommand.sequence;
            response.timestamp = millis();
            strcpy(response.message, "OK");
            udpStats.responsesReceived++;
            return true;
        }
    }

    DEBUG_PRINTLN("UDP: 命令响应超时");
    udpStats.timeouts++;
    return false;
}

void sendHeartbeat() {
    if (udpState != UDP_STATE_CONNECTED || !connectedBrain.isActive) {
        return;
    }

    UDPHeartbeatPacket heartbeat;
    heartbeat.packetType = UDP_PKT_HEARTBEAT;
    heartbeat.deviceId = getCurrentFeederID();
    heartbeat.timestamp_low = getTimestampLow();  // 使用优化后的低16位时间戳
    heartbeat.status = 0; // 正常状态

    udp.beginPacket(connectedBrain.ip, connectedBrain.port);
    udp.write((uint8_t*)&heartbeat, sizeof(heartbeat));
    bool sent = udp.endPacket();

    if (sent) {
        udpStats.heartbeatsSent++;
        DEBUG_PRINTF("UDP: 心跳已发送到 %s:%d\n", 
                     connectedBrain.ip.toString().c_str(), connectedBrain.port);
    } else {
        DEBUG_PRINTLN("UDP: 心跳发送失败");
        udpStats.errors++;
    }
}

// =============================================================================
// 内部UDP处理函数实现
// =============================================================================

void processUDPData() {
    // 处理主UDP端口的数据
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        IPAddress remoteIP = udp.remoteIP();
        uint16_t remotePort = udp.remotePort();
        
        size_t len = udp.read(udpBuffer, sizeof(udpBuffer));
        if (len > 0) {
            printUDPPacket(udpBuffer, len, true);
            
            // 处理不同类型的包
            UDPPacketType packetType = (UDPPacketType)udpBuffer[0];
            
            switch (packetType) {
                case UDP_PKT_RESPONSE:
                    if (len >= sizeof(UDPResponsePacket)) {
                        handleBusinessResponse(*(UDPResponsePacket*)udpBuffer);
                    }
                    break;
                    
                case UDP_PKT_COMMAND:
                    if (len >= sizeof(UDPCommandPacket)) {
                        UDPCommandPacket* cmdPkt = (UDPCommandPacket*)udpBuffer;
                        // 将UDP命令转换为原有的ESP-NOW命令格式
                        receivedCommandType = cmdPkt->command.commandType;
                        receivedFeederID = cmdPkt->command.feederId;
                        receivedFeedLength = cmdPkt->command.feedLength;
                        receivedSequence = cmdPkt->sequence;  // 保存命令序列号
                        commandTimestamp = millis();
                        hasNewCommand = true;
                        DEBUG_PRINTF("UDP: 接收到命令 seq=%u cmd=0x%02X id=%d len=%d\n", 
                                   receivedSequence, receivedCommandType, receivedFeederID, receivedFeedLength);
                    }
                    break;
                    
                case UDP_PKT_HEARTBEAT:
                    if (len >= sizeof(UDPHeartbeatPacket)) {
                        handleBrainHeartbeat(*(UDPHeartbeatPacket*)udpBuffer, remoteIP);
                    }
                    break;
                    
                default:
                    DEBUG_PRINTF("UDP: 主端口收到未知包类型 0x%02X\n", packetType);
                    break;
            }
        }
    }

    // 处理发现端口的数据
    packetSize = discoveryUdp.parsePacket();
    if (packetSize > 0) {
        IPAddress remoteIP = discoveryUdp.remoteIP();
        
        size_t len = discoveryUdp.read(udpBuffer, sizeof(udpBuffer));
        if (len > 0 && udpBuffer[0] == UDP_PKT_DISCOVERY_RESPONSE) {
            if (len >= sizeof(UDPDiscoveryResponse)) {
                handleDiscoveryResponse(*(UDPDiscoveryResponse*)udpBuffer, remoteIP);
            }
        }
    }
}

void handleDiscoveryResponse(const UDPDiscoveryResponse& response, IPAddress fromIP) {
    DEBUG_PRINTF("UDP: 收到Brain发现响应 从 %s:%d\n", 
                 fromIP.toString().c_str(), response.brainPort);
    
    // 更新Brain信息
    connectedBrain.ip = fromIP;  // 使用实际发送方IP，而不是包中的IP
    connectedBrain.port = response.brainPort;
    connectedBrain.lastSeen = millis();
    connectedBrain.isActive = true;
    strncpy(connectedBrain.info, response.brainInfo, sizeof(connectedBrain.info) - 1);
    connectedBrain.info[sizeof(connectedBrain.info) - 1] = '\0';
    
    udpState = UDP_STATE_CONNECTED;
    udpStats.discoveryResponses++;
    
    DEBUG_PRINTF("UDP: 已连接到Brain %s:%d (%s)\n", 
                 connectedBrain.ip.toString().c_str(), 
                 connectedBrain.port,
                 connectedBrain.info);
}

void handleBusinessResponse(const UDPResponsePacket& response) {
    DEBUG_PRINTF("UDP: 收到业务响应 seq=%u status=0x%02X\n", 
                 response.sequence, response.response.status);
    udpStats.responsesReceived++;
    
    // 这里可以根据sequence匹配对应的请求
    // 简化处理：设置标志表示收到响应
    hasNewCommand = true; // 复用原有标志
}

void handleBrainHeartbeat(const UDPHeartbeatPacket& heartbeat, IPAddress fromIP) {
    // 验证心跳来源是否为已连接的Brain
    if (connectedBrain.isActive && connectedBrain.ip == fromIP) {
        connectedBrain.lastSeen = millis();
        DEBUG_PRINTF("UDP: 收到Brain心跳 来自 %s\n", fromIP.toString().c_str());
    } else {
        DEBUG_PRINTF("UDP: 收到未知Brain心跳 来自 %s\n", fromIP.toString().c_str());
    }
}

bool sendDiscoveryRequest() {
    UDPDiscoveryRequest request;
    request.packetType = UDP_PKT_DISCOVERY_REQUEST;
    request.handId = getCurrentFeederID();
    request.timestamp_low = getTimestampLow();  // 使用优化后的低16位时间戳
    snprintf(request.handInfo, sizeof(request.handInfo), "Hand-%d", request.handId);

    // 广播发现请求
    IPAddress broadcastIP = WiFi.localIP();
    broadcastIP[3] = 255; // 设置为广播地址

    discoveryUdp.beginPacket(broadcastIP, UDP_DISCOVERY_PORT);
    discoveryUdp.write((uint8_t*)&request, sizeof(request));
    bool sent = discoveryUdp.endPacket();

    if (sent) {
        udpStats.discoveryRequests++;
        DEBUG_PRINTF("UDP: 发现请求已广播到 %s:%d\n", 
                     broadcastIP.toString().c_str(), UDP_DISCOVERY_PORT);
    } else {
        DEBUG_PRINTLN("UDP: 发现请求发送失败");
        udpStats.errors++;
    }

    return sent;
}

void checkBrainConnection() {
    if (!connectedBrain.isActive) {
        return;
    }

    // 检查最后通信时间
    uint32_t now = millis();
    if (now - connectedBrain.lastSeen > 60000) { // 60秒无通信
        DEBUG_PRINTLN("UDP: Brain连接超时");
        connectedBrain.isActive = false;
        udpState = UDP_STATE_DISCONNECTED;
    }
}

// =============================================================================
// 兼容函数实现（保持与原ESP-NOW接口兼容）
// =============================================================================

// 处理接收到的命令 - 保持与原来相同的接口
void processReceivedCommand() {
    if (!hasNewCommand) return;

    hasNewCommand = false;
    uint8_t myFeederID = getCurrentFeederID();

    // 检查命令是否针对本设备
    if (receivedFeederID != myFeederID && receivedFeederID != 255) {
        return;
    }

    DEBUG_PRINTF("UDP: 处理命令 Type=0x%02X, ID=%d, Len=%d\n",
                 receivedCommandType, receivedFeederID, receivedFeedLength);

    switch (receivedCommandType) {
        case CMD_FEEDER_ADVANCE:
            DEBUG_PRINTF("UDP: 喂料命令: %d mm\n", receivedFeedLength);
            feedTapeAction(receivedFeedLength);
            schedulePendingResponse(myFeederID, STATUS_OK, "Feed OK");
            break;

        case CMD_HEARTBEAT:
            DEBUG_PRINTLN("UDP: 收到心跳");
            schedulePendingResponse(myFeederID, STATUS_OK, "Online");
            break;

        case CMD_SET_FEEDER_ID:
            DEBUG_PRINTF("UDP: 设置ID命令: %d\n", receivedFeedLength);
            if (setFeederIDRemotely(receivedFeedLength)) {
                setLEDStatus(LED_STATUS_READY); // ID设置成功，设为就绪状态
                schedulePendingResponse(receivedFeedLength, STATUS_OK, "ID Set");
            } else {
                schedulePendingResponse(myFeederID, STATUS_ERROR, "ID Failed");
            }
            break;

        case CMD_FIND_ME:
            DEBUG_PRINTLN("UDP: Find Me命令");
            startFindMe(10); // 闪烁10秒
            schedulePendingResponse(myFeederID, STATUS_OK, "Find Me");
            break;

        default:
            DEBUG_PRINTF("UDP: 未知命令: 0x%02X\n", receivedCommandType);
            break;
    }
}

// 调度响应 - 保持与原来相同的接口
void schedulePendingResponse(uint8_t feederID, uint8_t status, const char *message) {
    pendingResponseFeederID = feederID;
    pendingResponseStatus = status;
    strncpy((char *)pendingResponseMessage, message, sizeof(pendingResponseMessage) - 1);
    pendingResponseMessage[sizeof(pendingResponseMessage) - 1] = '\0';
    hasPendingResponse = true;
}

// 处理待发送的响应 - 保持与原来相同的接口
void processPendingResponse() {
    if (!hasPendingResponse || udpState != UDP_STATE_CONNECTED || !connectedBrain.isActive) {
        return;
    }

    hasPendingResponse = false;

    DEBUG_PRINTF("UDP: 发送响应: ID=%d, Status=%d, Msg=%s\n",
                 pendingResponseFeederID, pendingResponseStatus, (char *)pendingResponseMessage);

    // 创建UDP响应包
    UDPResponsePacket udpResponse;
    udpResponse.packetType = UDP_PKT_RESPONSE;
    udpResponse.sequence = receivedSequence;  // 使用原始命令的序列号
    udpResponse.timestamp = getCurrentTimestamp();
    
    // 填充业务响应
    udpResponse.response.handId = pendingResponseFeederID;
    udpResponse.response.commandType = CMD_RESPONSE;
    udpResponse.response.status = pendingResponseStatus;
    udpResponse.response.sequence = udpResponse.sequence;
    udpResponse.response.timestamp = udpResponse.timestamp;
    strncpy(udpResponse.response.message, (char *)pendingResponseMessage, sizeof(udpResponse.response.message) - 1);
    udpResponse.response.message[sizeof(udpResponse.response.message) - 1] = '\0';

    // 发送响应
    udp.beginPacket(connectedBrain.ip, connectedBrain.port);
    udp.write((uint8_t*)&udpResponse, sizeof(udpResponse));
    bool sent = udp.endPacket();

    if (sent) {
        DEBUG_PRINTF("UDP: 响应已发送到 %s:%d\n", 
                     connectedBrain.ip.toString().c_str(), connectedBrain.port);
    } else {
        DEBUG_PRINTLN("UDP: 响应发送失败");
        udpStats.errors++;
    }
}

// =============================================================================
// 调试和统计函数实现
// =============================================================================

void printUDPStatus() {
    DEBUG_PRINTLN("=== UDP状态信息 ===");
    DEBUG_PRINTF("状态: %s\n", getUDPStateString());
    DEBUG_PRINTF("本地IP: %s\n", WiFi.localIP().toString().c_str());
    
    if (connectedBrain.isActive) {
        DEBUG_PRINTF("Brain: %s:%d (%s)\n", 
                     connectedBrain.ip.toString().c_str(),
                     connectedBrain.port,
                     connectedBrain.info);
        DEBUG_PRINTF("最后通信: %lu ms前\n", millis() - connectedBrain.lastSeen);
    } else {
        DEBUG_PRINTLN("Brain: 未连接");
    }
    
    DEBUG_PRINTLN("=== 统计信息 ===");
    DEBUG_PRINTF("发现请求: %u\n", udpStats.discoveryRequests);
    DEBUG_PRINTF("发现响应: %u\n", udpStats.discoveryResponses);
    DEBUG_PRINTF("命令发送: %u\n", udpStats.commandsSent);
    DEBUG_PRINTF("响应接收: %u\n", udpStats.responsesReceived);
    DEBUG_PRINTF("心跳发送: %u\n", udpStats.heartbeatsSent);
    DEBUG_PRINTF("超时次数: %u\n", udpStats.timeouts);
    DEBUG_PRINTF("错误次数: %u\n", udpStats.errors);
}

void resetUDPStats() {
    memset(&udpStats, 0, sizeof(udpStats));
}

const char* getUDPStateString() {
    switch (udpState) {
        case UDP_STATE_DISCONNECTED: return "断开";
        case UDP_STATE_DISCOVERING: return "发现中";
        case UDP_STATE_CONNECTED: return "已连接";
        case UDP_STATE_ERROR: return "错误";
        default: return "未知";
    }
}
