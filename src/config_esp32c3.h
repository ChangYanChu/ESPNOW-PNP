#ifndef CONFIG_ESP32C3_H
#define CONFIG_ESP32C3_H

#include <Arduino.h>

// =============================================================================
// ESP32C3 硬件配置
// =============================================================================

// 串口配置
#define SERIAL_BAUD 115200

// 调试模式开关
// #define DEBUG

// 喂料器数量配置 - ESP32C3限制为6个LEDC通道
#define NUMBER_OF_FEEDER 6  // 减少到6个以适应ESP32C3硬件限制

// ESP32C3 GPIO引脚映射 (根据实际硬件连接修改)
const int feederPinMap[NUMBER_OF_FEEDER] = {
    2,  // Feeder 0 -> GPIO 2
    3,  // Feeder 1 -> GPIO 3
    4,  // Feeder 2 -> GPIO 4
    5,  // Feeder 3 -> GPIO 5
    6,  // Feeder 4 -> GPIO 6
    7   // Feeder 5 -> GPIO 7
    // 移除GPIO 8和9以避免LEDC通道不足
};

// 反馈线引脚映射 (如果使用)
#ifdef HAS_FEEDBACKLINES
const int feederFeedbackPinMap[NUMBER_OF_FEEDER] = {
    10, // Feeder 0 feedback -> GPIO 10
    18, // Feeder 1 feedback -> GPIO 18
    19, // Feeder 2 feedback -> GPIO 19
    20, // Feeder 3 feedback -> GPIO 20
    21, // Feeder 4 feedback -> GPIO 21
    -1, // Feeder 5 no feedback
    -1, // Feeder 6 no feedback
    -1  // Feeder 7 no feedback
};
#endif

// 电源输出引脚配置 - 使用不同的GPIO避免冲突
#define NUMBER_OF_POWER_OUTPUT 2
const int pwrOutputPinMap[NUMBER_OF_POWER_OUTPUT] = {
    8,  // Power output 0 -> GPIO 8 (原来的0可能与下载模式冲突)
    9   // Power output 1 -> GPIO 9
};

// =============================================================================
// 喂料器默认参数配置
// =============================================================================

// 舵机角度配置
#define FEEDER_DEFAULT_FULL_ADVANCED_ANGLE  80    // [°] 完全推进角度
#define FEEDER_DEFAULT_HALF_ADVANCED_ANGLE  40    // [°] 半推进角度 (用于2mm元件)
#define FEEDER_DEFAULT_RETRACT_ANGLE  0           // [°] 回缩角度

// 喂料长度配置
#define FEEDER_MECHANICAL_ADVANCE_LENGTH  4       // [mm] 机械推进长度
#define FEEDER_DEFAULT_FEED_LENGTH FEEDER_MECHANICAL_ADVANCE_LENGTH

// 时间配置
#define FEEDER_DEFAULT_TIME_TO_SETTLE  255        // [ms] 舵机稳定时间

// 舵机脉宽配置
#define FEEDER_DEFAULT_MOTOR_MIN_PULSEWIDTH 500   // [µs] 最小脉宽
#define FEEDER_DEFAULT_MOTOR_MAX_PULSEWIDTH 2400  // [µs] 最大脉宽

// 反馈配置
#define FEEDER_DEFAULT_IGNORE_FEEDBACK 0          // 0: 检查反馈, 1: 忽略反馈

// =============================================================================
// M代码命令定义
// =============================================================================

#define MCODE_SET_FEEDER_ENABLE 610
#define MCODE_ADVANCE 600
#define MCODE_RETRACT_POST_PICK 601
#define MCODE_FEEDER_IS_OK 602
#define MCODE_SERVO_SET_ANGLE 280
#define MCODE_UPDATE_FEEDER_CONFIG 603
#define MCODE_GET_ADC_RAW 650
#define MCODE_GET_ADC_SCALED 651
#define MCODE_SET_SCALING 652
#define MCODE_SET_POWER_OUTPUT 653
#define MCODE_FACTORY_RESET 999

// =============================================================================
// Preferences存储配置 (替代EEPROM)
// =============================================================================

#define CONFIG_VERSION "e01"  // ESP32C3版本标识
#define PREFS_NAMESPACE "feeder"
#define PREFS_KEY_VERSION "version"
#define PREFS_KEY_COMMON "common"
#define PREFS_KEY_FEEDER "feeder"

// =============================================================================
// ADC配置 (如果使用)
// =============================================================================

#ifdef HAS_ANALOG_IN
#define ADC_READ_EVERY_MS 50  // ESP32C3 ADC读取间隔

// ADC缩放参数
#define ANALOG_A0_SCALING_FACTOR 1.0
#define ANALOG_A0_OFFSET 0.0
#define ANALOG_A1_SCALING_FACTOR 1.0
#define ANALOG_A1_OFFSET 0.0
#define ANALOG_A2_SCALING_FACTOR 1.0
#define ANALOG_A2_OFFSET 0.0
#define ANALOG_A3_SCALING_FACTOR 1.0
#define ANALOG_A3_OFFSET 0.0
#endif

#endif // CONFIG_ESP32C3_H
