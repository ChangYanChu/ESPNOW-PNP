#ifndef HAND_SERVO_H
#define HAND_SERVO_H

#include <Arduino.h>
#include "hand_config.h"


// 舵机控制函数
void setup_Servo();
void testServoOnStartup(); // 开机测试舵机
void feedTapeAction(uint8_t feedLength);
void servoTick(); // 需要在主循环中调用

#endif