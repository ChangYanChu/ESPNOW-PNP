#ifndef HAND_SERVO_H
#define HAND_SERVO_H

#include <Arduino.h>
#include <Servo.h>  // 直接使用标准Servo库
#include "hand_config.h"

// 前向声明
class HandStatusLED;

class HandServo {
public:
    HandServo();
    
    void begin();
    void update();
    void setStatusLED(HandStatusLED* led) { statusLED = led; }
    
    // 舵机控制
    bool attach(uint16_t minPulse = SERVO_MIN_PULSE, uint16_t maxPulse = SERVO_MAX_PULSE);
    void detach();
    bool setAngle(uint16_t angle);
    
    // 异步舵机控制（避免在中断中阻塞）
    void requestAttach(uint16_t minPulse = SERVO_MIN_PULSE, uint16_t maxPulse = SERVO_MAX_PULSE);
    void requestSetAngle(uint16_t angle);
    
    // 状态查询
    bool isAttached() const { return attached; }
    uint16_t getCurrentAngle() const { return currentAngle; }
    bool isMoving() const { return moving; }
    
private:
    Servo servo;
    bool attached;
    uint16_t currentAngle;
    uint16_t targetAngle;
    bool moving;
    unsigned long moveStartTime;
    uint16_t settleTime;
    HandStatusLED* statusLED;  // LED状态管理
    
    // 异步操作队列
    struct PendingOperation {
        enum Type { NONE, ATTACH, SET_ANGLE } type;
        uint16_t param1;  // angle 或 minPulse
        uint16_t param2;  // maxPulse
    } pendingOp;
    
    void updateMovement();
    void processPendingOperations();
};

#endif // HAND_SERVO_H
