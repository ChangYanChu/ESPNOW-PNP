#include <Arduino.h>
#include <ESP32Servo.h>
#include "../src/config_esp32c3.h"

// 测试用舵机对象
Servo testServos[NUMBER_OF_FEEDER];

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000);
    
    Serial.println(F("=== ESP32C3 Hardware Test ==="));
    
    // 允许定时器分配
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    // 测试所有喂料器引脚
    for (int i = 0; i < NUMBER_OF_FEEDER; i++) {
        Serial.print(F("Testing Feeder "));
        Serial.print(i);
        Serial.print(F(" on GPIO "));
        Serial.println(feederPinMap[i]);
        
        // 连接舵机
        testServos[i].setPeriodHertz(50);
        testServos[i].attach(feederPinMap[i], 500, 2400);
        
        // 测试角度范围
        testServos[i].write(0);
        delay(500);
        testServos[i].write(90);
        delay(500);
        testServos[i].write(0);
        delay(500);
        
        // 断开连接
        testServos[i].detach();
        
        Serial.print(F("Feeder "));
        Serial.print(i);
        Serial.println(F(" test completed"));
    }
    
    Serial.println(F("=== Hardware Test Completed ==="));
    Serial.println(F("Send 'test' to run individual feeder test"));
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readString();
        command.trim();
        
        if (command.equals("test")) {
            Serial.println(F("Running individual feeder test..."));
            
            // 测试喂料器0
            Serial.println(F("Testing Feeder 0 sequence..."));
            testServos[0].attach(feederPinMap[0], 500, 2400);
            
            // 模拟喂料序列
            Serial.println(F("Step 1: Retract (0°)"));
            testServos[0].write(0);
            delay(1000);
            
            Serial.println(F("Step 2: Half advance (40°)"));
            testServos[0].write(40);
            delay(1000);
            
            Serial.println(F("Step 3: Full advance (80°)"));
            testServos[0].write(80);
            delay(1000);
            
            Serial.println(F("Step 4: Retract (0°)"));
            testServos[0].write(0);
            delay(1000);
            
            testServos[0].detach();
            Serial.println(F("Individual test completed"));
        }
    }
}
