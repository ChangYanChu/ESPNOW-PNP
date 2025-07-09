#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <Arduino.h>

// =============================================================================
// 通用配置 - 所有设备共享
// =============================================================================

// 系统版本
#define SYSTEM_VERSION "1.0.0"

// 喂料器系统配置
#define TOTAL_FEEDERS 50        // 总喂料器数量
#define NUMBER_OF_FEEDER 50     // 支持的喂料器总数（与TOTAL_FEEDERS保持一致）
#define FEEDERS_PER_HAND 1      // 每个手控制的喂料器数量

// WiFi配置 - 统一配置点
#define WIFI_SSID "WIFI_SSID"  // WiFi SSID
#define WIFI_PASSWORD "WIFI_PASSWORD"
#define WIFI_POWER_MAX true     // 设置WiFi功率到最大

// 超时配置
#define COMMAND_TIMEOUT_MS 2000
#define HEARTBEAT_INTERVAL_MS 5000
#define HAND_OFFLINE_TIMEOUT_MS 10000

// 默认喂料器参数
#define DEFAULT_FULL_ADVANCE_ANGLE 90
#define DEFAULT_RETRACT_ANGLE 0
#define DEFAULT_FEED_LENGTH 4
#define DEFAULT_SETTLE_TIME 300

// EEPROM配置
#define EEPROM_SIZE 512         // EEPROM大小
#define FEEDER_ID_ADDR 0        // Feeder ID存储地址

// 调试配置宏定义
#define DEBUG_MODE_ENABLED 1    // 开发模式
#define DEBUG_MODE_DISABLED 0   // 正常模式

// 硬件引脚配置 - ESP01S通用引脚定义
// ESP01S可用引脚: TX(GPIO1), RX(GPIO3), GPIO0, GPIO2
// 注意: GPIO0用于启动控制，GPIO2有板载LED
#define ESP01S_GPIO0 0
#define ESP01S_GPIO1 1  // TX
#define ESP01S_GPIO2 2  // 板载LED
#define ESP01S_GPIO3 3  // RX

#endif // COMMON_CONFIG_H
