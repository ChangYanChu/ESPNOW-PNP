#ifndef HAND_LED_H
#define HAND_LED_H

#include <Arduino.h>

// LED闪烁类型
enum LEDBlinkType {
    LED_BLINK_FEED = 0,       // 送料指示闪烁
    LED_BLINK_ERROR = 1,      // 错误指示闪烁
    LED_BLINK_SUCCESS = 2,    // 成功指示闪烁
    LED_BLINK_FIND_ME = 3,    // Find Me闪烁
    LED_BLINK_WIFI_CONNECTING = 4,  // WiFi连接中
    LED_BLINK_WIFI_CONNECTED = 5,   // WiFi已连接
    LED_BLINK_UNASSIGNED = 6        // 未分配ID状态
};

// LED状态类型
enum LEDStatus {
    LED_STATUS_OFF = 0,           // 关闭
    LED_STATUS_HEARTBEAT = 1,     // 心跳（正常工作）
    LED_STATUS_WIFI_CONNECTING = 2, // WiFi连接中
    LED_STATUS_WIFI_CONNECTED = 3,  // WiFi已连接但未分配ID
    LED_STATUS_READY = 4,         // 就绪状态（已分配ID）
    LED_STATUS_WORKING = 5        // 工作中
};

// LED控制函数
void initLED();
void startLEDBlink(LEDBlinkType type, int blinks = 3);
void setLEDStatus(LEDStatus status);
void startFindMe(int duration_seconds = 10);
void handleLED();
bool isLEDBlinking();
LEDStatus getCurrentLEDStatus();

#endif