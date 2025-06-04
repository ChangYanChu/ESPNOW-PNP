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
#define FEEDER_ID 0 // 默认为0，实际部署时需要修改
#endif

// 硬件配置 - ESP01S引脚分配
// ESP01S可用引脚: TX(GPIO1), RX(GPIO3), GPIO0, GPIO2
// 注意: GPIO0用于启动控制，GPIO2有板载LED
#define SERVO_PIN 2 // 舵机连接到GPIO2 (注意：此引脚也连接LED)

// 舵机配置
#define SERVO_INITIAL_ANGLE 90 // 舵机首次连接时的初始角度

// 舵机行为控制
// #define SERVO_ATTACH_ON_STARTUP  // 启用启动时立即连接舵机（会导致舵机移动）
#define SERVO_SILENT_MODE // 静默模式：舵机仅在明确指令时才连接和移动

// 舵机测试配置
#define ENABLE_SERVO_STARTUP_TEST  // 启用开机舵机测试（注释掉则禁用）
#define SERVO_TEST_DELAY 300       // 舵机测试每步延迟时间(毫秒)

// EEPROM配置
#define EEPROM_SIZE 512  // EEPROM大小
#define FEEDER_ID_ADDR 0 // Feeder ID存储地址
#endif