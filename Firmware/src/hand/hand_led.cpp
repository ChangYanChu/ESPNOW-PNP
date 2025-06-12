#include "hand_led.h"
#include "hand_config.h"

#if !DEBUG_MODE

// LED闪烁状态结构
struct LEDBlinkState {
    bool isBlinking = false;
    int blinkCount = 0;
    int targetBlinks = 0;
    unsigned long lastBlinkTime = 0;
    unsigned long lastHeartbeatTime = 0;
    bool ledState = false;
    LEDBlinkType currentType = LED_BLINK_FEED;
    unsigned long blinkInterval = 100; // 默认闪烁间隔
};

static LEDBlinkState ledState;

// 不同类型闪烁的间隔设置
static const unsigned long blinkIntervals[] = {
    100,  // LED_BLINK_FEED - 快速闪烁
    200,  // LED_BLINK_ERROR - 中速闪烁  
    150   // LED_BLINK_SUCCESS - 中快闪烁
};

#endif

void initLED() {
#if !DEBUG_MODE
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
    ledState.lastHeartbeatTime = millis();
#endif
}

void startLEDBlink(LEDBlinkType type, int blinks) {
#if !DEBUG_MODE
    ledState.isBlinking = true;
    ledState.blinkCount = 0;
    ledState.targetBlinks = blinks * 2; // 每次闪烁包含开和关
    ledState.lastBlinkTime = millis();
    ledState.ledState = false;
    ledState.currentType = type;
    ledState.blinkInterval = blinkIntervals[type];
    digitalWrite(1, LOW);
#endif
}

void handleLED() {
#if !DEBUG_MODE
    unsigned long currentTime = millis();
    
    if (ledState.isBlinking) {
        // 处理特定闪烁模式
        if (currentTime - ledState.lastBlinkTime >= ledState.blinkInterval) {
            ledState.ledState = !ledState.ledState;
            digitalWrite(1, ledState.ledState ? HIGH : LOW);
            ledState.blinkCount++;
            ledState.lastBlinkTime = currentTime;
            
            if (ledState.blinkCount >= ledState.targetBlinks) {
                ledState.isBlinking = false;
                digitalWrite(1, LOW); // 确保LED最终关闭
            }
        }
    } else {
        // 心跳闪烁（正常状态指示）
        if (currentTime - ledState.lastHeartbeatTime > 1000) {
            digitalWrite(1, !digitalRead(1));
            ledState.lastHeartbeatTime = currentTime;
        }
    }
#endif
}

bool isLEDBlinking() {
#if !DEBUG_MODE
    return ledState.isBlinking;
#else
    return false;
#endif
}