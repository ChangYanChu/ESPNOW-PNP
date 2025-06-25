#ifndef FEEDER_ID_MANAGER_H
#define FEEDER_ID_MANAGER_H

#include <Arduino.h>

// 初始化Feeder ID管理器
void initFeederID();

// 保存Feeder ID到EEPROM
bool saveFeederID(uint8_t feederID);

// 获取当前Feeder ID
uint8_t getCurrentFeederID();

// 处理串口命令
void processSerialCommand();

// 打印帮助信息
void printHelp();

// 远程配置相关函数
bool isFeederIDUnassigned();
bool setFeederIDRemotely(uint8_t newID);

#endif