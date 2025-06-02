#ifndef HAND_CONFIG_H
#define HAND_CONFIG_H

#include <Arduino.h>
#include "../common/espnow_protocol.h"

// =============================================================================
// Hand控制器配置
// =============================================================================

#define HAND_VERSION "2.0.0"

// =============================================================================
// 调试配置 - 可选择性启用不同模块的调试输出
// =============================================================================

// 主调试开关 - 关闭此项可禁用所有调试输出
#define DEBUG_ENABLED   // 启用调试输出用于测试反馈系统

#ifdef DEBUG_ENABLED
    // 模块调试开关 - 只保留反馈系统相关的调试
    // #define DEBUG_HAND               // Hand主控制器调试 - 暂时关闭
    // #define DEBUG_HAND_ESPNOW        // ESP-NOW通信调试 - 启用用于反馈通信
    // #define DEBUG_HAND_SERVO         // 舵机控制调试 - 暂时关闭
    // #define DEBUG_HAND_CONFIG        // 配置管理调试 - 暂时关闭
    
    // 反馈系统专用调试
    #define DEBUG_HAND_FEEDBACK      // 反馈系统调试 - 保留
    
    // 详细调试开关
    // #define DEBUG_HAND_HEARTBEAT     // 心跳调试 - 暂时关闭
    // #define DEBUG_HAND_REGISTRATION  // 注册过程调试 - 暂时关闭
    // #define DEBUG_HAND_COMMANDS      // 命令处理调试 - 暂时关闭
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
#ifdef DEBUG_HAND
    #define DEBUG_HAND_PRINT(x) DEBUG_PRINT(F("[HAND] ")); DEBUG_PRINTLN(x)
    #define DEBUG_HAND_PRINTF(fmt, ...) DEBUG_PRINT(F("[HAND] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_HAND_PRINT(x)
    #define DEBUG_HAND_PRINTF(fmt, ...)
#endif

#ifdef DEBUG_HAND_ESPNOW
    #define DEBUG_HAND_ESPNOW_PRINT(x) DEBUG_PRINT(F("[HAND-ESPNOW] ")); DEBUG_PRINTLN(x)
    #define DEBUG_HAND_ESPNOW_PRINTF(fmt, ...) DEBUG_PRINT(F("[HAND-ESPNOW] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_HAND_ESPNOW_PRINT(x)
    #define DEBUG_HAND_ESPNOW_PRINTF(fmt, ...)
#endif

#ifdef DEBUG_HAND_SERVO
    #define DEBUG_HAND_SERVO_PRINT(x) DEBUG_PRINT(F("[HAND-SERVO] ")); DEBUG_PRINTLN(x)
    #define DEBUG_HAND_SERVO_PRINTF(fmt, ...) DEBUG_PRINT(F("[HAND-SERVO] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_HAND_SERVO_PRINT(x)
    #define DEBUG_HAND_SERVO_PRINTF(fmt, ...)
#endif

#ifdef DEBUG_HAND_FEEDBACK
    #define DEBUG_HAND_FEEDBACK_PRINT(x) DEBUG_PRINT(F("[FEEDBACK] ")); DEBUG_PRINTLN(x)
    #define DEBUG_HAND_FEEDBACK_PRINTF(fmt, ...) DEBUG_PRINT(F("[FEEDBACK] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_HAND_FEEDBACK_PRINT(x)
    #define DEBUG_HAND_FEEDBACK_PRINTF(fmt, ...)
#endif

#define TOTAL_FEEDERS 50             // 总喂料器数量

// 手部ID配置 (每个ESP01S需要设置不同的ID)
// 可以通过编译标志设置，例如: -D HAND_ID=0
#ifndef HAND_ID
#define HAND_ID 0  // 默认为0，实际部署时需要修改
#endif

// 硬件配置 - ESP01S引脚分配
// ESP01S可用引脚: TX(GPIO1), RX(GPIO3), GPIO0, GPIO2
// 注意: GPIO0用于启动控制，GPIO2有板载LED
#define SERVO_PIN 2              // 舵机连接到GPIO2 (注意：此引脚也连接LED)
#define STATUS_LED_PIN 2         // 状态LED引脚 (与舵机共用，需要合理调度)
#define STATUS_LED_INVERTED true // ESP01S的LED是低电平点亮

// 备用方案配置
// 如果GPIO2的LED干扰舵机，可以考虑以下配置：
// #define SERVO_PIN 0           // 使用GPIO0驱动舵机 (启动后可用)
// #define STATUS_LED_PIN 2      // 专用LED引脚

// 舵机配置
#define SERVO_MIN_PULSE 500      // 最小脉宽 (微秒)
#define SERVO_MAX_PULSE 2400     // 最大脉宽 (微秒)
#define SERVO_FREQUENCY 50       // PWM频率 (Hz)
#define SERVO_INITIAL_ANGLE 90   // 舵机首次连接时的初始角度

// 舵机行为控制
// #define SERVO_ATTACH_ON_STARTUP  // 启用启动时立即连接舵机（会导致舵机移动）
#define SERVO_SILENT_MODE        // 静默模式：舵机仅在明确指令时才连接和移动

// 舵机测试配置
// #define ENABLE_SERVO_STARTUP_TEST  // 启用启动时舵机功能测试
// #define SERVO_TEST_VERBOSE      // 详细的舵机测试输出
#define SERVO_TEST_DELAY 800       // 舵机测试每步延迟时间(毫秒)

// 系统配置
#define HEARTBEAT_RESPONSE_INTERVAL 5000  // 心跳响应间隔 (毫秒)
#define STATUS_BLINK_INTERVAL 1000        // 状态LED闪烁间隔
#define SERVO_SETTLE_TIME 500             // 舵机稳定时间

// 状态LED配置
#define LED_BLINK_FAST 200        // 快速闪烁间隔 (连接中)
#define LED_BLINK_SLOW 1000       // 慢速闪烁间隔 (已连接)
#define LED_BLINK_ERROR 100       // 错误闪烁间隔 (错误状态)
#define LED_ON_TIME 50            // LED点亮时间 (避免影响舵机)

// 性能优化配置
#define MIN_LOOP_DELAY 1                  // 最小循环延迟
#define SERVO_PROCESS_INTERVAL 50         // 舵机处理间隔
#define MAX_SERVO_MOVEMENTS_PER_SEC 5     // 限制舵机运动频率

// EEPROM配置
#define EEPROM_SIZE 512                   // EEPROM大小
#define FEEDER_ID_ADDR 0                  // Feeder ID存储地址
#define EEPROM_MAGIC_ADDR 1               // 魔数存储地址
#define EEPROM_MAGIC_VALUE 0xFE           // 魔数，用于验证EEPROM是否已初始化

// 调试配置 - 通过上述调试宏控制
// 为了向后兼容保留这些定义
#ifdef DEBUG_HAND
    // 兼容旧的调试开关
#endif

#ifdef DEBUG_HAND_SERVO
    // 兼容旧的调试开关  
#endif

// Brain控制器MAC地址 (需要根据实际ESP32C3填写)
extern uint8_t brain_mac_address[6];

// =============================================================================
// 反馈系统配置
// =============================================================================

// 反馈系统总开关
#define ENABLE_FEEDBACK_SYSTEM          // 启用反馈系统

#ifdef ENABLE_FEEDBACK_SYSTEM
    // 硬件配置
    #define FEEDBACK_PIN 0              // 反馈检测引脚 (GPIO0)
    
    // 检测参数
    #define FEEDBACK_DEBOUNCE_TIME 10   // 防抖时间 (毫秒)
    #define FEEDBACK_STABLE_COUNT 3     // 稳定状态计数阈值
    #define FEEDBACK_CHECK_INTERVAL 50  // 检测间隔 (毫秒)
    
    // 手动送料触发参数
    #define ENABLE_MANUAL_FEED          // 启用手动送料功能
    #define MANUAL_FEED_MIN_TIME 5      // 手动送料最短按压时间 (毫秒)
    #define MANUAL_FEED_MAX_TIME 200    // 手动送料最长按压时间 (毫秒) - 临时放宽用于测试
    
    // 状态报告配置
    #define FEEDBACK_REPORT_INTERVAL 10000  // 定期状态报告间隔 (毫秒)
    #define FEEDBACK_CHANGE_REPORT_DELAY 100 // 状态变化报告延迟 (毫秒)
    
    // 反馈系统调试
    #ifdef DEBUG_ENABLED
        #define DEBUG_FEEDBACK              // 启用反馈系统调试
    #endif
#endif

#ifdef DEBUG_FEEDBACK
    #define DEBUG_FEEDBACK_PRINT(x) DEBUG_PRINT(F("[FEEDBACK] ")); DEBUG_PRINTLN(x)
    #define DEBUG_FEEDBACK_PRINTF(fmt, ...) DEBUG_PRINT(F("[FEEDBACK] ")); DEBUG_PRINTF(fmt, __VA_ARGS__)
#else
    #define DEBUG_FEEDBACK_PRINT(x)
    #define DEBUG_FEEDBACK_PRINTF(fmt, ...)
#endif

#endif // HAND_CONFIG_H
