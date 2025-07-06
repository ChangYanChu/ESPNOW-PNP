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
    unsigned long findMeEndTime = 0;
    bool ledState = false;
    LEDBlinkType currentType = LED_BLINK_FEED;
    LEDStatus currentStatus = LED_STATUS_OFF;
    unsigned long blinkInterval = 100; // 默认闪烁间隔
    bool findMeActive = false;
};

static LEDBlinkState ledState;

// 不同类型闪烁的间隔设置
static const unsigned long blinkIntervals[] = {
    100,  // LED_BLINK_FEED - 快速闪烁
    200,  // LED_BLINK_ERROR - 中速闪烁  
    150,  // LED_BLINK_SUCCESS - 中快闪烁
    80,   // LED_BLINK_FIND_ME - 超快速闪烁，与心跳区别明显
    500,  // LED_BLINK_WIFI_CONNECTING - 慢闪
    1000, // LED_BLINK_WIFI_CONNECTED - 很慢闪
    2000  // LED_BLINK_UNASSIGNED - 超慢闪
};

#endif

void initLED() {
#if !DEBUG_MODE
    pinMode(ESP01S_GPIO1, OUTPUT);
    digitalWrite(ESP01S_GPIO1, LOW);
    ledState.lastHeartbeatTime = millis();
    ledState.currentStatus = LED_STATUS_OFF;
    ledState.findMeActive = false;
#endif
}

void startLEDBlink(LEDBlinkType type, int blinks) {
#if !DEBUG_MODE
    // Find Me有优先权，除非是Find Me结束
    if (ledState.findMeActive && type != LED_BLINK_FIND_ME) {
        return;
    }
    
    ledState.isBlinking = true;
    ledState.blinkCount = 0;
    ledState.targetBlinks = blinks * 2; // 每次闪烁包含开和关
    ledState.lastBlinkTime = millis();
    ledState.ledState = false;
    ledState.currentType = type;
    ledState.blinkInterval = blinkIntervals[type];
    digitalWrite(ESP01S_GPIO1, LOW);
#endif
}

void setLEDStatus(LEDStatus status) {
#if !DEBUG_MODE
    ledState.currentStatus = status;
    
    // 根据状态设置相应的LED模式
    switch (status) {
        case LED_STATUS_OFF:
            ledState.isBlinking = false;
            digitalWrite(ESP01S_GPIO1, LOW);
            break;
            
        case LED_STATUS_WIFI_CONNECTING:
            startLEDBlink(LED_BLINK_WIFI_CONNECTING, 0); // 持续闪烁
            break;
            
        case LED_STATUS_WIFI_CONNECTED:
            startLEDBlink(LED_BLINK_UNASSIGNED, 0); // 慢闪表示未分配ID
            break;
            
        case LED_STATUS_READY:
        case LED_STATUS_HEARTBEAT:
            ledState.isBlinking = false;
            // 将在handleLED中处理心跳
            break;
            
        case LED_STATUS_WORKING:
            startLEDBlink(LED_BLINK_FEED, 3);
            break;
    }
#endif
}

void startFindMe(int duration_seconds) {
#if !DEBUG_MODE
    ledState.findMeActive = true;
    ledState.findMeEndTime = millis() + (duration_seconds * 1000);
    startLEDBlink(LED_BLINK_FIND_ME, 0); // 持续闪烁直到超时
#endif
}

void handleLED() {
#if !DEBUG_MODE
    unsigned long currentTime = millis();
    
    // 检查Find Me是否到期
    if (ledState.findMeActive && currentTime >= ledState.findMeEndTime) {
        ledState.findMeActive = false;
        ledState.isBlinking = false;
        // 恢复到之前的状态
        setLEDStatus(ledState.currentStatus);
        return;
    }
    
    if (ledState.isBlinking) {
        // 处理特定闪烁模式
        if (currentTime - ledState.lastBlinkTime >= ledState.blinkInterval) {
            ledState.ledState = !ledState.ledState;
            digitalWrite(ESP01S_GPIO1, ledState.ledState ? HIGH : LOW);
            ledState.blinkCount++;
            ledState.lastBlinkTime = currentTime;
            
            // 如果不是持续闪烁类型，检查是否完成
            if (ledState.targetBlinks > 0 && ledState.blinkCount >= ledState.targetBlinks) {
                ledState.isBlinking = false;
                digitalWrite(ESP01S_GPIO1, LOW); // 确保LED最终关闭
                
                // 恢复到状态指示模式
                if (!ledState.findMeActive) {
                    setLEDStatus(ledState.currentStatus);
                }
            }
        }
    } else if (!ledState.findMeActive) {
        // 正常状态指示
        switch (ledState.currentStatus) {
            case LED_STATUS_HEARTBEAT:
            case LED_STATUS_READY:
                // 心跳闪烁（正常状态指示） - 每秒快闪一次
                if (currentTime - ledState.lastHeartbeatTime > 1000) {
                    digitalWrite(ESP01S_GPIO1, HIGH);
                    delay(50); // 短暂亮起
                    digitalWrite(ESP01S_GPIO1, LOW);
                    ledState.lastHeartbeatTime = currentTime;
                }
                break;
                
            case LED_STATUS_OFF:
                digitalWrite(ESP01S_GPIO1, LOW);
                break;
                
            default:
                // 其他状态由闪烁处理
                break;
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

LEDStatus getCurrentLEDStatus() {
#if !DEBUG_MODE
    return ledState.currentStatus;
#else
    return LED_STATUS_OFF;
#endif
}