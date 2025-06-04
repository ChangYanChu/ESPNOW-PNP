#include <Arduino.h>
#include "brain_config.h"
#include "brain_espnow.h"
#include "gcode.h"
void setup()
{
    // 初始化串口
    Serial.begin(115200);
    while (!Serial && millis() < 5000)
        ;

    // 初始化ESP-NOW通信
    espnow_setup();
    // 等待系统稳定
    delay(200);
}

void loop()
{
    listenToSerialStream();
}
