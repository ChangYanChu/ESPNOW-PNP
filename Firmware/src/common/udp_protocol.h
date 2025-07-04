#ifndef UDP_PROTOCOL_H
#define UDP_PROTOCOL_H

#include <Arduino.h>
#if defined ESP32
#include <WiFi.h>             // 提供IPAddress类型
#elif defined ESP8266
#include <ESP8266WiFi.h>      // ESP8266的WiFi库
#endif
#include "espnow_protocol.h"  // 继承原有协议结构

// =============================================================================
// UDP通信协议定义
// =============================================================================

// UDP通信端口定义
#define UDP_BRAIN_PORT      8266        // Brain端监听端口
#define UDP_HAND_PORT       8267        // Hand端监听端口
#define UDP_DISCOVERY_PORT  8268        // 发现服务端口
#define UDP_BROADCAST_PORT  8269        // 广播端口

// UDP通信超时设置 - 优化后的性能参数
#define UDP_DISCOVERY_TIMEOUT_MS    3000    // 发现超时3秒(减少等待时间)
#define UDP_COMMAND_TIMEOUT_MS      1500    // 命令超时1.5秒(减少阻塞)
#define UDP_DISCOVERY_INTERVAL_MS   15000   // 发现间隔15秒(减少网络负载)
#define UDP_HEARTBEAT_INTERVAL_MS   8000    // 心跳间隔8秒(平衡连接检测和网络负载)

// 性能优化参数
#define UDP_MAX_RETRY_COUNT         3       // 最大重试次数
#define UDP_BUFFER_SIZE             256     // UDP缓冲区大小(减少内存使用)
#define UDP_BATCH_SIZE              5       // 批量处理包数量

// UDP包类型定义
typedef enum {
    UDP_PKT_DISCOVERY_REQUEST = 0x10,   // 发现请求
    UDP_PKT_DISCOVERY_RESPONSE = 0x11,  // 发现响应
    UDP_PKT_COMMAND = 0x12,             // 业务命令
    UDP_PKT_RESPONSE = 0x13,            // 业务响应
    UDP_PKT_HEARTBEAT = 0x14,           // 心跳包
    UDP_PKT_PING = 0x15,                // Ping包
} UDPPacketType;

// UDP发现请求包 - 优化后更紧凑
struct UDPDiscoveryRequest {
    uint8_t packetType;                 // 包类型: UDP_PKT_DISCOVERY_REQUEST
    uint8_t handId;                     // Hand设备ID
    uint16_t timestamp_low;             // 时间戳低16位(减少包大小)
    char handInfo[12];                  // Hand设备信息(缩短以减少网络负载)
} __attribute__((packed));

// UDP发现响应包 - 优化后更紧凑
struct UDPDiscoveryResponse {
    uint8_t packetType;                 // 包类型: UDP_PKT_DISCOVERY_RESPONSE
    uint8_t brainId;                    // Brain设备ID
    uint16_t timestamp_low;             // 时间戳低16位
    uint8_t brainIP[4];                 // Brain IP地址(字节数组)
    uint16_t brainPort;                 // Brain监听端口
    char brainInfo[8];                  // Brain设备信息(缩短)
} __attribute__((packed));

// UDP命令包 (复用ESP-NOW的命令结构)
struct UDPCommandPacket {
    uint8_t packetType;                 // 包类型: UDP_PKT_COMMAND
    uint32_t sequence;                  // 序列号
    uint32_t timestamp;                 // 时间戳
    ESPNowPacket command;               // 业务命令(复用原有结构)
} __attribute__((packed));

// UDP响应包 (复用ESP-NOW的响应结构)
struct UDPResponsePacket {
    uint8_t packetType;                 // 包类型: UDP_PKT_RESPONSE
    uint32_t sequence;                  // 对应命令的序列号
    uint32_t timestamp;                 // 响应时间戳
    ESPNowResponse response;            // 业务响应(复用原有结构)
} __attribute__((packed));

// UDP心跳包 - 最小化设计
struct UDPHeartbeatPacket {
    uint8_t packetType;                 // 包类型: UDP_PKT_HEARTBEAT
    uint8_t deviceId;                   // 设备ID
    uint16_t timestamp_low;             // 心跳时间戳低16位
    uint8_t status;                     // 设备状态
} __attribute__((packed));

// UDP连接状态
typedef enum {
    UDP_STATE_DISCONNECTED = 0,         // 未连接
    UDP_STATE_DISCOVERING = 1,          // 发现中
    UDP_STATE_CONNECTED = 2,            // 已连接
    UDP_STATE_ERROR = 3                 // 错误状态
} UDPConnectionState;

// Brain设备信息
struct BrainInfo {
    IPAddress ip;                       // Brain IP地址
    uint16_t port;                      // Brain端口
    uint32_t lastSeen;                  // 最后发现时间
    bool isActive;                      // 是否活跃
    char info[16];                      // 设备信息
};

// UDP通信统计
struct UDPStats {
    uint32_t discoveryRequests;         // 发现请求次数
    uint32_t discoveryResponses;        // 发现响应次数
    uint32_t commandsSent;              // 发送命令次数
    uint32_t responsesReceived;         // 接收响应次数
    uint32_t heartbeatsSent;            // 发送心跳次数
    uint32_t timeouts;                  // 超时次数
    uint32_t errors;                    // 错误次数
};

// =============================================================================
// UDP工具函数声明
// =============================================================================

// 生成序列号
uint32_t generateSequence();

// 验证UDP包合法性
bool isValidUDPPacket(const uint8_t* data, size_t len, UDPPacketType expectedType);

// 打印UDP包信息 (调试用)
void printUDPPacket(const uint8_t* data, size_t len, bool isIncoming = true);

// 获取当前时间戳
uint32_t getCurrentTimestamp();

// 性能优化工具函数
uint16_t getTimestampLow();
bool isPacketFresh(uint16_t timestampLow, uint32_t maxAge = 30000);
void optimizeWiFiSettings();
void setUDPBufferSizes();

#endif // UDP_PROTOCOL_H
