#include "brain_udp.h"
#include "gcode.h"
#include "brain_tcp.h"  // 添加TCP支持

// =============================================================================
// 全局变量
// =============================================================================

HandInfo connectedHands[TOTAL_FEEDERS];
uint32_t handDiscoveryCount = 0;
UDPStats brainUdpStats = {0};

// UDP对象
WiFiUDP udp;
WiFiUDP discoveryUdp;

// 时间戳变量
uint32_t lastHeartbeatTime = 0;
uint32_t lastHandCheckTime = 0;

// 接收缓冲区 - 优化大小
uint8_t brainUdpBuffer[UDP_BUFFER_SIZE];

// 命令响应等待映射
struct PendingCommand {
    uint32_t sequence;
    uint8_t feederId;
    uint32_t sentTime;
    uint32_t timeoutMs;
    bool waiting;
    bool needTcpReply;    // 是否需要TCP回复
    uint8_t commandType;  // 命令类型，用于生成回复消息
};

PendingCommand pendingCommands[10]; // 最多同时等待10个命令响应
uint32_t nextSequence = 1;

// =============================================================================
// 兼容ESP-NOW的全局变量定义
// =============================================================================

FeederStatus feederStatusArray[TOTAL_FEEDERS];
uint32_t totalSessionFeeds = 0;
uint32_t totalWorkCount = 0;
uint32_t lastHandResponse[TOTAL_FEEDERS];

UnassignedHand unassignedHands[MAX_UNASSIGNED_HANDS];

// =============================================================================
// 核心UDP函数实现
// =============================================================================

void brain_udp_setup() {
    // WiFi连接设置
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    DEBUG_PRINTLN("Brain UDP: 连接WiFi中...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG_PRINT(".");
    }

    DEBUG_PRINTF("Brain UDP: WiFi已连接: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("Brain UDP: MAC地址: %s\n", WiFi.macAddress().c_str());

    // 优化WiFi性能设置
    optimizeWiFiSettings();

    // 初始化UDP
    if (udp.begin(UDP_BRAIN_PORT)) {
        DEBUG_PRINTF("Brain UDP: 监听端口 %d\n", UDP_BRAIN_PORT);
    } else {
        DEBUG_PRINTLN("Brain UDP: 主端口初始化失败");
    }

    if (discoveryUdp.begin(UDP_DISCOVERY_PORT)) {
        DEBUG_PRINTF("Brain UDP: 发现服务监听端口 %d\n", UDP_DISCOVERY_PORT);
    } else {
        DEBUG_PRINTLN("Brain UDP: 发现端口初始化失败");
    }

    // 初始化Hand信息数组
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        connectedHands[i].ip = IPAddress(0, 0, 0, 0);
        connectedHands[i].port = 0;
        connectedHands[i].lastSeen = 0;
        connectedHands[i].isOnline = false;
        connectedHands[i].feederId = i;
        memset(connectedHands[i].handInfo, 0, sizeof(connectedHands[i].handInfo));
    }

    // 初始化待命令数组
    for (int i = 0; i < 10; i++) {
        pendingCommands[i].waiting = false;
        pendingCommands[i].needTcpReply = false;
        pendingCommands[i].commandType = 0;
    }

    resetBrainUDPStats();
    DEBUG_PRINTLN("Brain UDP: 初始化完成");
}

void brain_udp_update() {
    uint32_t now = millis();

    // 处理接收到的UDP数据
    processBrainUDPData();

    // 定期发送心跳
    if (now - lastHeartbeatTime > UDP_HEARTBEAT_INTERVAL_MS) {
        sendHeartbeatToAllHands();
        lastHeartbeatTime = now;
    }

    // 检查Hand连接状态
    if (now - lastHandCheckTime > UNASSIGNED_HAND_TIMEOUT_MS) { // 30秒检查一次
        checkHandConnections();
        lastHandCheckTime = now;
    }

    // 检查命令超时
    for (int i = 0; i < 10; i++) {
        if (pendingCommands[i].waiting) {        if (now - pendingCommands[i].sentTime > pendingCommands[i].timeoutMs) {
            pendingCommands[i].waiting = false;
            brainUdpStats.timeouts++;
        }
        }
    }
}

bool sendCommandToHand(uint8_t feederId, const ESPNowPacket& command, uint32_t timeoutMs) {
    if (feederId >= TOTAL_FEEDERS || !connectedHands[feederId].isOnline) {
        DEBUG_PRINTF("Brain UDP: Hand %d 未连接\n", feederId);
        return false;
    }

    // 创建UDP命令包
    UDPCommandPacket udpCommand;
    udpCommand.packetType = UDP_PKT_COMMAND;
    udpCommand.sequence = nextSequence++;
    udpCommand.timestamp = getCurrentTimestamp();
    udpCommand.command = command;

    // 记录待命令
    if (timeoutMs > 0) {
        for (int i = 0; i < 10; i++) {
            if (!pendingCommands[i].waiting) {
                pendingCommands[i].sequence = udpCommand.sequence;
                pendingCommands[i].feederId = feederId;
                pendingCommands[i].sentTime = millis();
                pendingCommands[i].timeoutMs = timeoutMs;
                pendingCommands[i].waiting = true;
                pendingCommands[i].needTcpReply = false;    // 默认不需要TCP回复
                pendingCommands[i].commandType = command.commandType;
                break;
            }
        }
    }

    // 发送命令
    udp.beginPacket(connectedHands[feederId].ip, connectedHands[feederId].port);
    udp.write((uint8_t*)&udpCommand, sizeof(udpCommand));
    bool sent = udp.endPacket();

    if (sent) {
        brainUdpStats.commandsSent++;
        // 更新最后通信时间
        connectedHands[feederId].lastSeen = millis();
        
        // 通知Web界面命令已发送
        if (command.commandType == CMD_FEEDER_ADVANCE) {
            notifyCommandReceived(feederId, command.feedLength);
        }
    } else {
        brainUdpStats.errors++;
    }

    return sent;
}

bool sendCommandToHand(uint8_t feederId, const ESPNowPacket& command, uint32_t timeoutMs, bool needTcpReply) {
    if (feederId >= TOTAL_FEEDERS || !connectedHands[feederId].isOnline) {
        return false;
    }

    // 创建UDP命令包
    UDPCommandPacket udpCommand;
    udpCommand.packetType = UDP_PKT_COMMAND;
    udpCommand.sequence = nextSequence++;
    udpCommand.timestamp = getCurrentTimestamp();
    udpCommand.command = command;

    // 记录待命令
    if (timeoutMs > 0) {
        for (int i = 0; i < 10; i++) {
            if (!pendingCommands[i].waiting) {
                pendingCommands[i].sequence = udpCommand.sequence;
                pendingCommands[i].feederId = feederId;
                pendingCommands[i].sentTime = millis();
                pendingCommands[i].timeoutMs = timeoutMs;
                pendingCommands[i].waiting = true;
                pendingCommands[i].needTcpReply = needTcpReply;
                pendingCommands[i].commandType = command.commandType;
                break;
            }
        }
    }

    // 发送命令
    udp.beginPacket(connectedHands[feederId].ip, connectedHands[feederId].port);
    udp.write((uint8_t*)&udpCommand, sizeof(udpCommand));
    bool sent = udp.endPacket();

    if (sent) {
        brainUdpStats.commandsSent++;
        // 更新最后通信时间
        connectedHands[feederId].lastSeen = millis();
        
        // 通知Web界面命令已发送
        if (command.commandType == CMD_FEEDER_ADVANCE) {
            notifyCommandReceived(feederId, command.feedLength);
        }
    } else {
        brainUdpStats.errors++;
    }

    return sent;
}

void sendHeartbeatToAllHands() {
    UDPHeartbeatPacket heartbeat;
    heartbeat.packetType = UDP_PKT_HEARTBEAT;
    heartbeat.deviceId = 0; // Brain ID
    heartbeat.timestamp_low = getTimestampLow();  // 使用优化后的低16位时间戳
    heartbeat.status = 0; // 正常状态

    int sentCount = 0;
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        if (connectedHands[i].isOnline) {
            udp.beginPacket(connectedHands[i].ip, connectedHands[i].port);
            udp.write((uint8_t*)&heartbeat, sizeof(heartbeat));
            if (udp.endPacket()) {
                sentCount++;
                connectedHands[i].lastSeen = millis();
                DEBUG_PRINTF("UDP: 心跳已发送到Hand %d (%s:%d)\n", 
                           i, connectedHands[i].ip.toString().c_str(), connectedHands[i].port);
            }
        }
    }

    if (sentCount > 0) {
        brainUdpStats.heartbeatsSent += sentCount;
        DEBUG_PRINTF("UDP: 心跳批量发送完成，共发送给 %d 个Hand\n", sentCount);
    }
}

int getOnlineHandCount() {
    int count = 0;
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        if (connectedHands[i].isOnline) {
            count++;
        }
    }
    return count;
}

// =============================================================================
// 内部UDP处理函数实现
// =============================================================================

void processBrainUDPData() {
    // 处理主UDP端口的数据
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        IPAddress remoteIP = udp.remoteIP();
        uint16_t remotePort = udp.remotePort();
        
        size_t len = udp.read(brainUdpBuffer, sizeof(brainUdpBuffer));
        if (len > 0) {
            // printUDPPacket(brainUdpBuffer, len, true); // 打印接收的UDP包信息
            
            UDPPacketType packetType = (UDPPacketType)brainUdpBuffer[0];
            
            switch (packetType) {
                case UDP_PKT_RESPONSE:
                    if (len >= sizeof(UDPResponsePacket)) {
                        handleHandResponse(*(UDPResponsePacket*)brainUdpBuffer, remoteIP);
                    }
                    break;
                    
                case UDP_PKT_HEARTBEAT:
                    if (len >= sizeof(UDPHeartbeatPacket)) {
                        handleHandHeartbeat(*(UDPHeartbeatPacket*)brainUdpBuffer, remoteIP);
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }

    // 处理发现端口的数据
    packetSize = discoveryUdp.parsePacket();
    if (packetSize > 0) {
        IPAddress remoteIP = discoveryUdp.remoteIP();
        uint16_t remotePort = discoveryUdp.remotePort();
        
        size_t len = discoveryUdp.read(brainUdpBuffer, sizeof(brainUdpBuffer));
        if (len > 0 && brainUdpBuffer[0] == UDP_PKT_DISCOVERY_REQUEST) {
            if (len >= sizeof(UDPDiscoveryRequest)) {
                handleDiscoveryRequest(*(UDPDiscoveryRequest*)brainUdpBuffer, remoteIP, remotePort);
            }
        }
    }
}

void handleDiscoveryRequest(const UDPDiscoveryRequest& request, IPAddress fromIP, uint16_t fromPort) {
    handDiscoveryCount++;
    brainUdpStats.discoveryRequests++;
    
    // 发送发现响应
    sendDiscoveryResponse(fromIP, UDP_HAND_PORT, request.handId);
    
    // 更新Hand信息
    updateHandInfo(request.handId, fromIP, UDP_HAND_PORT, request.handInfo);
}

void handleHandResponse(const UDPResponsePacket& response, IPAddress fromIP) {
    uint8_t feederId = response.response.handId;
    
    brainUdpStats.responsesReceived++;
    
    // 更新Hand最后通信时间
    if (feederId < TOTAL_FEEDERS) {
        connectedHands[feederId].lastSeen = millis();
        
        // 更新IP地址（可能发生变化）
        if (connectedHands[feederId].ip != fromIP) {
            connectedHands[feederId].ip = fromIP;
        }
    }
    
    // 清除对应的待命令并处理TCP回复
    for (int i = 0; i < 10; i++) {
        if (pendingCommands[i].waiting && 
            pendingCommands[i].sequence == response.sequence &&
            pendingCommands[i].feederId == feederId) {
            
            // 如果需要TCP回复，发送给TCP客户端
            if (pendingCommands[i].needTcpReply) {
                WiFiClient* tcpClient = getCurrentTcpClient();
                if (tcpClient && tcpClient->connected()) {
                    String tcpResponse;
                    if (response.response.status == STATUS_OK) {
                        tcpResponse = "ok ";
                        switch (pendingCommands[i].commandType) {
                            case CMD_FEEDER_ADVANCE:
                                tcpResponse += "Feed completed - " + String(response.response.message);
                                break;
                            default:
                                tcpResponse += String(response.response.message);
                                break;
                        }
                    } else {
                        tcpResponse = "error " + String(response.response.message);
                    }
                    
                    tcpClient->println(tcpResponse);
                    tcpClient->flush();
                }
            }
            
            // 通知Web界面命令已完成
            if (pendingCommands[i].commandType == CMD_FEEDER_ADVANCE) {
                bool success = (response.response.status == STATUS_OK);
                notifyCommandCompleted(feederId, success, response.response.message);
            }
            
            pendingCommands[i].waiting = false;
            break;
        }
    }
}

void handleHandHeartbeat(const UDPHeartbeatPacket& heartbeat, IPAddress fromIP) {
    uint8_t feederId = heartbeat.deviceId;
    
    DEBUG_PRINTF("UDP: 收到Hand %d心跳 from %s\n", feederId, fromIP.toString().c_str());
    
    // 更新Hand信息
    if (feederId < TOTAL_FEEDERS) {
        bool wasOnline = connectedHands[feederId].isOnline;
        connectedHands[feederId].ip = fromIP;
        connectedHands[feederId].port = UDP_HAND_PORT;
        connectedHands[feederId].lastSeen = millis();
        connectedHands[feederId].isOnline = true;
        connectedHands[feederId].feederId = feederId;
        snprintf(connectedHands[feederId].handInfo, sizeof(connectedHands[feederId].handInfo), "Hand-%d", feederId);
        
        // 如果之前离线，现在上线了，通知Web界面
        if (!wasOnline) {
            notifyHandOnline(feederId);
        }
    } else if (feederId == 255) {
        // 处理未分配的Hand设备（ID=255）
        DEBUG_PRINTF("UDP: 处理未分配Hand设备心跳 from %s\n", fromIP.toString().c_str());
        
        // 查找或创建未分配设备条目
        int unassignedIndex = -1;
        uint32_t currentTime = millis();
        
        // 首先查找是否已经存在这个IP的设备
        for (int i = 0; i < 10; i++) {
            if (unassignedHands[i].isValid && 
                IPAddress(unassignedHands[i].mac[0], unassignedHands[i].mac[1], 
                         unassignedHands[i].mac[2], unassignedHands[i].mac[3]) == fromIP) {
                unassignedIndex = i;
                break;
            }
        }
        
        // 如果没找到，寻找空闲槽位
        if (unassignedIndex == -1) {
            for (int i = 0; i < 10; i++) {
                if (!unassignedHands[i].isValid) {
                    unassignedIndex = i;
                    break;
                }
            }
        }
        
        // 如果还没找到，使用最旧的槽位
        if (unassignedIndex == -1) {
            uint32_t oldestTime = currentTime;
            for (int i = 0; i < 10; i++) {
                if (unassignedHands[i].lastSeen < oldestTime) {
                    oldestTime = unassignedHands[i].lastSeen;
                    unassignedIndex = i;
                }
            }
        }
        
        // 更新未分配设备信息
        if (unassignedIndex != -1) {
            unassignedHands[unassignedIndex].isValid = true;
            unassignedHands[unassignedIndex].lastSeen = currentTime;
            // 使用IP地址的字节作为临时MAC（用于兼容原有接口）
            unassignedHands[unassignedIndex].mac[0] = fromIP[0];
            unassignedHands[unassignedIndex].mac[1] = fromIP[1];
            unassignedHands[unassignedIndex].mac[2] = fromIP[2];
            unassignedHands[unassignedIndex].mac[3] = fromIP[3];
            unassignedHands[unassignedIndex].mac[4] = 0;
            unassignedHands[unassignedIndex].mac[5] = 0;
            snprintf(unassignedHands[unassignedIndex].info, sizeof(unassignedHands[unassignedIndex].info), 
                     "Hand-255@%s", fromIP.toString().c_str());
        }
    } else {
        DEBUG_PRINTF("UDP: 收到无效的Hand ID %d心跳\n", feederId);
    }
}

bool sendDiscoveryResponse(IPAddress handIP, uint16_t handPort, uint8_t handId) {
    UDPDiscoveryResponse response;
    response.packetType = UDP_PKT_DISCOVERY_RESPONSE;
    response.brainId = 0; // Brain ID
    response.timestamp_low = getTimestampLow();  // 使用优化后的低16位时间戳
    
    // 将IP地址转换为字节数组
    response.brainIP[0] = WiFi.localIP()[0];
    response.brainIP[1] = WiFi.localIP()[1];
    response.brainIP[2] = WiFi.localIP()[2];
    response.brainIP[3] = WiFi.localIP()[3];
    
    response.brainPort = UDP_BRAIN_PORT;
    snprintf(response.brainInfo, sizeof(response.brainInfo), "Brain-ESP32");

    discoveryUdp.beginPacket(handIP, UDP_DISCOVERY_PORT);
    discoveryUdp.write((uint8_t*)&response, sizeof(response));
    bool sent = discoveryUdp.endPacket();

    if (sent) {
        brainUdpStats.discoveryResponses++;
    } else {
        brainUdpStats.errors++;
    }

    return sent;
}

void checkHandConnections() {
    uint32_t now = millis();
    int disconnectedCount = 0;
    
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        if (connectedHands[i].isOnline) {
            if (now - connectedHands[i].lastSeen > 60000) { // 60秒无通信
                connectedHands[i].isOnline = false;
                disconnectedCount++;
                
                // 通知Web界面Hand离线
                notifyHandOffline(i);
            }
        }
    }
}

void updateHandInfo(uint8_t feederId, IPAddress ip, uint16_t port, const char* info) {
    if (feederId >= TOTAL_FEEDERS) {
        return;
    }
    
    bool wasOnline = connectedHands[feederId].isOnline;
    connectedHands[feederId].ip = ip;
    connectedHands[feederId].port = port;
    connectedHands[feederId].lastSeen = millis();
    connectedHands[feederId].isOnline = true;
    connectedHands[feederId].feederId = feederId;
    
    strncpy(connectedHands[feederId].handInfo, info, sizeof(connectedHands[feederId].handInfo) - 1);
    connectedHands[feederId].handInfo[sizeof(connectedHands[feederId].handInfo) - 1] = '\0';
    
    // 如果之前离线，现在上线了，通知Web界面
    if (!wasOnline) {
        notifyHandOnline(feederId);
    }
}

// =============================================================================
// 兼容函数实现（保持与原ESP-NOW接口兼容）
// =============================================================================

bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength, uint32_t timeoutMs) {
    ESPNowPacket command;
    command.commandType = CMD_FEEDER_ADVANCE;
    command.feederId = feederId;
    command.feedLength = feedLength;
    memset(command.reserved, 0, sizeof(command.reserved));
    
    return sendCommandToHand(feederId, command, timeoutMs);
}

bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength, uint32_t timeoutMs, bool needTcpReply) {
    ESPNowPacket command;
    command.commandType = CMD_FEEDER_ADVANCE;
    command.feederId = feederId;
    command.feedLength = feedLength;
    memset(command.reserved, 0, sizeof(command.reserved));
    
    return sendCommandToHand(feederId, command, timeoutMs, needTcpReply);
}

bool sendSetFeederIDCommand(uint8_t feederId, uint8_t newFeederID) {
    ESPNowPacket command;
    command.commandType = CMD_SET_FEEDER_ID;
    command.feederId = 255; // 广播
    command.feedLength = newFeederID; // 新ID放在feedLength字段
    memset(command.reserved, 0, sizeof(command.reserved));
    
    // 发送到所有在线Hand（广播模式）
    bool anySent = false;
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        if (connectedHands[i].isOnline) {
            if (sendCommandToHand(i, command, UDP_COMMAND_TIMEOUT_MS)) {
                anySent = true;
            }
        }
    }
    
    return anySent;
}

bool sendSetFeederIDCommandToDevice(IPAddress targetIP, uint16_t targetPort, uint8_t newFeederID) {
    DEBUG_PRINTF("Brain UDP: 发送设置ID命令到设备 %s:%d，新ID: %d\n", 
                 targetIP.toString().c_str(), targetPort, newFeederID);
    
    // 创建UDP命令包
    UDPCommandPacket udpCommand;
    udpCommand.packetType = UDP_PKT_COMMAND;
    udpCommand.sequence = nextSequence++;
    udpCommand.timestamp = millis();
    
    // 设置命令内容
    udpCommand.command.commandType = CMD_SET_FEEDER_ID;
    udpCommand.command.feederId = 255; // 广播模式
    udpCommand.command.feedLength = newFeederID; // 新ID放在feedLength字段
    memset(udpCommand.command.reserved, 0, sizeof(udpCommand.command.reserved));
    
    // 发送UDP包到指定设备
    bool sent = false;
    udp.beginPacket(targetIP, targetPort);
    udp.write((uint8_t*)&udpCommand, sizeof(udpCommand));
    sent = udp.endPacket();
    
    if (sent) {
        brainUdpStats.commandsSent++;
        DEBUG_PRINTF("Brain UDP: 成功发送设置ID命令到 %s:%d\n", 
                     targetIP.toString().c_str(), targetPort);
    } else {
        brainUdpStats.errors++;
        DEBUG_PRINTF("Brain UDP: 发送设置ID命令失败到 %s:%d\n", 
                     targetIP.toString().c_str(), targetPort);
    }
    
    return sent;
}

bool sendFindMeCommand(uint8_t feederId) {
    DEBUG_PRINTF("Brain UDP: 发送Find Me命令到Feeder %d\n", feederId);
    
    // 检查Feeder ID有效性
    if (feederId >= TOTAL_FEEDERS) {
        DEBUG_PRINTF("Brain UDP: 无效的Feeder ID: %d\n", feederId);
        return false;
    }
    
    // 检查Hand是否在线
    if (!connectedHands[feederId].isOnline) {
        DEBUG_PRINTF("Brain UDP: Feeder %d 不在线\n", feederId);
        return false;
    }
    
    // 创建Find Me命令
    ESPNowPacket command;
    command.commandType = CMD_FIND_ME;
    command.feederId = feederId;
    command.feedLength = 0; // Find Me命令不需要feedLength
    memset(command.reserved, 0, sizeof(command.reserved));
    
    return sendCommandToHand(feederId, command, UDP_COMMAND_TIMEOUT_MS);
}

bool sendFindMeCommandToDevice(IPAddress targetIP, uint16_t targetPort) {
    DEBUG_PRINTF("Brain UDP: 发送Find Me命令到设备 %s:%d\n", 
                 targetIP.toString().c_str(), targetPort);
    
    // 创建UDP命令包
    UDPCommandPacket udpCommand;
    udpCommand.packetType = UDP_PKT_COMMAND;
    udpCommand.sequence = nextSequence++;
    udpCommand.timestamp = millis();
    
    // 设置命令内容
    udpCommand.command.commandType = CMD_FIND_ME;
    udpCommand.command.feederId = 255; // 未分配设备使用255
    udpCommand.command.feedLength = 0; // Find Me命令不需要feedLength
    memset(udpCommand.command.reserved, 0, sizeof(udpCommand.command.reserved));
    
    // 发送UDP包到指定设备
    bool sent = false;
    udp.beginPacket(targetIP, targetPort);
    udp.write((uint8_t*)&udpCommand, sizeof(udpCommand));
    sent = udp.endPacket();
    
    if (sent) {
        brainUdpStats.commandsSent++;
        DEBUG_PRINTF("Brain UDP: 成功发送Find Me命令到 %s:%d\n", 
                     targetIP.toString().c_str(), targetPort);
    } else {
        brainUdpStats.errors++;
        DEBUG_PRINTF("Brain UDP: 发送Find Me命令失败到 %s:%d\n", 
                     targetIP.toString().c_str(), targetPort);
    }
    
    return sent;
}

// Web通知函数实现（在brain_web.cpp中实现，这里只保留空声明保持兼容性）
// void notifyCommandReceived(uint8_t feederId, uint8_t feedLength) - 在brain_web.cpp中实现
// void notifyCommandCompleted(uint8_t feederId, bool success, const char* message) - 在brain_web.cpp中实现  
// void notifyHandOnline(uint8_t feederId) - 在brain_web.cpp中实现
// void notifyHandOffline(uint8_t feederId) - 在brain_web.cpp中实现

// 兼容ESP-NOW的旧接口函数
void processReceivedResponse() {
    // 这个函数的功能已经在brain_udp_update()中实现
    // 保留空实现以保持兼容性
}

void checkCommandTimeout() {
    // 这个函数的功能已经在brain_udp_update()中实现
    // 保留空实现以保持兼容性
}

void sendHeartbeat() {
    // 这个函数的功能已经在brain_udp_update()中实现
    // 手动调用心跳发送
    sendHeartbeatToAllHands();
}

// gcode.cpp需要的函数实现
bool sendSetFeederIDCommand(uint8_t* targetMAC, uint8_t newFeederID) {
    // UDP版本：广播设置ID命令到所有Hand
    DEBUG_PRINTF("Brain UDP: 设置Feeder ID命令 targetMAC=忽略 newID=%d\n", newFeederID);
    return sendSetFeederIDCommand(255, newFeederID); // 使用UDP版本的实现
}

// =============================================================================
// 缺失函数的实现
// =============================================================================

void initFeederStatus() {
    DEBUG_PRINTF("Brain UDP: 初始化Feeder状态数组\n");
    
    // 初始化所有Feeder状态
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        feederStatusArray[i].waitingForResponse = false;
        feederStatusArray[i].commandSentTime = 0;
        feederStatusArray[i].timeoutMs = 0;
        feederStatusArray[i].totalFeedCount = 0;
        feederStatusArray[i].sessionFeedCount = 0;
        feederStatusArray[i].totalPartCount = 0;
        feederStatusArray[i].remainingPartCount = 0;
        strcpy(feederStatusArray[i].componentName, "未设置");
        strcpy(feederStatusArray[i].packageType, "N/A");
        
        lastHandResponse[i] = 0;
    }
    
    // 初始化未分配设备数组
    for (int i = 0; i < 10; i++) {
        unassignedHands[i].isValid = false;
        memset(unassignedHands[i].mac, 0, 6);
        memset(unassignedHands[i].info, 0, 20);
        unassignedHands[i].lastSeen = 0;
    }
    
    // 加载配置
    loadFeederConfig();
}

void resetBrainUDPStats() {
    DEBUG_PRINTF("Brain UDP: 重置UDP统计信息\n");
    
    memset(&brainUdpStats, 0, sizeof(brainUdpStats));
    handDiscoveryCount = 0;
    totalSessionFeeds = 0;
    totalWorkCount = 0;
}

const char* getHandStatusString(uint8_t feederId) {
    if (feederId >= TOTAL_FEEDERS) {
        return "无效ID";
    }
    
    if (!connectedHands[feederId].isOnline) {
        return "离线";
    }
    
    uint32_t now = millis();
    uint32_t timeSinceLastSeen = now - connectedHands[feederId].lastSeen;
    
    if (timeSinceLastSeen > 30000) { // 30秒无响应
        return "离线";
    } else if (timeSinceLastSeen > 15000) { // 15秒响应慢
        return "不稳定";
    } else {
        return "在线";
    }
}

void getUnassignedHandsList(String &response) {
    DEBUG_PRINTF("Brain UDP: 获取未分配Hand列表\n");
    
    response = "未分配Hand设备:\n";
    uint32_t currentTime = millis();
    int count = 0;
    
    // 检查unassignedHands数组
    for (int i = 0; i < 10; i++) {
        if (unassignedHands[i].isValid) {
            // 检查设备是否还在线（30秒内有心跳）
            if (currentTime - unassignedHands[i].lastSeen < 30000) {
                IPAddress deviceIP(unassignedHands[i].mac[0], unassignedHands[i].mac[1], 
                                 unassignedHands[i].mac[2], unassignedHands[i].mac[3]);
                response += String(count + 1) + ". IP: " + deviceIP.toString() + 
                           " 端口: " + String(UDP_HAND_PORT) + 
                           " 信息: " + String(unassignedHands[i].info) + 
                           " 最后看到: " + String((currentTime - unassignedHands[i].lastSeen) / 1000) + "秒前\n";
                count++;
            }
        }
    }
    
    // 检查connectedHands数组中feederId为255的设备
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        if (connectedHands[i].isOnline && connectedHands[i].feederId == 255) {
            if (currentTime - connectedHands[i].lastSeen < 30000) {
                response += String(count + 1) + ". IP: " + connectedHands[i].ip.toString() + 
                           " 端口: " + String(connectedHands[i].port) + 
                           " 信息: " + String(connectedHands[i].handInfo) + 
                           " 最后看到: " + String((currentTime - connectedHands[i].lastSeen) / 1000) + "秒前\n";
                count++;
            }
        }
    }
    
    if (count == 0) {
        response += "没有发现未分配的Hand设备\n";
    } else {
        response += "总计: " + String(count) + " 个未分配设备\n";
    }
}

void getOnlineHandDetails(String &response) {
    DEBUG_PRINTF("Brain UDP: 获取在线Hand详细信息\n");
    
    response = "在线Hand设备详情:\n";
    uint32_t currentTime = millis();
    int onlineCount = 0;
    
    for (int i = 0; i < TOTAL_FEEDERS; i++) {
        if (connectedHands[i].isOnline) {
            uint32_t timeSinceLastSeen = currentTime - connectedHands[i].lastSeen;
            if (timeSinceLastSeen < 30000) { // 30秒内有通信认为在线
                response += "Feeder " + String(i) + ": ";
                response += "IP=" + connectedHands[i].ip.toString();
                response += " 端口=" + String(connectedHands[i].port);
                response += " 状态=" + String(getHandStatusString(i));
                response += " 信息=" + String(connectedHands[i].handInfo);
                response += " 最后通信=" + String(timeSinceLastSeen / 1000) + "秒前";
                response += " 总送料=" + String(feederStatusArray[i].totalFeedCount);
                response += " 会话送料=" + String(feederStatusArray[i].sessionFeedCount);
                response += "\n";
                onlineCount++;
            }
        }
    }
    
    if (onlineCount == 0) {
        response += "没有在线的Hand设备\n";
    } else {
        response += "总计: " + String(onlineCount) + " 个在线设备\n";
    }
}

void loadFeederConfig() {
    DEBUG_PRINTF("Brain UDP: 加载Feeder配置（暂未实现持久化存储）\n");
    // TODO: 实现从存储加载配置
    // 目前使用默认配置
}

void saveFeederConfig() {
    DEBUG_PRINTF("Brain UDP: 保存Feeder配置（暂未实现持久化存储）\n");
    // TODO: 实现配置持久化存储
    // 目前只在内存中保存
}

void updateFeederStats(uint8_t feederId, bool success) {
    if (feederId >= TOTAL_FEEDERS) {
        return;
    }
    
    if (success) {
        feederStatusArray[feederId].totalFeedCount++;
        feederStatusArray[feederId].sessionFeedCount++;
        totalSessionFeeds++;
        totalWorkCount++;
        
        // 减少剩余零件数量
        if (feederStatusArray[feederId].remainingPartCount > 0) {
            feederStatusArray[feederId].remainingPartCount--;
        }
        
        DEBUG_PRINTF("Brain UDP: Feeder %d 送料成功，总计=%lu，会话=%d\n", 
                     feederId, feederStatusArray[feederId].totalFeedCount, 
                     feederStatusArray[feederId].sessionFeedCount);
    } else {
        DEBUG_PRINTF("Brain UDP: Feeder %d 送料失败\n", feederId);
    }
}
