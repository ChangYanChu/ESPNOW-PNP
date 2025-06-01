#ifndef HAND_CONFIG_H
#define HAND_CONFIG_H

#include <Arduino.h>
#include "../common/espnow_protocol.h"

// =============================================================================
// Hand控制器配置
// =============================================================================

#define HAND_VERSION "1.0.0"

// 手部ID配置 (每个ESP01S需要设置不同的ID)
// 可以通过编译标志设置，例如: -D HAND_ID=0
#ifndef HAND_ID
#define HAND_ID 0  // 默认为0，实际部署时需要修改
#endif

// 硬件配置
#define SERVO_PIN 2              // 舵机连接到GPIO2

// 舵机配置
#define SERVO_MIN_PULSE 500      // 最小脉宽 (微秒)
#define SERVO_MAX_PULSE 2400     // 最大脉宽 (微秒)
#define SERVO_FREQUENCY 50       // PWM频率 (Hz)

// 系统配置
#define HEARTBEAT_RESPONSE_INTERVAL 5000  // 心跳响应间隔 (毫秒)
#define STATUS_BLINK_INTERVAL 1000        // 状态LED闪烁间隔
#define SERVO_SETTLE_TIME 500             // 舵机稳定时间

// 性能优化配置
#define MIN_LOOP_DELAY 1                  // 最小循环延迟
#define SERVO_PROCESS_INTERVAL 50         // 舵机处理间隔
#define MAX_SERVO_MOVEMENTS_PER_SEC 5     // 限制舵机运动频率

// 调试配置 - 启用以查看详细信息
#define DEBUG_HAND
#define DEBUG_SERVO

// Brain控制器MAC地址 (需要根据实际ESP32C3填写)
extern uint8_t brain_mac_address[6];

#endif // HAND_CONFIG_H
