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
#endif

    // 初始化按钮并设置回调
    initButton();
    setButtonDoubleClickCallback(onFeedButtonDoubleClick);

    // 初始化Feeder ID管理器
    initFeederID();

    // 初始化ESP-NOW
    espnow_setup();

    // 初始化舵机
    setup_Servo();

    DEBUG_PRINTLN("=== Setup Complete ===");
}

void loop()
{
    // 处理按钮事件
    handleButton();
    
    // 处理LED状态
    handleLED();

    // 调用舵机tick
    servoTick();

    // 处理ESP-NOW命令和响应
    processReceivedCommand();
    processPendingResponse();

    // 简单延迟，避免看门狗
    delay(1);
}