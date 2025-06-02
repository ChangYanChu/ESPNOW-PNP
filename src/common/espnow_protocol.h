#ifndef ESPNOW_PROTOCOL_H
#define ESPNOW_PROTOCOL_H

#include <Arduino.h>

// =============================================================================
// ESP-NOW通信协议定义
// =============================================================================

// 魔数用于验证数据包
#define ESPNOW_MAGIC 0xFEED

// 广播MAC地址
#define BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

// 命令类型枚举
typedef enum {
    CMD_SERVO_SET_ANGLE = 0x01,      // 设置舵机角度
    CMD_SERVO_ATTACH = 0x02,         // 连接舵机
    CMD_SERVO_DETACH = 0x03,         // 断开舵机
    CMD_FEEDER_ADVANCE = 0x04,       // 喂料推进
    CMD_FEEDER_RETRACT = 0x05,       // 喂料回缩
    CMD_STATUS_REQUEST = 0x06,       // 状态查询
    CMD_RESPONSE = 0x07,             // 响应命令
    CMD_HEARTBEAT = 0x08,            // 心跳包
    CMD_HAND_REGISTER = 0x09,        // Hand注册命令
    CMD_BRAIN_DISCOVERY = 0x0A,      // Brain发现广播
    CMD_REQUEST_REGISTRATION = 0x0B, // 请求Hand主动注册
    // 反馈系统命令
    CMD_CHECK_FEEDBACK = 0x0C,       // 查询反馈状态
    CMD_FEEDBACK_STATUS = 0x0D,      // 反馈状态报告
    CMD_ENABLE_FEEDBACK = 0x0E       // 启用/禁用反馈
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
    uint8_t feederId;                // 喂料器ID (用于注册)
    uint16_t angle;                  // 舵机角度 (0-180)
    uint16_t pulseMin;               // 最小脉宽
    uint16_t pulseMax;               // 最大脉宽
    uint8_t feedLength;              // 喂料长度
    uint8_t reserved[3];             // 保留字段 (减少1个字节)
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

// 反馈状态数据包结构
struct ESPNowFeedbackPacket {
    uint16_t magic;                  // 魔数验证
    uint8_t handId;                  // 手部ID
    uint8_t commandType;             // 命令类型 (CMD_FEEDBACK_STATUS)
    uint8_t feederId;                // 喂料器ID
    uint8_t tapeLoaded : 1;          // Tape装载状态 (1bit)
    uint8_t feedbackEnabled : 1;     // 反馈启用状态 (1bit)
    uint8_t manualFeedDetected : 1;  // 手动进料检测 (1bit)
    uint8_t reserved_bits : 5;       // 保留位 (5bits)
    uint16_t errorCount;             // 错误计数
    uint32_t lastCheckTime;          // 最后检查时间
    uint32_t sequence;               // 序列号
    uint16_t checksum;               // 校验和
    uint8_t padding[8];              // 填充到32字节
} __attribute__((packed));

// 反馈控制数据包结构
struct ESPNowFeedbackControl {
    uint16_t magic;                  // 魔数验证
    uint8_t handId;                  // 手部ID
    uint8_t commandType;             // 命令类型
    uint8_t feederId;                // 喂料器ID
    uint8_t enableFeedback;          // 启用反馈标志
    uint8_t clearFlags;              // 清除标志位
    uint8_t reserved[1];             // 保留字段
    uint32_t sequence;               // 序列号
    uint16_t checksum;               // 校验和
    uint8_t padding[14];             // 填充到32字节
} __attribute__((packed));

// 系统配置
#define MAX_HANDS 50                 // 最大手部数量 (与TOTAL_FEEDERS保持一致)
#define ESPNOW_CHANNEL 0            // WiFi通道
#define PACKET_TIMEOUT_MS 1000      // 数据包超时时间

// 工具函数声明
uint16_t calculateChecksum(const ESPNowPacket* packet);
bool verifyChecksum(const ESPNowPacket* packet);
void setPacketChecksum(ESPNowPacket* packet);

#endif // ESPNOW_PROTOCOL_H
