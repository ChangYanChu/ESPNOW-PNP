#include <Arduino.h>
#include <ESP32Servo.h>
#include "config_esp32c3.h"  // 确保首先包含配置
#include "feeder.h"          // 然后包含feeder类定义
#include "gcode_processor.h" // 最后包含gcode处理器

// 全局喂料器数组 - 确保在这里定义
FeederClass feeders[NUMBER_OF_FEEDER];

// 全局命令枚举
enum eFeederCommands {
    cmdSetup,
    cmdUpdate,
    cmdEnable,
    cmdDisable,
    cmdOutputCurrentSettings,
    cmdInitializeFeederWithId,
    cmdFactoryReset,
};

// 对所有喂料器执行命令
void executeCommandOnAllFeeder(eFeederCommands command) {
    for (uint8_t i = 0; i < NUMBER_OF_FEEDER; i++) {
        switch(command) {
            case cmdSetup:
                feeders[i].setup();
                break;
            case cmdUpdate:
                feeders[i].update();
                break;
            case cmdEnable:
                feeders[i].enable();
                break;
            case cmdDisable:
                feeders[i].disable();
                break;
            case cmdOutputCurrentSettings:
                feeders[i].outputCurrentSettings();
                break;
            case cmdInitializeFeederWithId:
                feeders[i].initialize(i);
                break;
            case cmdFactoryReset:
                feeders[i].factoryReset();
                break;
            default:
                break;
        }
    }
}

void setup() {
    // 初始化串口通信
    Serial.begin(115200);
    while (!Serial && millis() < 5000); // 等待串口或超时5秒
    
    Serial.println(F("ESP32C3 Feeder Controller Starting..."));
    
    // 优化ESP32定时器分配 - 只分配需要的数量
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    // 不分配更多定时器，让舵机库按需分配
    
    // 初始化G-code处理器
    gcodeProcessor.begin();
    
    // 初始化喂料器ID
    executeCommandOnAllFeeder(cmdInitializeFeederWithId);
    
    // 设置喂料器对象
    executeCommandOnAllFeeder(cmdSetup);
    
    // 延迟确保舵机稳定
    delay(1000);
    
    // 启用喂料器
    executeCommandOnAllFeeder(cmdEnable);
    
    // 输出所有设置到控制台
    executeCommandOnAllFeeder(cmdOutputCurrentSettings);
    
    Serial.println(F("ESP32C3 Feeder Controller Ready!"));
    Serial.println(F("Supported G-code commands:"));
    Serial.println(F("M610 S1/S0 - Enable/Disable feeders"));
    Serial.println(F("M600 Nx Fy - Advance feeder x by y mm"));
    Serial.println(F("M601 Nx - Retract feeder x after pick"));
    Serial.println(F("M602 Nx - Check feeder x status"));
    Serial.println(F("M280 Nx Ay - Set feeder x servo to angle y"));
    Serial.println(F("M603 Nx Ay... - Update feeder x configuration"));
    Serial.println(F("Note: ESP32C3 supports max 6 feeders due to LEDC limitations"));
}

void loop() {
    // 处理G-code命令
    gcodeProcessor.update();
    
    // 更新所有喂料器状态
    executeCommandOnAllFeeder(cmdUpdate);
    
    // 短暂延迟以避免过度占用CPU
    delay(1);
}