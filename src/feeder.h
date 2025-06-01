#ifndef FEEDER_H
#define FEEDER_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include "config_esp32c3.h"

// =============================================================================
// 喂料器类定义
// =============================================================================

class FeederClass {
protected:
    // 喂料器编号
    int feederNo = -1;
    
    // 错误状态枚举
    enum tFeederErrorState {
        sOK = 0,
        sOK_NOFEEDBACKLINE = 1,
        sERROR_IGNORED = 2,
        sERROR = -1,
    };
    
    tFeederErrorState getFeederErrorState();

public:
    // 喂料器设置结构
    struct sFeederSettings {
        uint8_t full_advanced_angle;
        uint8_t half_advanced_angle;
        uint8_t retract_angle;
        uint8_t feed_length;
        int time_to_settle;
        int motor_min_pulsewidth;
        int motor_max_pulsewidth;
#ifdef HAS_FEEDBACKLINES  
        uint8_t ignore_feedback;
#endif
    };

    // 剩余喂料长度
    uint8_t remainingFeedLength = 4;

    // 喂料器状态枚举
    enum sFeederState {
        sFEEDER_DISABLED,
        sIDLE,
        sMOVING,
        sADVANCING_CYCLE_COMPLETED,
    } feederState = sFEEDER_DISABLED;

    // 喂料器位置枚举
    enum sFeederPosition {
        sAT_UNKNOWN,
        sAT_FULL_ADVANCED_POSITION,
        sAT_HALF_ADVANCED_POSITION,
        sAT_RETRACT_POSITION,
    } lastFeederPosition = sAT_UNKNOWN, feederPosition = sAT_UNKNOWN;

    // 时间戳记录
    unsigned long lastTimePositionChange;
    
    // 反馈线相关变量
#ifdef HAS_FEEDBACKLINES
    uint8_t feedbackLineTickCounter = 0;
    unsigned long lastTimeFeedbacklineCheck;
    int lastButtonState;
#endif

    // 设置存储
    sFeederSettings feederSettings = {
        FEEDER_DEFAULT_FULL_ADVANCED_ANGLE,
        FEEDER_DEFAULT_HALF_ADVANCED_ANGLE,
        FEEDER_DEFAULT_RETRACT_ANGLE,
        FEEDER_DEFAULT_FEED_LENGTH,
        FEEDER_DEFAULT_TIME_TO_SETTLE,
        FEEDER_DEFAULT_MOTOR_MIN_PULSEWIDTH,
        FEEDER_DEFAULT_MOTOR_MAX_PULSEWIDTH,
#ifdef HAS_FEEDBACKLINES
        FEEDER_DEFAULT_IGNORE_FEEDBACK,
#endif
    };

    // ESP32舵机对象
    Servo servo;
    
    // =============================================================================
    // 公共方法
    // =============================================================================
    
    void initialize(uint8_t _feederNo);
    bool isInitialized();
    bool hasFeedbackLine();
    
    void outputCurrentSettings();
    void setup();
    sFeederSettings getSettings();
    void setSettings(sFeederSettings UpdatedFeederSettings);
    void loadFeederSettings();
    void saveFeederSettings();
    void factoryReset();

    void gotoPostPickPosition();
    void gotoRetractPosition();
    void gotoRetractPositionSetup();
    void gotoHalfAdvancedPosition();
    void gotoFullAdvancedPosition();
    void gotoAngle(uint8_t angle);
    bool advance(uint8_t feedLength, bool overrideError = false);

    String reportFeederErrorState();
    bool feederIsOk();

    void enable();
    void disable();

    void update();

private:
    // Preferences对象用于存储设置
    Preferences prefs;
    
    // 私有辅助方法
    void detachServo();
    void attachServo();
};

#endif // FEEDER_H
