#ifndef ESPNOW_PROTOCOL_H
#define ESPNOW_PROTOCOL_H

#include <Arduino.h>

// =============================================================================
// ESP-NOW通信协议定义
// =============================================================================

// 命令类型枚举
typedef enum {
    CMD_FEEDER_ADVANCE = 0x04,       // 喂料推进
    CMD_STATUS_REQUEST = 0x06,       // 状态查询
    CMD_RESPONSE = 0x07,             // 响应命令
    CMD_HEARTBEAT = 0x08,            // 心跳包
    CMD_HAND_REGISTER = 0x09,        // Hand注册命令
    CMD_DISCOVERY = 0x0A,            // 发现Brain命令
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
    uint8_t commandType;             // 命令类型
    uint8_t feederId;                // 喂料器ID (用于注册)
    uint8_t feedLength;              // 喂料长度
    uint8_t reserved[4];             // 保留字段
} __attribute__((packed));

// 响应数据包结构
struct ESPNowResponse {
    uint8_t handId;                  // 手部ID
    uint8_t commandType;             // 原始命令类型
    uint8_t status;                  // 状态码
    uint8_t reserved[3];             // 保留字段
    uint32_t sequence;               // 对应的序列号
    uint32_t timestamp;              // 时间戳
    char message[16];                // 状态消息
} __attribute__((packed));


// 系统配置
#define MAX_HANDS 50                 // 最大手部数量 (与TOTAL_FEEDERS保持一致)
#endif // ESPNOW_PROTOCOL_H
