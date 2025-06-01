#ifndef BRAIN_GCODE_H
#define BRAIN_GCODE_H

#include <Arduino.h>
#include "brain_config.h"

// 前向声明
class FeederManager;

class BrainGCode {
public:
    BrainGCode();
    
    void begin(FeederManager* feederMgr);
    void update();
    
private:
    FeederManager* feederManager;
    String inputBuffer;
    bool systemEnabled;
    
    // G-code解析和处理
    void processSerialInput();
    void processCommand();
    float parseParameter(char code, float defaultVal);
    void sendResponse(bool success, const String& message);
    bool validateFeederNumber(int feederId);
    
    // M代码处理函数
    void processM610(); // 系统启用/禁用
    void processM600(); // 推进喂料
    void processM601(); // 取料后回缩
    void processM602(); // 查询状态
    void processM280(); // 设置舵机角度
    void processM603(); // 更新配置
    void processM620(); // 查询所有手部状态
};

#endif // BRAIN_GCODE_H
