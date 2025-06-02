#include "hand_servo.h"
#include "hand_status_led.h"

HandServo::HandServo() : attached(false), currentAngle(0), targetAngle(0), 
                        moving(false), moveStartTime(0), settleTime(SERVO_SETTLE_TIME),
                        statusLED(nullptr) {
    pendingOp.type = PendingOperation::NONE;
}

void HandServo::begin() {
    DEBUG_HAND_SERVO_PRINTF("Initializing servo on pin %d\n", SERVO_PIN);
    
    // ESP01S特殊处理 - 确保GPIO0在启动完成后才使用
    if (SERVO_PIN == 0) {
        // GPIO0在启动时用于启动控制，需要确保启动完成
        delay(2000);  // 等待启动完全完成
        DEBUG_HAND_SERVO_PRINT("GPIO0 ready for servo control after boot");
    }
    
    // 设置初始状态但不连接舵机
#ifdef SERVO_SILENT_MODE
    // 静默模式：设置为默认角度，实际连接时不会移动
    currentAngle = SERVO_INITIAL_ANGLE;
    targetAngle = SERVO_INITIAL_ANGLE;
    DEBUG_HAND_SERVO_PRINTF("Silent mode: Initial angle set to %d degrees\n", SERVO_INITIAL_ANGLE);
#else
    // 普通模式：从0开始
    currentAngle = 0;
    targetAngle = 0;
#endif
    
    pendingOp.type = PendingOperation::NONE;
    
    // 不进行连接测试，避免舵机在开机时移动
    // 舵机将在第一次需要使用时才连接
    DEBUG_HAND_SERVO_PRINT("HandServo initialized - servo will attach on first use");
}

void HandServo::update() {
    processPendingOperations();
    updateMovement();
}

bool HandServo::attach(uint16_t minPulse, uint16_t maxPulse) {
    if (attached) {
        DEBUG_HAND_SERVO_PRINT("Servo already attached, detaching first");
        servo.detach();
        delay(50); // 短暂延迟确保完全断开
    }
    
    // 如果与LED共用引脚，通知LED停止使用
    if (statusLED && SERVO_PIN == STATUS_LED_PIN) {
        statusLED->setServoActive(true);
    }
    
#ifdef SERVO_SILENT_MODE
    // 静默模式：仅在必要时连接舵机，并使用当前角度避免跳跃
    DEBUG_HAND_SERVO_PRINT("Silent mode: Attaching servo without initial movement");
    
    // ESP8266 Servo库的attach方法
    bool result = servo.attach(SERVO_PIN, minPulse, maxPulse);
    attached = result;
    
    if (result) {
        // 在静默模式下，立即写入当前角度以防止舵机跳跃
        if (currentAngle >= 0 && currentAngle <= 180) {
            servo.write(currentAngle);
            DEBUG_HAND_SERVO_PRINTF("Silent attach: Set to current angle %d to prevent movement\n", currentAngle);
        } else {
            // 如果当前角度无效，使用默认角度
            servo.write(SERVO_INITIAL_ANGLE);
            currentAngle = SERVO_INITIAL_ANGLE;
            DEBUG_HAND_SERVO_PRINTF("Silent attach: Set to default angle %d\n", SERVO_INITIAL_ANGLE);
        }
        
        DEBUG_HAND_SERVO_PRINTF("Servo silently attached to pin %d, pulse range: %d - %d\n", 
                                SERVO_PIN, minPulse, maxPulse);
    } else {
        DEBUG_HAND_SERVO_PRINT("Failed to attach servo!");
        // 失败时恢复LED使用
        if (statusLED && SERVO_PIN == STATUS_LED_PIN) {
            statusLED->setServoActive(false);
        }
    }
#else
    // 普通模式：标准连接
    bool result = servo.attach(SERVO_PIN, minPulse, maxPulse);
    attached = result;
    
    if (result) {
        DEBUG_HAND_SERVO_PRINTF("Servo successfully attached to pin %d, pulse range: %d - %d\n", 
                                SERVO_PIN, minPulse, maxPulse);
    } else {
        DEBUG_HAND_SERVO_PRINT("Failed to attach servo!");
        // 失败时恢复LED使用
        if (statusLED && SERVO_PIN == STATUS_LED_PIN) {
            statusLED->setServoActive(false);
        }
    }
#endif
    
    return result;
}

void HandServo::detach() {
    if (attached) {
        servo.detach();
        attached = false;
        moving = false;
        
        // 释放引脚后，LED可以重新使用
        if (statusLED && SERVO_PIN == STATUS_LED_PIN) {
            statusLED->setServoActive(false);
        }
        
        DEBUG_HAND_SERVO_PRINT("Servo detached");
    }
}

bool HandServo::setAngle(uint16_t angle) {
    if (angle > 180) {
        DEBUG_HAND_SERVO_PRINTF("Invalid angle: %d\n", angle);
        return false;
    }
    
    targetAngle = angle;
    
    if (!attached) {
        DEBUG_HAND_SERVO_PRINT("Servo not attached, attaching with default settings");
        
#ifdef SERVO_SILENT_MODE
        // 静默模式：在连接前先设置当前角度，避免跳跃
        if (currentAngle == 0) {
            // 如果当前角度还是初始值0，使用目标角度作为起始点
            currentAngle = angle;
            DEBUG_HAND_SERVO_PRINTF("Silent mode: Setting initial position to target angle %d\n", angle);
        }
#endif
        
        if (!attach()) {
            DEBUG_HAND_SERVO_PRINT("Failed to attach servo for angle setting");
            return false;
        }
    }
    
    DEBUG_HAND_SERVO_PRINTF("Writing angle %d to servo on pin %d\n", angle, SERVO_PIN);
    
    // 确保舵机已连接
    if (servo.attached()) {
        servo.write(angle);
        #ifdef DEBUG_VERBOSE_HAND_SERVO
        DEBUG_HAND_SERVO_PRINTF("Servo.write() called with angle: %d\n", angle);
        
        // 读回当前角度验证
        int readAngle = servo.read();
        DEBUG_HAND_SERVO_PRINTF("Servo.read() returned: %d\n", readAngle);
        #endif
    } else {
        DEBUG_HAND_SERVO_PRINT("ERROR: Servo not attached when trying to write angle!");
        return false;
    }
    
    currentAngle = angle;
    moving = true;
    moveStartTime = millis();
    
    return true;
}

void HandServo::requestAttach(uint16_t minPulse, uint16_t maxPulse) {
    DEBUG_HAND_SERVO_PRINTF("Requesting servo attach with pulse range: %d - %d\n", minPulse, maxPulse);
    
    pendingOp.type = PendingOperation::ATTACH;
    pendingOp.param1 = minPulse;
    pendingOp.param2 = maxPulse;
}

void HandServo::requestSetAngle(uint16_t angle) {
    DEBUG_HAND_SERVO_PRINTF("Requesting servo angle: %d\n", angle);
    
    pendingOp.type = PendingOperation::SET_ANGLE;
    pendingOp.param1 = angle;
    pendingOp.param2 = 0;
}

void HandServo::processPendingOperations() {
    if (pendingOp.type == PendingOperation::NONE) {
        return;
    }
    
    DEBUG_HAND_SERVO_PRINTF("Processing pending operation type: %d\n", pendingOp.type);
    
    switch (pendingOp.type) {
        case PendingOperation::ATTACH:
            DEBUG_HAND_SERVO_PRINT("Executing attach operation");
            attach(pendingOp.param1, pendingOp.param2);
            break;
            
        case PendingOperation::SET_ANGLE:
            DEBUG_HAND_SERVO_PRINT("Executing set angle operation");
            setAngle(pendingOp.param1);
            break;
            
        default:
            DEBUG_HAND_SERVO_PRINT("Unknown operation type");
            break;
    }
    
    pendingOp.type = PendingOperation::NONE;
    DEBUG_HAND_SERVO_PRINT("Pending operation completed");
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
