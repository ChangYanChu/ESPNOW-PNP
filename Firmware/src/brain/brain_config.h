#ifndef BRAIN_CONFIG_H
#define BRAIN_CONFIG_H

#include <Arduino.h>
#include "../common/common_config.h"


//=============================================================================
// Brain控制器专用配置
// =============================================================================

#define BRAIN_VERSION SYSTEM_VERSION  // 使用通用版本号
#define SERIAL_BAUD 115200

// LCD硬件支持配置
// 设置为 1 启用LCD显示，设置为 0 禁用LCD（适用于没有LCD硬件的brain）
#define HAS_LCD 0  // 1=有LCD硬件, 0=无LCD硬件

// G-code处理器配置
#define MAX_GCODE_LINE_LENGTH 64
#define GCODE_BUFFER_SIZE 128
#define MAX_UNASSIGNED_HANDS 10   // 最多跟踪10个未分配设备
#define UNASSIGNED_HAND_TIMEOUT_MS 30000  // 未分配设备超时时间（30秒）
// 调试配置 - Brain开发模式
#define DEBUG_MODE DEBUG_MODE_DISABLED  // 1=开发模式(启用串口), 0=正常模式(禁用串口)

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
