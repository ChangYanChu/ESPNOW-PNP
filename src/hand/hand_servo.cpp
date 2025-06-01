#include "hand_servo.h"

HandServo::HandServo() : attached(false), currentAngle(0), targetAngle(0), 
                        moving(false), moveStartTime(0), settleTime(SERVO_SETTLE_TIME) {
    pendingOp.type = PendingOperation::NONE;
}

void HandServo::begin() {
    Serial.print(F("Initializing servo on pin "));
    Serial.println(SERVO_PIN);
    
    // 设置初始状态但不连接舵机
    currentAngle = 0;
    targetAngle = 0;
    pendingOp.type = PendingOperation::NONE;
    
    // 立即连接舵机并设置到中间位置进行测试
    attach();
    servo.write(90);
    delay(1000);
    servo.write(0);
    delay(1000);
    detach();
    
    Serial.println(F("HandServo initialized and tested"));
}

void HandServo::update() {
    processPendingOperations();
    updateMovement();
}

bool HandServo::attach(uint16_t minPulse, uint16_t maxPulse) {
    if (attached) {
        Serial.println(F("Servo already attached, detaching first"));
        servo.detach();
        delay(50); // 短暂延迟确保完全断开
    }
    
    // ESP8266 Servo库的attach方法
    bool result = servo.attach(SERVO_PIN, minPulse, maxPulse);
    attached = result;
    
    if (result) {
        Serial.print(F("Servo successfully attached to pin "));
        Serial.print(SERVO_PIN);
        Serial.print(F(", pulse range: "));
        Serial.print(minPulse);
        Serial.print(F(" - "));
        Serial.println(maxPulse);
    } else {
        Serial.println(F("Failed to attach servo!"));
    }
    
    return result;
}

void HandServo::detach() {
    if (attached) {
        servo.detach();
        attached = false;
        moving = false;
        
        Serial.println(F("Servo detached"));
    }
}

bool HandServo::setAngle(uint16_t angle) {
    if (angle > 180) {
        Serial.print(F("Invalid angle: "));
        Serial.println(angle);
        return false;
    }
    
    targetAngle = angle;
    
    if (!attached) {
        Serial.println(F("Servo not attached, attaching with default settings"));
        if (!attach()) {
            Serial.println(F("Failed to attach servo for angle setting"));
            return false;
        }
    }
    
    Serial.print(F("Writing angle "));
    Serial.print(angle);
    Serial.print(F(" to servo on pin "));
    Serial.println(SERVO_PIN);
    
    // 确保舵机已连接
    if (servo.attached()) {
        servo.write(angle);
        Serial.print(F("Servo.write() called with angle: "));
        Serial.println(angle);
        
        // 读回当前角度验证
        int readAngle = servo.read();
        Serial.print(F("Servo.read() returned: "));
        Serial.println(readAngle);
    } else {
        Serial.println(F("ERROR: Servo not attached when trying to write angle!"));
        return false;
    }
    
    currentAngle = angle;
    moving = true;
    moveStartTime = millis();
    
    return true;
}

void HandServo::requestAttach(uint16_t minPulse, uint16_t maxPulse) {
    Serial.print(F("Requesting servo attach with pulse range: "));
    Serial.print(minPulse);
    Serial.print(F(" - "));
    Serial.println(maxPulse);
    
    pendingOp.type = PendingOperation::ATTACH;
    pendingOp.param1 = minPulse;
    pendingOp.param2 = maxPulse;
}

void HandServo::requestSetAngle(uint16_t angle) {
    Serial.print(F("Requesting servo angle: "));
    Serial.println(angle);
    
    pendingOp.type = PendingOperation::SET_ANGLE;
    pendingOp.param1 = angle;
    pendingOp.param2 = 0;
}

void HandServo::processPendingOperations() {
    if (pendingOp.type == PendingOperation::NONE) {
        return;
    }
    
    Serial.print(F("Processing pending operation type: "));
    Serial.println(pendingOp.type);
    
    switch (pendingOp.type) {
        case PendingOperation::ATTACH:
            Serial.println(F("Executing attach operation"));
            attach(pendingOp.param1, pendingOp.param2);
            break;
            
        case PendingOperation::SET_ANGLE:
            Serial.println(F("Executing set angle operation"));
            setAngle(pendingOp.param1);
            break;
            
        default:
            Serial.println(F("Unknown operation type"));
            break;
    }
    
    pendingOp.type = PendingOperation::NONE;
    Serial.println(F("Pending operation completed"));
}

void HandServo::updateMovement() {
    if (moving && attached) {
        // 检查是否到达稳定时间
        if (millis() - moveStartTime >= settleTime) {
            moving = false;
            
            #ifdef DEBUG_SERVO
            Serial.print(F("Servo settled at "));
            Serial.print(currentAngle);
            Serial.println(F(" degrees"));
            #endif
        }
    }
}
