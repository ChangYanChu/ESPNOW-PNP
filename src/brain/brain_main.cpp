#include <Arduino.h>
#include "brain_config.h"
#include "brain_gcode.h"
#include "brain_espnow.h"
#include "brain_feeder_manager.h"

// 全局对象
BrainGCode gcodeProcessor;
BrainESPNow espnowManager;
FeederManager feederManager;

void setup() {
    // 初始化串口
    Serial.begin(115200);
    while (!Serial && millis() < 5000);
    
    Serial.println(F("=== ESP32C3 Brain Controller Starting ==="));
    Serial.print(F("Version: "));
    Serial.println(BRAIN_VERSION);
    Serial.print(F("Max Hands: "));
    Serial.println(MAX_HANDS);
    
    // 初始化ESP-NOW通信
    if (!espnowManager.begin()) {
        Serial.println(F("ESP-NOW initialization failed!"));
        while(1) {
            delay(1000);
            Serial.println(F("System halted - ESP-NOW failed"));
        }
    }
    
    // 初始化喂料器管理器
    feederManager.begin(&espnowManager);
    
    // 初始化G-code处理器
    gcodeProcessor.begin(&feederManager);
    
    // 等待系统稳定
    delay(2000);
    
    Serial.println(F("=== Brain Controller Ready ==="));
    Serial.println(F("Supported G-code commands:"));
    Serial.println(F("M610 S1/S0 - Enable/Disable feeders"));
    Serial.println(F("M600 Nx Fy - Advance feeder x by y mm"));
    Serial.println(F("M601 Nx - Retract feeder x after pick"));
    Serial.println(F("M602 Nx - Check feeder x status"));
    Serial.println(F("M280 Nx Ay - Set feeder x servo to angle y"));
    Serial.println(F("M603 Nx... - Update feeder x configuration"));
    Serial.println(F("M620 - List all hands status"));
}

void loop() {
    // 处理G-code命令
    gcodeProcessor.update();
    
    // 更新ESP-NOW通信
    espnowManager.update();
    
    // 更新喂料器管理器
    feederManager.update();
    
    // 短暂延迟
    delay(10);
}
