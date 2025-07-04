#include "gcode.h"
#include "brain_udp.h"    // 添加UDP功能支持
#include "lcd.h"
#include <WiFi.h>
#include "brain_tcp.h"

String inputBuffer = ""; // Buffer for incoming G-Code lines

// Add these lines if not already defined elsewhere:
#define FEEDER_ENABLED 1

// UnassignedHand结构在brain_udp.h中已定义
// extern UnassignedHand unassignedHands[10]; // 在brain_udp.h中已声明

#define FEEDER_DISABLED 0
uint8_t feederEnabled = FEEDER_DISABLED;

// Function to check if the feeder number is valid
bool validFeederNo(int8_t signedFeederNo, uint8_t feederNoMandatory = 0)
{
    if (signedFeederNo == -1 && feederNoMandatory >= 1)
    {
        // no number given (-1) but it is mandatory.
        return false;
    }
    else
    {
        // state now: number is given, check for valid range
        if (signedFeederNo < 0 || signedFeederNo > (NUMBER_OF_FEEDER - 1))
        {
            // error, number not in a valid range
            return false;
        }
        else
        {
            // perfectly fine number
            return true;
        }
    }
}

void sendAnswer(uint8_t error, String message)
{
    String response;
    if (error == 0)
    {
        Serial.print(F("ok "));
        response = "ok " + message;
    }
    else
    {
        Serial.print(F("error "));
        response = "error " + message;
    }

    Serial.println(message);
    
    // 如果有TCP客户端连接，也发送给TCP客户端
    WiFiClient* tcpClient = getCurrentTcpClient();
    if (tcpClient && tcpClient->connected()) {
        String fullResponse = "";
        if (error == 0) {
            tcpClient->print("ok ");
            fullResponse = "ok ";
        } else {
            tcpClient->print("error ");
            fullResponse = "error ";
        }
        tcpClient->println(message);
        fullResponse += message;
        tcpClient->flush(); // 确保数据发送
    }

#if HAS_LCD
    // 添加防抖逻辑，避免短时间内重复更新LCD
    static unsigned long last_lcd_update = 0;
    static String last_response = "";
    unsigned long now = millis();

    // 只在响应内容改变或距离上次更新超过200ms时才更新LCD
    if (response != last_response || (now - last_lcd_update > 200))
    {
        lcd_update_gcode("", response.c_str());
        last_lcd_update = now;
        last_response = response;
    }
#endif
}

void sendAnswer(int error, const __FlashStringHelper *message)
{
    // 构建响应
    String response = "";
    String msg = String(message); // 正确转换FlashStringHelper

    if (error == 0)
    {
        Serial.print(F("ok "));
        response = "ok";
        if (msg.length() > 0)
        {
            response += " " + msg;
        }
    }
    else
    {
        Serial.print(F("error "));
        response = "error " + msg;
    }

    Serial.println(msg);
    
    // 如果有TCP客户端连接，也发送给TCP客户端
    WiFiClient* tcpClient = getCurrentTcpClient();
    if (tcpClient && tcpClient->connected()) {
        String fullResponse = "";
        if (error == 0) {
            tcpClient->print("ok ");
            fullResponse = "ok ";
        } else {
            tcpClient->print("error ");
            fullResponse = "error ";
        }
        tcpClient->println(msg);
        fullResponse += msg;
        tcpClient->flush(); // 确保数据发送
    }

#if HAS_LCD
    // 添加防抖逻辑，避免短时间内重复更新LCD
    static unsigned long last_lcd_update = 0;
    static String last_response = "";
    unsigned long now = millis();

    // 只在响应内容改变或距离上次更新超过200ms时才更新LCD
    if (response != last_response || (now - last_lcd_update > 200))
    {
        lcd_update_gcode("", response.c_str());
        last_lcd_update = now;
        last_response = response;
    }
#endif
}

/**
 * 在 inputBuffer 中查找字符 /code/，并读取其后紧跟的浮点数。
 * @return 返回找到的值。如果未找到，则返回 /defaultVal/。
 * @param code 要查找的字符。
 * @param defaultVal 如果未找到 /code/ 时返回的默认值。
 **/
float parseParameter(char code, float defaultVal)
{
    int codePosition = inputBuffer.indexOf(code);
    if (codePosition != -1)
    {
        // code found in buffer

        // find end of number (separated by " " (space))
        int delimiterPosition = inputBuffer.indexOf(" ", codePosition + 1);

        float parsedNumber = inputBuffer.substring(codePosition + 1, delimiterPosition).toFloat();

        return parsedNumber;
    }
    else
    {
        return defaultVal;
    }
}

/**
 * 读取输入缓冲区并查找任何已识别的指令。每行只允许一个 G 或 M 指令。
 */
void processCommand()
{
#if HAS_LCD
    // 在LCD上显示接收到的G-code命令
    if (inputBuffer.length() > 0)
    {
        lcd_update_gcode(inputBuffer.c_str(), "");
    }
#endif // HAS_LCD

    // get the command, default -1 if no command found
    int cmd = parseParameter('M', -1);

#ifdef DEBUG
    Serial.print("command found: M");
    Serial.println(cmd);
#endif

    switch (cmd)
    {
    case MCODE_SET_FEEDER_ENABLE: // M610 S0 or S1
    {
        int8_t _feederEnabled = parseParameter('S', -1);
        if ((_feederEnabled == 0 || _feederEnabled == 1))
        {

            if ((uint8_t)_feederEnabled == 1)
            {
                feederEnabled = FEEDER_ENABLED;
                sendAnswer(0, F("Feeder set enabled and operational"));
            }
            else
            {
                feederEnabled = FEEDER_DISABLED;
                sendAnswer(0, F("Feeder set disabled"));
            }
        }
        else if (_feederEnabled == -1)
        {
            sendAnswer(0, ("current powerState: "));
        }
        else
        {
            sendAnswer(1, F("Invalid parameters"));
        }

        break;
    }

    case MCODE_ADVANCE: // M600 N0 F12 X1
    {
        // 取消检测是否使用M610 S1启用送料器
        // 1st to check: are feeder enabled?
        // if (feederEnabled != FEEDER_ENABLED)
        // {
        //     sendAnswer(1, String(String("Enable feeder first!") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1")));
        //     break;
        // }

        int8_t signedFeederNo = (int)parseParameter('N', -1);
        int8_t overrideErrorRaw = (int)parseParameter('X', -1);
        bool overrideError = false;
        if (overrideErrorRaw >= 1)
        {
            overrideError = true;
        }

        // 检查是否存在必需的 FeederNo
        if (!validFeederNo(signedFeederNo, 1))
        {
            sendAnswer(1, F("feederNo missing or invalid"));
            break;
        }

        // 确定送料长度
        uint8_t feedLength;
        // 如果有提供送料长度则获取，否则使用默认配置的 feed_length
        feedLength = (uint8_t)parseParameter('F', 2);

        if (((feedLength % 2) != 0) || feedLength < 2 || feedLength > 24)
        {
            // 送料长度必须为2的倍数且在2-18mm范围内
            sendAnswer(1, F("Invalid feedLength, must be even number 2-24"));
            break;
        }

        if (((feedLength % 2) != 0) || feedLength > 24)
        {
            // 送料长度只能为2的倍数且最大不超过24mm
            sendAnswer(1, F("Invalid feedLength"));
            break;
        }

#ifdef DEBUG
        Serial.print("Determined feedLength ");
        Serial.print(feedLength);
        Serial.println();
#endif

        // start feeding
        // 通过UDP发送命令到Hand，并等待响应后回复TCP客户端
        bool triggerFeedOK = sendFeederAdvanceCommand((uint8_t)signedFeederNo, feedLength, UDP_COMMAND_TIMEOUT_MS, true);
        if (!triggerFeedOK)
        {
            // UDP发送失败，立即报告错误
            sendAnswer(1, F("Failed to send feeder advance command"));
        }
        // 如果UDP发送成功，不立即回复
        // 等待Hand处理完成后通过UDP响应处理自动回复TCP客户端

        break;
    }

    case MCODE_GET_FEEDER_ID: // M620 N0
    {
        String response;
        getOnlineHandDetails(response);
        sendAnswer(0, response);
        break;
    }

    case 630: // MCODE_LIST_UNASSIGNED - 已迁移到Web界面
    {
        sendAnswer(0, "This command has been moved to Web interface. Please use 'Feeder Management' tab in the web interface.");
        break;
    }

    case 631: // MCODE_SET_HAND_ID - 已迁移到Web界面
    {
        sendAnswer(0, "This command has been moved to Web interface. Please use 'Feeder Management' tab to assign Feeder IDs.");
        break;
    }
       
    default:
        sendAnswer(0, F("unknown or empty command ignored"));

        break;
    }
}

void listenToSerialStream()
{

    while (Serial.available())
    {

        // get the received byte, convert to char for adding to buffer
        char receivedChar = (char)Serial.read();

// print back for debugging
#ifdef DEBUG
        Serial.print(receivedChar);
#endif

        // add to buffer
        inputBuffer += receivedChar;

        // if the received character is a newline, processCommand
        if (receivedChar == '\n')
        {

            // remove comments
            inputBuffer.remove(inputBuffer.indexOf(";"));
            inputBuffer.trim();

            processCommand();

            // clear buffer
            inputBuffer = "";
        }
    }
}
