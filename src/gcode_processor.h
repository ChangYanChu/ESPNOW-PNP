#ifndef GCODE_PROCESSOR_H
#define GCODE_PROCESSOR_H

#include <Arduino.h>
#include "config_esp32c3.h"  // 包含配置文件

// 前向声明
class FeederClass;

// =============================================================================
// G-code处理器配置和常量定义
// =============================================================================

// 缓冲区大小配置
#define MAX_BUFFER_GCODE_LINE 64

// 喂料器状态枚举 - 重命名避免与ESP32框架冲突
enum eFeederEnabledState {
    FEEDER_DISABLED = 0,
    FEEDER_ENABLED = 1
};

// =============================================================================
// G-code处理器类定义
// =============================================================================

class GCodeProcessor {
public:
    // 构造函数
    GCodeProcessor();
    
    // 初始化函数
    void begin();
    
    // 主更新循环
    void update();
    
    // 监听串口数据流
    void listenToSerialStream();

private:
    // 输入缓冲区
    String inputBuffer;
    
    // 喂料器启用状态
    eFeederEnabledState feederEnabled = FEEDER_ENABLED;
    
    // =============================================================================
    // 内部处理函数
    // =============================================================================
    
    // 解析参数值
    float parseParameter(char code, float defaultVal);
    
    // 发送应答
    void sendAnswer(uint8_t error, const String& message);
    
    // 验证喂料器编号
    bool validFeederNo(int8_t signedFeederNo, uint8_t feederNoMandatory = 0);
    
    // 处理命令
    void processCommand();
    
    // =============================================================================
    // M代码命令处理函数
    // =============================================================================
    
    // M610 - 设置喂料器启用状态
    void processMCode610();
    
    // M600 - 推进喂料
    void processMCode600();
    
    // M601 - 取料后回缩
    void processMCode601();
    
    // M602 - 检查喂料器状态
    void processMCode602();
    
    // M280 - 设置舵机角度
    void processMCode280();
    
    // M603 - 更新喂料器配置
    void processMCode603();
    
    // TODO: 添加其他M代码处理函数
    // void processMCode650(); // 获取ADC原始值
    // void processMCode651(); // 获取ADC缩放值
    // void processMCode652(); // 设置缩放参数
    // void processMCode653(); // 设置电源输出
    // void processMCode999(); // 恢复出厂设置
};

// 全局G-code处理器实例
extern GCodeProcessor gcodeProcessor;

#endif // GCODE_PROCESSOR_H
