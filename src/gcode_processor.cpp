#include "gcode_processor.h"
#include "feeder.h"  // 添加feeder头文件包含
#include <Arduino.h>

// 外部声明的喂料器数组
extern FeederClass feeders[NUMBER_OF_FEEDER];

// G-code处理器实例
GCodeProcessor gcodeProcessor;

// 构造函数
GCodeProcessor::GCodeProcessor() : inputBuffer("") {
    inputBuffer.reserve(MAX_BUFFER_GCODE_LINE);
}

// 初始化G-code处理器
void GCodeProcessor::begin() {
    inputBuffer.reserve(MAX_BUFFER_GCODE_LINE);
    Serial.println(F("G-code processor initialized"));
}

// 查找参数值的核心函数
float GCodeProcessor::parseParameter(char code, float defaultVal) {
    int codePosition = inputBuffer.indexOf(code);
    if(codePosition != -1) {
        // 找到代码在缓冲区中的位置
        
        // 查找数字结束位置（由空格分隔）
        int delimiterPosition = inputBuffer.indexOf(" ", codePosition + 1);
        
        // 如果没找到空格，使用字符串末尾
        if(delimiterPosition == -1) {
            delimiterPosition = inputBuffer.length();
        }
        
        float parsedNumber = inputBuffer.substring(codePosition + 1, delimiterPosition).toFloat();
        return parsedNumber;
    } else {
        return defaultVal;
    }
}

// 发送应答信息
void GCodeProcessor::sendAnswer(uint8_t error, const String& message) {
    if(error == 0) {
        Serial.print(F("ok "));
    } else {
        Serial.print(F("error "));
    }
    Serial.println(message);
}

// 验证喂料器编号有效性
bool GCodeProcessor::validFeederNo(int8_t signedFeederNo, uint8_t feederNoMandatory) {
    if(signedFeederNo == -1 && feederNoMandatory >= 1) {
        // 没有给出编号(-1)但编号是必需的
        return false;
    } else {
        // 状态：给出了编号，检查有效范围
        if(signedFeederNo < 0 || signedFeederNo > (NUMBER_OF_FEEDER - 1)) {
            // 错误：编号不在有效范围内
            return false;
        } else {
            // 完全有效的编号
            return true;
        }
    }
}

// 处理M610命令 - 设置喂料器启用状态
void GCodeProcessor::processMCode610() {
    int8_t _feederEnabled = parseParameter('S', -1);
    
    if((_feederEnabled == 0 || _feederEnabled == 1)) {
        if((uint8_t)_feederEnabled == 1) {
            // 启用所有喂料器
            feederEnabled = FEEDER_ENABLED;
            for (uint8_t i = 0; i < NUMBER_OF_FEEDER; i++) {
                feeders[i].enable();
            }
            sendAnswer(0, F("Feeder set enabled and operational"));
        } else {
            // 禁用所有喂料器
            feederEnabled = FEEDER_DISABLED;
            for (uint8_t i = 0; i < NUMBER_OF_FEEDER; i++) {
                feeders[i].disable();
            }
            sendAnswer(0, F("Feeder set disabled"));
        }
    } else if(_feederEnabled == -1) {
        // 查询当前状态
        sendAnswer(0, String("current powerState: ") + String(feederEnabled));
    } else {
        sendAnswer(1, F("Invalid parameters"));
    }
}

// 处理M600命令 - 推进喂料
void GCodeProcessor::processMCode600() {
    // 首先检查：喂料器是否启用？
    if(feederEnabled != FEEDER_ENABLED) {
        sendAnswer(1, String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1"));
        return;
    }

    int8_t signedFeederNo = (int)parseParameter('N', -1);
    int8_t overrideErrorRaw = (int)parseParameter('X', -1);
    bool overrideError = false;
    
    if(overrideErrorRaw >= 1) {
        overrideError = true;
        #ifdef DEBUG
        Serial.println("Argument X1 found, feedbackline/error will be ignored");
        #endif
    }

    // 检查是否存在必需的喂料器编号
    if(!validFeederNo(signedFeederNo, 1)) {
        sendAnswer(1, F("feederNo missing or invalid"));
        return;
    }

    // 确定喂料长度
    uint8_t feedLength = (uint8_t)parseParameter('F', feeders[signedFeederNo].getSettings().feed_length);

    if(((feedLength % 2) != 0) || feedLength > 24) {
        sendAnswer(1, F("Invalid feedLength"));
        return;
    }
    
    #ifdef DEBUG
    Serial.print("Determined feedLength ");
    Serial.print(feedLength);
    Serial.println();
    #endif

    // 开始喂料
    bool triggerFeedOK = feeders[(uint8_t)signedFeederNo].advance(feedLength, overrideError);
    
    if(!triggerFeedOK) {
        sendAnswer(1, F("feeder not OK (not activated, no tape or tension of cover tape not OK)"));
    } else {
        sendAnswer(0, F("Feed command accepted"));
    }
}

// 处理M601命令 - 取料后回缩
void GCodeProcessor::processMCode601() {
    // 首先检查：喂料器是否启用？
    if(feederEnabled != FEEDER_ENABLED) {
        sendAnswer(1, String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1"));
        return;
    }

    int8_t signedFeederNo = (int)parseParameter('N', -1);

    // 检查喂料器编号是否存在
    if(!validFeederNo(signedFeederNo, 1)) {
        sendAnswer(1, F("feederNo missing or invalid"));
        return;
    }

    // 执行取料后位置回缩
    feeders[(uint8_t)signedFeederNo].gotoPostPickPosition();
    
    sendAnswer(0, F("feeder postPickRetract done if needed"));
}

// 处理M602命令 - 检查喂料器状态
void GCodeProcessor::processMCode602() {
    int8_t signedFeederNo = (int)parseParameter('N', -1);

    // 检查喂料器编号是否存在
    if(!validFeederNo(signedFeederNo, 1)) {
        sendAnswer(1, F("feederNo missing or invalid"));
        return;
    }

    // 返回喂料器状态报告
    sendAnswer(0, feeders[(uint8_t)signedFeederNo].reportFeederErrorState());
}

// 处理M280命令 - 设置舵机角度
void GCodeProcessor::processMCode280() {
    // 首先检查：喂料器是否启用？
    if(feederEnabled != FEEDER_ENABLED) {
        sendAnswer(1, String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1"));
        return;
    }

    int8_t signedFeederNo = (int)parseParameter('N', -1);
    uint8_t angle = (int)parseParameter('A', 90);

    // 检查喂料器编号是否存在
    if(!validFeederNo(signedFeederNo, 1)) {
        sendAnswer(1, F("feederNo missing or invalid"));
        return;
    }
    
    // 检查角度有效性
    if(angle > 180) {
        sendAnswer(1, F("illegal angle"));
        return;
    }

    // 设置舵机角度
    feeders[(uint8_t)signedFeederNo].gotoAngle(angle);
    
    sendAnswer(0, F("angle set"));
}

// 处理M603命令 - 更新喂料器配置
void GCodeProcessor::processMCode603() {
    int8_t signedFeederNo = (int)parseParameter('N', -1);

    // 检查是否存在必需的喂料器编号
    if(!validFeederNo(signedFeederNo, 1)) {
        sendAnswer(1, F("feederNo missing or invalid"));
        return;
    }

    // 获取当前设置
    FeederClass::sFeederSettings oldSettings = feeders[(uint8_t)signedFeederNo].getSettings();
    FeederClass::sFeederSettings updatedSettings;

    // 合并给定参数到旧设置
    updatedSettings.full_advanced_angle = parseParameter('A', oldSettings.full_advanced_angle);
    updatedSettings.half_advanced_angle = parseParameter('B', oldSettings.half_advanced_angle);
    updatedSettings.retract_angle = parseParameter('C', oldSettings.retract_angle);
    updatedSettings.feed_length = parseParameter('F', oldSettings.feed_length);
    updatedSettings.time_to_settle = parseParameter('U', oldSettings.time_to_settle);
    updatedSettings.motor_min_pulsewidth = parseParameter('V', oldSettings.motor_min_pulsewidth);
    updatedSettings.motor_max_pulsewidth = parseParameter('W', oldSettings.motor_max_pulsewidth);
#ifdef HAS_FEEDBACKLINES
    updatedSettings.ignore_feedback = parseParameter('X', oldSettings.ignore_feedback);
#endif
    
    // 设置到喂料器
    feeders[(uint8_t)signedFeederNo].setSettings(updatedSettings);

    // 保存到存储器
    feeders[(uint8_t)signedFeederNo].saveFeederSettings();

    // 重新设置舵机
    feeders[(uint8_t)signedFeederNo].setup();

    // 确认
    sendAnswer(0, F("Feeder config updated"));
}

// 处理命令的主函数
void GCodeProcessor::processCommand() {
    // 获取命令，如果没找到命令则默认为-1
    int cmd = parseParameter('M', -1);

    #ifdef DEBUG
    Serial.print("command found: M");
    Serial.println(cmd);
    #endif

    switch(cmd) {
        case MCODE_SET_FEEDER_ENABLE: {
            processMCode610();
            break;
        }

        case MCODE_ADVANCE: {
            processMCode600();
            break;
        }

        case MCODE_RETRACT_POST_PICK: {
            processMCode601();
            break;
        }

        case MCODE_FEEDER_IS_OK: {
            processMCode602();
            break;
        }

        case MCODE_SERVO_SET_ANGLE: {
            processMCode280();
            break;
        }

        case MCODE_UPDATE_FEEDER_CONFIG: {  // 新增M603命令处理
            processMCode603();
            break;
        }

        // TODO: 实现其他M代码命令
        // case MCODE_GET_ADC_RAW:
        // case MCODE_GET_ADC_SCALED:
        // case MCODE_SET_SCALING:
        // case MCODE_SET_POWER_OUTPUT:
        // case MCODE_FACTORY_RESET:

        default:
            sendAnswer(0, F("unknown or empty command ignored"));
            break;
    }
}

// 监听串口数据流
void GCodeProcessor::listenToSerialStream() {
    while(Serial.available()) {
        // 获取接收到的字节，转换为字符添加到缓冲区
        char receivedChar = (char)Serial.read();

        // 回显用于调试
        #ifdef DEBUG
        Serial.print(receivedChar);
        #endif

        // 添加到缓冲区
        inputBuffer += receivedChar;

        // 如果接收到的字符是换行符，则处理命令
        if(receivedChar == '\n') {
            // 移除注释
            int commentPos = inputBuffer.indexOf(";");
            if(commentPos != -1) {
                inputBuffer.remove(commentPos);
            }
            inputBuffer.trim();

            // 处理命令
            processCommand();

            // 清空缓冲区
            inputBuffer = "";
        }
    }
}

// 处理更新循环
void GCodeProcessor::update() {
    listenToSerialStream();
}
