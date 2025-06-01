#include <Arduino.h>
#include <ESP8266WiFi.h>  // 确保包含WiFi库
#include "hand_config.h"
#include "hand_espnow.h"
#include "hand_servo.h"

// 全局对象
HandESPNow espnowManager;
HandServo servoController;

void setup() {
    Serial.begin(115200);
    
    // 减少初始延迟
    for(int i = 0; i < 100; i++) {
        yield();
        if(i % 10 == 0) delay(10); // 短暂延迟
    }
    
    Serial.println();
    Serial.println(F("=== ESP01S Hand Controller Starting ==="));
    Serial.print(F("Hand ID: "));
    Serial.println(HAND_ID);
    Serial.print(F("Version: "));
    Serial.println(HAND_VERSION);
    
    // 初始化舵机控制器
    servoController.begin();
    
    // 简单的舵机功能测试
    Serial.println(F("Testing servo functionality..."));
    servoController.attach();
    delay(100);
    
    Serial.println(F("Moving to 90 degrees"));
    servoController.setAngle(90);
    delay(1000);
    
    Serial.println(F("Moving to 0 degrees"));
    servoController.setAngle(0);
    delay(1000);
    
    Serial.println(F("Moving to 180 degrees"));
    servoController.setAngle(180);
    delay(1000);
    
    Serial.println(F("Moving back to 90 degrees"));
    servoController.setAngle(90);
    delay(1000);
    
    servoController.detach();
    Serial.println(F("Servo test completed"));
    
    // 初始化ESP-NOW通信
    if (!espnowManager.begin(&servoController)) {
        Serial.println(F("ESP-NOW initialization failed!"));
        while(1) {
            yield();
            delay(100);
            Serial.println(F("System halted - ESP-NOW failed"));
        }
    }
    
    Serial.print(F("MAC Address: "));
    Serial.println(WiFi.macAddress());
    Serial.println(F("=== Hand Controller Ready ==="));
    
    // 发送初始状态 - 使用非阻塞方式
    for(int i = 0; i < 200; i++) {
        yield();
        if(i % 20 == 0) delay(10);
    }
    
    espnowManager.sendStatus(STATUS_OK, "Hand ready");
}

void loop() {
    // 更新舵机控制器（处理异步操作）
    servoController.update();
    
    // 更新ESP-NOW通信
    espnowManager.update();
    
    // 输出舵机状态信息（每5秒一次）
    static unsigned long lastStatusOutput = 0;
    if (millis() - lastStatusOutput > 5000) {
        lastStatusOutput = millis();
        Serial.print(F("Servo status - Attached: "));
        Serial.print(servoController.isAttached() ? "YES" : "NO");
        Serial.print(F(", Current angle: "));
        Serial.print(servoController.getCurrentAngle());
        Serial.print(F(", Moving: "));
        Serial.println(servoController.isMoving() ? "YES" : "NO");
    }
    
    // 给系统更多时间处理其他任务
    yield();
    
    // 非常短暂的延迟
    delay(1);
}
