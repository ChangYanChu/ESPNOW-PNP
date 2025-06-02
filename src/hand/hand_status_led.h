#ifndef HAND_STATUS_LED_H
#define HAND_STATUS_LED_H

#include "hand_config.h"

enum LEDStatus {
    LED_OFF,           // LED关闭
    LED_INITIALIZING,  // 初始化中 - 快速闪烁
    LED_SEARCHING,     // 搜索Brain - 中速闪烁
    LED_CONNECTED,     // 已连接 - 慢速闪烁
    LED_WORKING,       // 工作中 - 常亮短暂时间
    LED_ERROR          // 错误状态 - 极快闪烁
};

class HandStatusLED {
private:
    LEDStatus currentStatus;
    unsigned long lastUpdate;
    bool ledState;
    bool servoActive;  // 标记舵机是否正在使用引脚
    
public:
    HandStatusLED();
    void begin();
    void update();
    void setStatus(LEDStatus status);
    void setServoActive(bool active);  // 舵机使用时禁用LED
    LEDStatus getStatus() const;
    
private:
    void updateLED();
    unsigned long getBlinkInterval() const;
    void setLEDState(bool state);
};

#endif // HAND_STATUS_LED_H
