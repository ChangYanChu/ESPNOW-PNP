#include "hand_status_led.h"

HandStatusLED::HandStatusLED() : currentStatus(LED_OFF), lastUpdate(0), 
                                ledState(false), servoActive(false) {
}

void HandStatusLED::begin() {
    // 初始化LED引脚
    pinMode(STATUS_LED_PIN, OUTPUT);
    setLEDState(false);  // LED初始关闭
    
    // 启动时显示初始化状态
    setStatus(LED_INITIALIZING);
    
    DEBUG_HAND_PRINTF("Status LED initialized on pin %d\n", STATUS_LED_PIN);
}

void HandStatusLED::update() {
    updateLED();
}

void HandStatusLED::setStatus(LEDStatus status) {
    if (currentStatus != status) {
        currentStatus = status;
        lastUpdate = 0;  // 强制立即更新
        DEBUG_HAND_PRINTF("LED status changed to: %d\n", status);
    }
}

void HandStatusLED::setServoActive(bool active) {
    servoActive = active;
    if (active) {
        // 舵机激活时关闭LED避免干扰
        setLEDState(false);
    }
}

LEDStatus HandStatusLED::getStatus() const {
    return currentStatus;
}

void HandStatusLED::updateLED() {
    if (servoActive) {
        // 舵机使用期间不操作LED
        return;
    }
    
    unsigned long now = millis();
    unsigned long interval = getBlinkInterval();
    
    if (now - lastUpdate >= interval) {
        lastUpdate = now;
        
        switch (currentStatus) {
            case LED_OFF:
                setLEDState(false);
                break;
                
            case LED_WORKING:
                // 工作状态：短暂常亮后关闭
                setLEDState(true);
                delay(LED_ON_TIME);
                setLEDState(false);
                break;
                
            case LED_INITIALIZING:
            case LED_SEARCHING:
            case LED_CONNECTED:
            case LED_ERROR:
                // 闪烁模式
                ledState = !ledState;
                setLEDState(ledState);
                break;
        }
    }
}

unsigned long HandStatusLED::getBlinkInterval() const {
    switch (currentStatus) {
        case LED_INITIALIZING:
        case LED_SEARCHING:
            return LED_BLINK_FAST;
            
        case LED_CONNECTED:
            return LED_BLINK_SLOW;
            
        case LED_ERROR:
            return LED_BLINK_ERROR;
            
        case LED_WORKING:
            return LED_ON_TIME * 2;
            
        case LED_OFF:
        default:
            return 1000;  // 默认1秒
    }
}

void HandStatusLED::setLEDState(bool state) {
    if (!servoActive) {
        // ESP01S的LED通常是低电平点亮
        digitalWrite(STATUS_LED_PIN, STATUS_LED_INVERTED ? !state : state);
    }
}
