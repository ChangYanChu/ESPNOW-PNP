#include <Arduino.h>
#include "brain_config.h"
#include "brain_gcode.h"
#include "brain_espnow.h"
#include "brain_feeder_manager.h"

// 全局对象
BrainGCode gcodeProcessor;
BrainESPNow espnowManager;
FeederManager feederManager;

// 串口命令处理 - 只处理非G-code的管理命令
void checkSerialCommands() {
    // 不在这里读取串口，让G-code处理器优先处理
    // 这个函数现在用于处理特殊的管理命令
}

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
    
    // 设置ESP-NOW管理器的FeederManager引用
    espnowManager.setFeederManager(&feederManager);
    
    // 初始化G-code处理器
    gcodeProcessor.begin(&feederManager, &espnowManager);
    
    // 等待系统稳定
    delay(2000);
    
    Serial.println(F("=== Brain Controller Ready ==="));
    Serial.println(F("Manual commands: discovery, request_registration, clear_registration, status, help"));
    Serial.println(F("Supported G-code commands:"));
    Serial.println(F("M610 S1/S0 - Enable/Disable feeders"));
    Serial.println(F("M600 Nx Fy - Complete feed cycle for feeder x (advance + retract)"));
    Serial.println(F("M602 Nx - Query feeder x status"));
    Serial.println(F("M280 Nx Ay - Set feeder x servo to angle y"));
    Serial.println(F("M603 Nx Ay By Cy Fy - Update feeder x config"));
    Serial.println(F("M620 - Show all hands status"));
    Serial.println(F("Feedback system commands:"));
    Serial.println(F("M604 Nx - Check feedback status for feeder x"));
    Serial.println(F("M605 Nx S1/S0 - Enable/Disable feedback for feeder x"));
    Serial.println(F("M606 Nx - Clear manual feed flag for feeder x"));
    Serial.println(F("M607 Nx - Process manual feed for feeder x"));
}

void loop() {
    // 处理所有串口输入（G-code和管理命令）
    gcodeProcessor.update();
    
    // 更新ESP-NOW通信
    espnowManager.update();
    
    // 更新喂料器管理器
    feederManager.update();
    
    // 短暂延迟
    delay(10);
}
