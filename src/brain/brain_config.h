#ifndef BRAIN_CONFIG_H
#define BRAIN_CONFIG_H

#include <Arduino.h>
#include "../common/espnow_protocol.h"

// =============================================================================
// Brain控制器配置
// =============================================================================

#define BRAIN_VERSION "1.0.0"
#define SERIAL_BAUD 115200

// 调试配置
// #define DEBUG_BRAIN
// #define DEBUG_ESPNOW
// #define DEBUG_GCODE

// G-code处理器配置
#define MAX_GCODE_LINE_LENGTH 64
#define GCODE_BUFFER_SIZE 128

// 喂料器配置
#define TOTAL_FEEDERS 8              // 总喂料器数量
#define FEEDERS_PER_HAND 1           // 每个手控制的喂料器数量

// 默认喂料器参数
#define DEFAULT_FULL_ADVANCE_ANGLE 80
#define DEFAULT_HALF_ADVANCE_ANGLE 40
#define DEFAULT_RETRACT_ANGLE 0
#define DEFAULT_FEED_LENGTH 4
#define DEFAULT_SETTLE_TIME 255
#define DEFAULT_MIN_PULSE 500
#define DEFAULT_MAX_PULSE 2400

// M代码定义
#define MCODE_FEEDER_ENABLE 610
#define MCODE_ADVANCE 600
#define MCODE_RETRACT 601
#define MCODE_STATUS 602
#define MCODE_SERVO_ANGLE 280
#define MCODE_CONFIG_UPDATE 603
#define MCODE_HANDS_STATUS 620

// 超时配置
#define COMMAND_TIMEOUT_MS 2000
#define HEARTBEAT_INTERVAL_MS 5000
#define HAND_OFFLINE_TIMEOUT_MS 10000

// 手部MAC地址配置 (需要根据实际ESP01S填写)
extern uint8_t hand_mac_addresses[MAX_HANDS][6];

#endif // BRAIN_CONFIG_H
