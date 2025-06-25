#ifndef HAND_CONFIG_H
#define HAND_CONFIG_H

#include <Arduino.h>
#include "../common/espnow_protocol.h"

// =============================================================================
// Hand控制器配置
// =============================================================================

#define HAND_VERSION "1.0.0"

#define TOTAL_FEEDERS 50 // 总喂料器数量

// 手部ID配置 (每个ESP01S需要设置不同的ID)
// 可以通过编译标志设置，例如: -D HAND_ID=0
#ifndef FEEDER_ID
#define FEEDER_ID 255 // 默认为255（未分配状态），支持远程配置
#endif

// 硬件配置 - ESP01S引脚分配
// ESP01S可用引脚: TX(GPIO1), RX(GPIO3), GPIO0, GPIO2
// 注意: GPIO0用于启动控制，GPIO2有板载LED
#define SERVO_PIN 2 // 舵机连接到GPIO2 (注意：此引脚也连接LED)

// 舵机测试配置
// #define ENABLE_SERVO_STARTUP_TEST  // 启用开机舵机测试（注释掉则禁用）
#define SERVO_TEST_DELAY 300       // 舵机测试每步延迟时间(毫秒)

// EEPROM配置
#define EEPROM_SIZE 512  // EEPROM大小
#define FEEDER_ID_ADDR 0 // Feeder ID存储地址
#endif

#define BUTTON_PIN 3  // GPIO3 (RXD) 连接按钮
#define BUTTON_ACTIVE_LOW true  // 按钮按下时为低电平（另一端接地）

// 串口调试控制宏
// 开发模式: 启用串口日志和命令
// 正常模式: 禁用串口，GPIO1可用作其他用途（如LED）
#define DEBUG_MODE 0  // 1=开发模式(启用串口), 0=正常模式(禁用串口)

// 如果启用调试模式，定义串口输出宏
#if DEBUG_MODE
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
    #define DEBUG_BEGIN(baud) Serial.begin(baud)
    #define DEBUG_AVAILABLE() Serial.available()
    #define DEBUG_READ_STRING_UNTIL(c) Serial.readStringUntil(c)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(fmt, ...)
    #define DEBUG_BEGIN(baud)
    #define DEBUG_AVAILABLE() 0
    #define DEBUG_READ_STRING_UNTIL(c) String("")
#endif
