#include <Arduino.h>
#include "hand_config.h"
#include "feeder_id_manager.h"
#include "hand_espnow.h"
#include "hand_servo.h"
#include "hand_led.h"
#include "hand_button.h"

// 按钮双击回调函数
void onFeedButtonDoubleClick() {
    DEBUG_PRINTLN("Button double-clicked - Feeding once");
    
    // 执行送料动作
    feedOnce();
    
    // 启动LED闪烁指示
    startLEDBlink(LED_BLINK_FEED, 3);
}


void setup()
{
#if DEBUG_MODE
    DEBUG_BEGIN(115200);
    delay(1000);

    DEBUG_PRINTLN("\n=== Hand Controller Starting ===");
    DEBUG_PRINTF("Version: %s\n", HAND_VERSION);
#else
    // 正常模式下，GPIO1可用作LED等其他用途
    pinMode(1, OUTPUT); // 例如作为状态LED
#endif

    // 初始化按钮并设置回调
    initButton();
    setButtonDoubleClickCallback(onFeedButtonDoubleClick);
    
    DEBUG_PRINTLN("Button initialized on GPIO3");

    // 初始化Feeder ID管理器
    initFeederID();

    // 检查是否为未分配状态
    if (getCurrentFeederID() == 255) {
        DEBUG_PRINTLN("=== UNASSIGNED MODE ===");
        DEBUG_PRINTLN("Feeder ID not assigned, waiting for remote configuration...");
        DEBUG_PRINTF("MAC Address: %s\n", WiFi.macAddress().c_str());
    }

    // 初始化ESP-NOW
    espnow_setup();

    // 初始化舵机
    setup_Servo();

#if DEBUG_MODE
    DEBUG_PRINTLN("=== Setup Complete ===");
    DEBUG_PRINTLN("Type 'HELP' for available commands");
    DEBUG_PRINTLN("Double-click button to feed once");
#endif
}

void loop()
{
    // 处理按钮事件
    handleButton();
    
    // 处理LED状态
    handleLED();

    // 调用舵机tick
    servoTick();

    // 处理ESP-NOW
    processReceivedCommand();
    processPendingResponse(); // 处理待发送的响应

    // 其他循环逻辑...
    delay(1);
}