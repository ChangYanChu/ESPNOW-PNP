#include <Arduino.h>
#include "brain_config.h"
#include "brain_udp.h"     // 使用UDP通信替代ESP-NOW
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

    // 初始化UDP通信
    Serial.println("Initializing UDP communication...");
    brain_udp_setup();

    // 初始化feeder状态数组
    initFeederStatus();

#if HAS_LCD
    lcd_update_system_status(SYSTEM_RUNNING);
#endif

    tcp_setup(); // 初始化TCP服务器
    web_setup(); // 初始化Web服务器

    // 输出网络信息
    Serial.printf("WiFi连接状态: %s\n", WiFi.status() == WL_CONNECTED ? "已连接" : "未连接");
    Serial.printf("本地IP地址: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("TCP服务器端口: 8080\n");
    Serial.printf("Web服务器端口: 80\n");
    Serial.printf("UDP监听端口: %d\n", UDP_BRAIN_PORT);

    // 等待系统稳定
    delay(200);
    Serial.println("系统初始化完成，进入主循环");
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

    // 处理UDP通信
    brain_udp_update();

    // 处理Web服务器更新
    web_update();

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
