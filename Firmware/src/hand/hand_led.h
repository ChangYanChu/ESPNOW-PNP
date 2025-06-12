#ifndef HAND_LED_H
#define HAND_LED_H

#include <Arduino.h>

// LED闪烁类型
enum LEDBlinkType {
    LED_BLINK_FEED = 0,     // 送料指示闪烁
    LED_BLINK_ERROR = 1,    // 错误指示闪烁
    LED_BLINK_SUCCESS = 2   // 成功指示闪烁
};

// LED控制函数
void initLED();
void startLEDBlink(LEDBlinkType type, int blinks = 3);
void handleLED();
bool isLEDBlinking();

#endif