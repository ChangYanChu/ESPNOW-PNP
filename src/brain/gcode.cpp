#include "gcode.h"
#include "brain_espnow.h"

String inputBuffer = ""; // Buffer for incoming G-Code lines

// Add these lines if not already defined elsewhere:
#define ENABLED 1
#define DISABLED 0
uint8_t feederEnabled = DISABLED;

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
    if (error == 0)
        Serial.print(F("ok "));
    else
        Serial.print(F("error "));

    Serial.println(message);
}

void sendAnswer(int error, const __FlashStringHelper *message)
{
    if (error == 0)
    {
        Serial.print("ok ");
    }
    else
    {
        Serial.print("error ");
    }
    Serial.println(message);
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
                // digitalWrite(FEEDER_ENABLE_PIN, HIGH);
                feederEnabled = ENABLED;

                // executeCommandOnAllFeeder(cmdEnable);

                sendAnswer(0, F("Feeder set enabled and operational"));
            }
            else
            {
                // digitalWrite(FEEDER_ENABLE_PIN, LOW);
                feederEnabled = DISABLED;

                // executeCommandOnAllFeeder(cmdDisable);

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
        // 1st to check: are feeder enabled?
        if (feederEnabled != ENABLED)
        {
            sendAnswer(1, String(String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1")));
            break;
        }

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
        // 这里修改成使用ESPNow进行发送
        bool triggerFeedOK = sendFeederAdvanceCommand((uint8_t)signedFeederNo, feedLength);
        if (!triggerFeedOK)
        {
            // report error to host at once, tape was not advanced...
            sendAnswer(1, F("feeder not OK (not activated, no tape or tension of cover tape not OK)"));
        }
        else
        {
            // 向主机回复OK（如果没有错误）-> 不，这里暂时不回复：
            // 等待送料过程完成后再发送OK，否则取料会过早开始。
            // 实际的消息在 feeder.cpp 中发送
        }

        break;
    }

        // case MCODE_RETRACT_POST_PICK:
        // {
        //     // 1st to check: are feeder enabled?
        //     if (feederEnabled != ENABLED)
        //     {
        //         sendAnswer(1, String(String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1")));
        //         break;
        //     }

        //     int8_t signedFeederNo = (int)parseParameter('N', -1);

        //     // check for presence of FeederNo
        //     if (!validFeederNo(signedFeederNo, 1))
        //     {
        //         sendAnswer(1, F("feederNo missing or invalid"));
        //         break;
        //     }

        //     feeders[(uint8_t)signedFeederNo].gotoPostPickPosition();

        //     sendAnswer(0, F("feeder postPickRetract done if needed"));

        //     break;
        // }

        // case MCODE_FEEDER_IS_OK:
        // {
        //     int8_t signedFeederNo = (int)parseParameter('N', -1);

        //     // check for presence of FeederNo
        //     if (!validFeederNo(signedFeederNo, 1))
        //     {
        //         sendAnswer(1, F("feederNo missing or invalid"));
        //         break;
        //     }

        //     sendAnswer(0, feeders[(uint8_t)signedFeederNo].reportFeederErrorState());

        //     break;
        // }

        // case MCODE_SERVO_SET_ANGLE:
        // {
        //     // 1st to check: are feeder enabled?
        //     if (feederEnabled != ENABLED)
        //     {
        //         sendAnswer(1, String(String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1")));
        //         break;
        //     }

        //     int8_t signedFeederNo = (int)parseParameter('N', -1);
        //     uint8_t angle = (int)parseParameter('A', 90);

        //     // check for presence of FeederNo
        //     if (!validFeederNo(signedFeederNo, 1))
        //     {
        //         sendAnswer(1, F("feederNo missing or invalid"));
        //         break;
        //     }
        //     // check for valid angle
        //     if (angle > 180)
        //     {
        //         sendAnswer(1, F("illegal angle"));
        //         break;
        //     }

        //     feeders[(uint8_t)signedFeederNo].gotoAngle(angle);

        //     sendAnswer(0, F("angle set"));

        //     break;
        // }

        //     case MCODE_UPDATE_FEEDER_CONFIG:
        //     {
        //         int8_t signedFeederNo = (int)parseParameter('N', -1);

        //         // check for presence of FeederNo
        //         if (!validFeederNo(signedFeederNo, 1))
        //         {
        //             sendAnswer(1, F("feederNo missing or invalid"));
        //             break;
        //         }

        //         // merge given parameters to old settings
        //         FeederClass::sFeederSettings oldFeederSettings = feeders[(uint8_t)signedFeederNo].getSettings();
        //         FeederClass::sFeederSettings updatedFeederSettings;
        //         updatedFeederSettings.full_advanced_angle = parseParameter('A', oldFeederSettings.full_advanced_angle);
        //         updatedFeederSettings.half_advanced_angle = parseParameter('B', oldFeederSettings.half_advanced_angle);
        //         updatedFeederSettings.retract_angle = parseParameter('C', oldFeederSettings.retract_angle);
        //         updatedFeederSettings.feed_length = parseParameter('F', oldFeederSettings.feed_length);
        //         updatedFeederSettings.time_to_settle = parseParameter('U', oldFeederSettings.time_to_settle);
        //         updatedFeederSettings.motor_min_pulsewidth = parseParameter('V', oldFeederSettings.motor_min_pulsewidth);
        //         updatedFeederSettings.motor_max_pulsewidth = parseParameter('W', oldFeederSettings.motor_max_pulsewidth);
        // #ifdef HAS_FEEDBACKLINES
        //         updatedFeederSettings.ignore_feedback = parseParameter('X', oldFeederSettings.ignore_feedback);
        // #endif

        //         // set to feeder
        //         feeders[(uint8_t)signedFeederNo].setSettings(updatedFeederSettings);

        //         // save to eeprom
        //         feeders[(uint8_t)signedFeederNo].saveFeederSettings();

        //         // reattach servo with new settings
        //         feeders[(uint8_t)signedFeederNo].setup();

        //         // confirm
        //         sendAnswer(0, F("Feeders config updated."));

        //         break;
        //     }
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
        // #ifdef DEBUG
        Serial.print(receivedChar);
        // #endif

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
