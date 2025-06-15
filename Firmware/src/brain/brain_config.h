#ifndef BRAIN_CONFIG_H
#define BRAIN_CONFIG_H

#include <Arduino.h>
#include "../common/espnow_protocol.h"

// =============================================================================
// Brain控制器配置
// =============================================================================

#define BRAIN_VERSION "1.0.0"
#define SERIAL_BAUD 115200

// LCD硬件支持配置
// 设置为 1 启用LCD显示，设置为 0 禁用LCD（适用于没有LCD硬件的brain）
#define HAS_LCD 0  // 1=有LCD硬件, 0=无LCD硬件

// G-code处理器配置
#define MAX_GCODE_LINE_LENGTH 64
#define GCODE_BUFFER_SIZE 128

// 喂料器配置
#define TOTAL_FEEDERS 50             // 总喂料器数量
#define FEEDERS_PER_HAND 1           // 每个手控制的喂料器数量

// 默认喂料器参数
#define DEFAULT_FULL_ADVANCE_ANGLE 80
#define DEFAULT_HALF_ADVANCE_ANGLE 40
#define DEFAULT_RETRACT_ANGLE 0
#define DEFAULT_FEED_LENGTH 4
#define DEFAULT_SETTLE_TIME 255
#define DEFAULT_MIN_PULSE 500
#define DEFAULT_MAX_PULSE 2400

// 超时配置
#define COMMAND_TIMEOUT_MS 2000
#define HEARTBEAT_INTERVAL_MS 5000
#define HAND_OFFLINE_TIMEOUT_MS 10000

// 手部MAC地址配置 (需要根据实际ESP01S填写)
extern uint8_t hand_mac_addresses[MAX_HANDS][6];

#endif // BRAIN_CONFIG_H
