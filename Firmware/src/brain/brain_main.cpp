#include <Arduino.h>
#include "brain_config.h"
#include "brain_espnow.h"
#include "gcode.h"
#include "lcd.h"

void setup()
{
    // 初始化串口
    Serial.begin(115200);
    while (!Serial && millis() < 5000)
        ;

    // Serial.println("ESP-NOW PNP Brain Controller Starting...");

    // 初始化LCD
    lcd_setup();
    lcd_update_system_status(SYSTEM_INIT);

    // 初始化ESP-NOW通信
    // Serial.println("Initializing ESP-NOW...");
    espnow_setup();

    // 初始化feeder状态数组
    initFeederStatus();

    // Serial.println("System ready!");
    lcd_update_system_status(SYSTEM_RUNNING);

    // 等待系统稳定
    delay(200);
}

void loop()
{
    // 更新LCD显示 (放在最前面确保及时更新)
    lcd_update();

    // 处理串口G-code命令
    listenToSerialStream();

    // 处理TCP通信
    // tcp_loop();

    // 处理接收到的ESP-NOW响应
    processReceivedResponse();

    // 检查命令超时
    checkCommandTimeout();

    // 发送心跳包检测Hand在线状态
    sendHeartbeat();

    // 更新在线手部数量
    static unsigned long last_hand_count_update = 0;
    if (millis() - last_hand_count_update > 5000)
    { // 每5秒更新一次
        int onlineHands = getOnlineHandCount();
        lcd_update_hand_count(onlineHands, TOTAL_FEEDERS);
        last_hand_count_update = millis();
    }

    // 添加一个小延迟以避免过度占用CPU
    delay(1);
}
