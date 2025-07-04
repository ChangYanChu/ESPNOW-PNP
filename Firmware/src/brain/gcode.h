#ifndef GCODE_H
#define GCODE_H
#include <Arduino.h>
#include "brain_config.h"    // 使用brain_config.h中的定义

// #define NUMBER_OF_FEEDER 50 // 已在brain_config.h中定义

#define MCODE_ADVANCE 600 // 送料指令
#define MCODE_SET_FEEDER_ENABLE 610 // 启用或禁用送料器
#define MCODE_GET_FEEDER_ID 620 // 获取全部在线送料器ID
// #define MCODE_LIST_UNASSIGNED 630 // 列出未分配ID的Hand - 已迁移到Web界面
// #define MCODE_SET_HAND_ID 631 // 设置Hand的Feeder ID - 已迁移到Web界面


void listenToSerialStream();
void sendAnswer(uint8_t error, String message);
void sendAnswer(int error, const __FlashStringHelper* message);
void processCommand();
#endif // GCODE_H