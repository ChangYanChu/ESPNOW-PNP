#ifndef HAND_UDP_H
#define HAND_UDP_H

#include <Arduino.h>
#if defined ESP32
#include <WiFi.h>
#include <WiFiUdp.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#endif
#include "common/udp_protocol.h"

// =============================================================================
// UDP通信状态和配置
// =============================================================================

// UDP连接状态
extern UDPConnectionState udpState;
extern BrainInfo connectedBrain;
extern UDPStats udpStats;

// =============================================================================
// 核心UDP函数
// =============================================================================

// 初始化UDP通信
void udp_setup();

// UDP主循环处理
void udp_update();

// 发现Brain设备
bool discoverBrain();

// 发送命令并等待响应
bool sendCommandAndWaitResponse(const ESPNowPacket& command, ESPNowResponse& response, uint32_t timeoutMs = UDP_COMMAND_TIMEOUT_MS);

// 发送心跳包
void sendHeartbeat();

// =============================================================================
// 内部UDP处理函数
// =============================================================================

// 处理接收到的UDP数据
void processUDPData();

// 处理发现响应
void handleDiscoveryResponse(const UDPDiscoveryResponse& response, IPAddress fromIP);

// 处理业务响应
void handleBusinessResponse(const UDPResponsePacket& response);

// 处理Brain心跳包
void handleBrainHeartbeat(const UDPHeartbeatPacket& heartbeat, IPAddress fromIP);

// 发送发现请求
bool sendDiscoveryRequest();

// 检查Brain连接状态
void checkBrainConnection();

// =============================================================================
// 兼容函数（保持与原ESP-NOW接口兼容）
// =============================================================================

// 处理接收到的命令
void processReceivedCommand();

// 调度响应
void schedulePendingResponse(uint8_t feederID, uint8_t status, const char *message);

// 处理待发送的响应
void processPendingResponse();

// =============================================================================
// 调试和统计函数
// =============================================================================

// 打印UDP状态信息
void printUDPStatus();

// 重置UDP统计
void resetUDPStats();

// 获取连接状态字符串
const char* getUDPStateString();

#endif // HAND_UDP_H
