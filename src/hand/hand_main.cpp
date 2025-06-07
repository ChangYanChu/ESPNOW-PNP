#include <Arduino.h>
#include "hand_config.h"
#include "feeder_id_manager.h"
#include "hand_espnow.h"
#include "hand_servo.h"
void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== Hand Controller Starting ===");
    Serial.printf("Version: %s\n", HAND_VERSION);

    // 初始化Feeder ID管理器
    initFeederID();

    // 初始化ESP-NOW
    espnow_setup();

    setup_Servo();

    Serial.println("=== Setup Complete ===");
    Serial.println("Type 'HELP' for available commands");
}

void loop()
{

    // 必须频繁调用舵机tick
    servoTick();
    
    // 处理串口命令
    processSerialCommand();

    // 处理ESP-NOW
    // esp_update();
    processReceivedCommand();
    processPendingResponse(); // 处理待发送的响应

    // 其他循环逻辑...
    delay(10);
}