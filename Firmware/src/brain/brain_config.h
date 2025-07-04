#ifndef BRAIN_CONFIG_H
#define BRAIN_CONFIG_H

#include <Arduino.h>
// 注释掉不再需要的ESP-NOW协议包含，UDP协议已自行包含所需结构
// #include "../common/espnow_protocol.h"

// =============================================================================
// Brain控制器配置
// =============================================================================

#define BRAIN_VERSION "1.0.0"
#define SERIAL_BAUD 115200

// 喂料器数量定义（需要在其他配置之前定义）
#define NUMBER_OF_FEEDER 50 // 支持的喂料器总数
#define TOTAL_FEEDERS 50    // 总喂料器数量（与NUMBER_OF_FEEDER保持一致）

// LCD硬件支持配置
// 设置为 1 启用LCD显示，设置为 0 禁用LCD（适用于没有LCD硬件的brain）
#define HAS_LCD 0  // 1=有LCD硬件, 0=无LCD硬件

// G-code处理器配置
#define MAX_GCODE_LINE_LENGTH 64
#define GCODE_BUFFER_SIZE 128

// 喂料器配置
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

// WiFi配置
#define WIFI_SSID "HUAWEI-P99"
#define WIFI_PASSWORD "12345678"
#define WIFI_POWER_MAX true  // 设置WiFi功率到最大

// 调试配置
#define DEBUG_MODE 1  // 1=开发模式(启用串口), 0=正常模式(禁用串口)

// 调试宏定义
#if DEBUG_MODE
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
    #define DEBUG_BEGIN(baud) Serial.begin(baud)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(fmt, ...)
    #define DEBUG_BEGIN(baud)
#endif

// UDP通信已不需要手部MAC地址配置
#endif // BRAIN_CONFIG_H
