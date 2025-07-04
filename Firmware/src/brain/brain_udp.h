#ifndef BRAIN_UDP_H
#define BRAIN_UDP_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "common/udp_protocol.h"
#include "brain_config.h"

// =============================================================================
// Brain端UDP通信状态和配置
// =============================================================================

// Hand设备信息结构
struct HandInfo {
    IPAddress ip;                       // Hand IP地址
    uint16_t port;                      // Hand端口
    uint32_t lastSeen;                  // 最后通信时间
    bool isOnline;                      // 是否在线
    uint8_t feederId;                   // 喂料器ID
    char handInfo[20];                  // Hand设备信息
};

// Brain端UDP状态
extern HandInfo connectedHands[TOTAL_FEEDERS];
extern uint32_t handDiscoveryCount;
extern UDPStats brainUdpStats;

// =============================================================================
// 核心UDP函数
// =============================================================================

// 初始化Brain端UDP通信
void brain_udp_setup();

// Brain端UDP主循环处理
void brain_udp_update();

// 发送命令到指定Hand
bool sendCommandToHand(uint8_t feederId, const ESPNowPacket& command, uint32_t timeoutMs = UDP_COMMAND_TIMEOUT_MS);

// 发送命令到指定Hand（支持TCP回复）
bool sendCommandToHand(uint8_t feederId, const ESPNowPacket& command, uint32_t timeoutMs, bool needTcpReply);

// 发送心跳到所有在线Hand
void sendHeartbeatToAllHands();

// 获取在线Hand数量
int getOnlineHandCount();

// =============================================================================
// Web通知函数（在brain_web.cpp中实现）
// =============================================================================

// Web通知函数声明
void notifyCommandReceived(uint8_t feederId, uint8_t feedLength);
void notifyCommandCompleted(uint8_t feederId, bool success, const char* message);
void notifyHandOnline(uint8_t feederId);
void notifyHandOffline(uint8_t feederId);

// =============================================================================
// 内部UDP处理函数
// =============================================================================

// 处理接收到的UDP数据
void processBrainUDPData();

// 处理发现请求
void handleDiscoveryRequest(const UDPDiscoveryRequest& request, IPAddress fromIP, uint16_t fromPort);

// 处理Hand响应
void handleHandResponse(const UDPResponsePacket& response, IPAddress fromIP);

// 处理Hand心跳
void handleHandHeartbeat(const UDPHeartbeatPacket& heartbeat, IPAddress fromIP);

// 发送发现响应
bool sendDiscoveryResponse(IPAddress handIP, uint16_t handPort, uint8_t handId);

// 检查Hand连接状态
void checkHandConnections();

// 更新Hand信息
void updateHandInfo(uint8_t feederId, IPAddress ip, uint16_t port, const char* info);

// =============================================================================
// 兼容函数（保持与原ESP-NOW接口兼容）
// =============================================================================

// 发送喂料命令（兼容原接口）
bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength, uint32_t timeoutMs = 0);

// 发送喂料命令（支持TCP回复）
bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength, uint32_t timeoutMs, bool needTcpReply);

// 发送设置ID命令（兼容原接口）
bool sendSetFeederIDCommand(uint8_t feederId, uint8_t newFeederID);

// 发送设置ID命令（原MAC地址版本，兼容gcode.cpp）
bool sendSetFeederIDCommand(uint8_t* targetMAC, uint8_t newFeederID);

// 发送设置ID命令到指定IP地址和端口
bool sendSetFeederIDCommandToDevice(IPAddress targetIP, uint16_t targetPort, uint8_t newFeederID);

// 发送Find Me命令
bool sendFindMeCommand(uint8_t feederId);

// 发送Find Me命令到指定IP地址和端口
bool sendFindMeCommandToDevice(IPAddress targetIP, uint16_t targetPort);

// 获取未分配ID的Hand列表
void getUnassignedHandsList(String &response);

// 获取在线Hand详细信息（兼容原接口）
void getOnlineHandDetails(String &response);

// =============================================================================
// 调试和统计函数
// =============================================================================

// 打印Brain UDP状态
void printBrainUDPStatus();

// 重置Brain UDP统计
void resetBrainUDPStats();

// 获取Hand状态字符串
const char* getHandStatusString(uint8_t feederId);

// =============================================================================
// 兼容ESP-NOW的数据结构和变量（保持原有接口）
// =============================================================================

// 喂料器状态结构（保持与原ESP-NOW兼容）
struct FeederStatus
{
    bool waitingForResponse;
    uint32_t commandSentTime;
    uint32_t timeoutMs;
    // 新增统计字段
    uint32_t totalFeedCount;        // 总送料次数
    uint16_t sessionFeedCount;      // 本次开机送料次数
    uint16_t totalPartCount;        // 总零件数量
    uint16_t remainingPartCount;    // 剩余零件数量
    char componentName[16];         // 元件名称（压缩长度）
    char packageType[8];            // 封装类型（压缩长度）
};

// 兼容变量声明
extern FeederStatus feederStatusArray[TOTAL_FEEDERS];
extern uint32_t totalSessionFeeds;
extern uint32_t totalWorkCount;
extern uint32_t lastHandResponse[TOTAL_FEEDERS];

// 未分配Hand结构（用于gcode.cpp兼容）
struct UnassignedHand {
    uint8_t mac[6];
    char info[20];
    uint32_t lastSeen;
    bool isValid;
};
extern UnassignedHand unassignedHands[10];

// 兼容函数声明
void loadFeederConfig();
void saveFeederConfig();
void updateFeederStats(uint8_t feederId, bool success);
void initFeederStatus();

// 其他兼容函数
void processReceivedResponse();
void checkCommandTimeout();
void sendHeartbeat();

#endif // BRAIN_UDP_H
