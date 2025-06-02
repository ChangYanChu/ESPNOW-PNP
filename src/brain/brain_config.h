#ifndef BRAIN_CONFIG_H
#define BRAIN_CONFIG_H

#include <Arduino.h>
#include "../common/espnow_protocol.h"

// =============================================================================
// Brain控制器配置
// =============================================================================

#define BRAIN_VERSION "1.0.0"
#define SERIAL_BAUD 115200

// =============================================================================
// 调试配置 - 可选择性启用不同模块的调试输出
// =============================================================================

// 主调试开关 - 关闭此项可禁用所有调试输出
// #define DEBUG_ENABLED   // 取消注释以启用调试输出，注释掉以关闭所有调试

#ifdef DEBUG_ENABLED
    // 模块调试开关 - 可单独控制各模块的调试输出
    // #define DEBUG_BRAIN              // Brain主控制器调试 - 暂时关闭
    // #define DEBUG_ESPNOW             // ESP-NOW通信调试 - 启用用于反馈通信  
    #define DEBUG_GCODE              // G-code处理调试 - 保留用于测试反馈命令
    #define DEBUG_FEEDER_MANAGER     // 喂料器管理调试 - 保留用于反馈系统
    // #define DEBUG_COMMANDS           // 命令执行调试 - 暂时关闭
    
    // 详细调试开关 - 更详细的调试信息
    // #define DEBUG_VERBOSE_ESPNOW     // ESP-NOW详细调试 - 暂时关闭
    // #define DEBUG_VERBOSE_GCODE      // G-code详细调试 - 暂时关闭
    // #define DEBUG_HEARTBEAT          // 心跳调试 - 暂时关闭
    // #define DEBUG_REGISTRATION       // 注册过程调试 - 暂时关闭
    
    // 反馈系统专用调试
    #define DEBUG_FEEDBACK_SYSTEM    // 反馈系统调试 - 新增
    
    // 反馈系统行为配置
    #define AUTO_PROCESS_MANUAL_FEED // 自动处理手动进料请求
#endif

// 调试宏定义
#ifdef DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(fmt, ...)
#endif

// 模块特定调试宏
#ifdef DEBUG_BRAIN
    #define DEBUG_BRAIN_PRINT(x) DEBUG_PRINT(F("[BRAIN] ")); DEBUG_PRINTLN(x)
#else
    #define DEBUG_BRAIN_PRINT(x)
#endif

#ifdef DEBUG_ESPNOW
    #define DEBUG_ESPNOW_PRINT(x) DEBUG_PRINT(F("[ESPNOW] ")); DEBUG_PRINTLN(x)
    #define DEBUG_ESPNOW_PRINTF(fmt, ...) DEBUG_PRINT(F("[ESPNOW] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
    
    // Brain ESP-NOW 特定调试宏
    #define DEBUG_BRAIN_ESPNOW_PRINT(x) DEBUG_PRINT(F("[BRAIN-ESPNOW] ")); DEBUG_PRINTLN(x)
    #define DEBUG_BRAIN_ESPNOW_PRINTF(fmt, ...) DEBUG_PRINT(F("[BRAIN-ESPNOW] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_ESPNOW_PRINT(x)
    #define DEBUG_ESPNOW_PRINTF(fmt, ...)
    #define DEBUG_BRAIN_ESPNOW_PRINT(x)
    #define DEBUG_BRAIN_ESPNOW_PRINTF(fmt, ...)
#endif

#ifdef DEBUG_GCODE
    #define DEBUG_GCODE_PRINT(x) DEBUG_PRINT(F("[GCODE] ")); DEBUG_PRINTLN(x)
    #define DEBUG_GCODE_PRINTF(fmt, ...) DEBUG_PRINT(F("[GCODE] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_GCODE_PRINT(x)
    #define DEBUG_GCODE_PRINTF(fmt, ...)
#endif

#ifdef DEBUG_FEEDER_MANAGER
    #define DEBUG_FEEDER_MANAGER_PRINT(x) DEBUG_PRINT(F("[FEEDER] ")); DEBUG_PRINTLN(x)
    #define DEBUG_FEEDER_MANAGER_PRINTF(fmt, ...) DEBUG_PRINT(F("[FEEDER] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_FEEDER_MANAGER_PRINT(x)
    #define DEBUG_FEEDER_MANAGER_PRINTF(fmt, ...)
#endif

#ifdef DEBUG_FEEDBACK_SYSTEM
    #define DEBUG_FEEDBACK_PRINT(x) DEBUG_PRINT(F("[FEEDBACK] ")); DEBUG_PRINTLN(x)
    #define DEBUG_FEEDBACK_PRINTF(fmt, ...) DEBUG_PRINT(F("[FEEDBACK] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_FEEDBACK_PRINT(x)
    #define DEBUG_FEEDBACK_PRINTF(fmt, ...)
#endif

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

// M代码定义
#define MCODE_FEEDER_ENABLE 610
#define MCODE_ADVANCE 600
#define MCODE_RETRACT 601
#define MCODE_STATUS 602
#define MCODE_SERVO_ANGLE 280
#define MCODE_CONFIG_UPDATE 603
#define MCODE_HANDS_STATUS 620
// 反馈系统M命令
#define MCODE_CHECK_FEEDBACK 604
#define MCODE_ENABLE_FEEDBACK 605
#define MCODE_CLEAR_MANUAL_FEED 606
#define MCODE_PROCESS_MANUAL_FEED 607

// 超时配置
#define COMMAND_TIMEOUT_MS 2000
#define HEARTBEAT_INTERVAL_MS 5000
#define HAND_OFFLINE_TIMEOUT_MS 10000

// 手部MAC地址配置 (需要根据实际ESP01S填写)
extern uint8_t hand_mac_addresses[MAX_HANDS][6];

#endif // BRAIN_CONFIG_H
