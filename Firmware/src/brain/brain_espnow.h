#ifndef BRAIN_ESPNOW_H
#define BRAIN_ESPNOW_H

#include <Arduino.h>
#include "brain_config.h"
#include "gcode.h"
void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast);
void espnow_setup();
void esp_update();
void processReceivedResponse();
void checkCommandTimeout();
void sendHeartbeat();
int getOnlineHandCount();
bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength, uint32_t timeoutMs = 0);

void initFeederStatus();

// 远程配置相关函数
bool sendSetFeederIDCommand(uint8_t targetMAC[6], uint8_t newFeederID);
void listUnassignedHands(String &response);

// 获取在线Hand详细信息
void getOnlineHandDetails(String &response);

// 添加按设备管理的状态结构
struct FeederStatus
{
    bool waitingForResponse;
    uint32_t commandSentTime;
    uint32_t timeoutMs;
    // 新增统计字段
    uint32_t totalFeedCount;        // 总送料次数
    uint16_t sessionFeedCount;      // 本次开机送料次数
    uint16_t totalPartCount;        // 总零件数量
    uint16_t remainingPartCount;    // 剩余零件数量
    char componentName[16];         // 元件名称（压缩长度）
    char packageType[8];            // 封装类型（压缩长度）
};

extern FeederStatus feederStatusArray[NUMBER_OF_FEEDER];

// 全局统计变量
extern uint32_t totalSessionFeeds;
extern uint32_t totalWorkCount;

// 外部变量声明，用于Web监控
extern uint32_t lastHandResponse[TOTAL_FEEDERS];

// 配置管理函数声明
void loadFeederConfig();
void saveFeederConfig();
void updateFeederStats(uint8_t feederId, bool success);

// Web通知函数声明
void notifyCommandReceived(uint8_t feederId, uint8_t feedLength);
void notifyCommandCompleted(uint8_t feederId, bool success, const char* message);
void notifyHandOnline(uint8_t feederId);
void notifyHandOffline(uint8_t feederId);

#endif // BRAIN_ESPNOW_H
