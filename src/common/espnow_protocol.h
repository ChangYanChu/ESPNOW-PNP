#ifndef ESPNOW_PROTOCOL_H
#define ESPNOW_PROTOCOL_H

#include <Arduino.h>

// =============================================================================
// ESP-NOW通信协议定义
// =============================================================================

// 魔数用于验证数据包
#define ESPNOW_MAGIC 0xFEED

// 命令类型枚举
typedef enum {
    CMD_SERVO_SET_ANGLE = 0x01,      // 设置舵机角度
    CMD_SERVO_ATTACH = 0x02,         // 连接舵机
    CMD_SERVO_DETACH = 0x03,         // 断开舵机
    CMD_FEEDER_ADVANCE = 0x04,       // 喂料推进
    CMD_FEEDER_RETRACT = 0x05,       // 喂料回缩
    CMD_STATUS_REQUEST = 0x06,       // 状态查询
    CMD_RESPONSE = 0x07,             // 响应命令
    CMD_HEARTBEAT = 0x08             // 心跳包
} ESPNowCommandType;

// 状态码枚举
typedef enum {
    STATUS_OK = 0x00,
    STATUS_ERROR = 0x01,
    STATUS_BUSY = 0x02,
    STATUS_TIMEOUT = 0x03,
    STATUS_INVALID_PARAM = 0x04
} ESPNowStatusCode;

// ESP-NOW数据包结构 (保持32字节以内以提高可靠性)
struct ESPNowPacket {
    uint16_t magic;                  // 魔数验证
    uint8_t handId;                  // 手部ID (0-255)
    uint8_t commandType;             // 命令类型
    uint16_t angle;                  // 舵机角度 (0-180)
    uint16_t pulseMin;               // 最小脉宽
    uint16_t pulseMax;               // 最大脉宽
    uint8_t feedLength;              // 喂料长度
    uint8_t reserved[4];             // 保留字段
    uint32_t sequence;               // 序列号
    uint16_t checksum;               // 校验和
} __attribute__((packed));

// 响应数据包结构
struct ESPNowResponse {
    uint16_t magic;                  // 魔数验证
    uint8_t handId;                  // 手部ID
    uint8_t commandType;             // 原始命令类型
    uint8_t status;                  // 状态码
    uint8_t reserved[3];             // 保留字段
    uint32_t sequence;               // 对应的序列号
    uint32_t timestamp;              // 时间戳
    char message[16];                // 状态消息
} __attribute__((packed));

// 系统配置
#define MAX_HANDS 8                  // 最大手部数量
#define ESPNOW_CHANNEL 0            // WiFi通道
#define PACKET_TIMEOUT_MS 1000      // 数据包超时时间

// 工具函数声明
uint16_t calculateChecksum(const ESPNowPacket* packet);
bool verifyChecksum(const ESPNowPacket* packet);
void setPacketChecksum(ESPNowPacket* packet);

#endif // ESPNOW_PROTOCOL_H
