#include <Arduino.h>
#include "brain_config.h"
#include "brain_espnow.h"
#include "brain_web.h"
#include "gcode.h"
#include "lcd.h"
#include "brain_tcp.h"

void setup()
{
    // 初始化串口
    Serial.begin(115200);
    delay(1000); // 等待串口稳定
    Serial.println("ESP-NOW PNP Brain Controller Starting...");
#if HAS_LCD
    // 只有在有LCD硬件时才初始化LCD
    lcd_setup();
    lcd_update_system_status(SYSTEM_INIT);
    Serial.println("LCD initialized");
#else
    Serial.println("Running without LCD display");
#endif

    // 初始化ESP-NOW通信
    // Serial.println("Initializing ESP-NOW...");
    espnow_setup();

    // 初始化feeder状态数组
    initFeederStatus();

#if HAS_LCD
    lcd_update_system_status(SYSTEM_RUNNING);
#endif

    tcp_setup(); // 初始化TCP服务器
    web_setup(); // 初始化Web服务器

    // 等待系统稳定
    delay(200);
}

void loop()
{
#if HAS_LCD
    // 更新LCD显示 (只有在有LCD时才调用)
    lcd_update();
#endif

    // 处理串口G-code命令
    listenToSerialStream();

    // 处理TCP通信
    tcp_loop();

    // 处理接收到的ESP-NOW响应
    processReceivedResponse();

    // 检查命令超时
    checkCommandTimeout();

    // 发送心跳包检测Hand在线状态
    sendHeartbeat();

#if HAS_LCD
    // 更新在线手部数量 (只有在有LCD时才需要)
    static unsigned long last_hand_count_update = 0;
    if (millis() - last_hand_count_update > 5000)
    {
        int onlineHands = getOnlineHandCount();
        lcd_update_hand_count(onlineHands, TOTAL_FEEDERS);
        last_hand_count_update = millis();
    }
#endif

    // 添加一个小延迟以避免过度占用CPU
    delay(1);
}
