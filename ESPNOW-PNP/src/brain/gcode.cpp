#include "gcode.h"
#include "brain_espnow.h"
#include "brain_tcp_server.h"

String inputBuffer = ""; // Buffer for incoming G-Code lines

#define ENABLED 1
#define DISABLED 0
uint8_t feederEnabled = DISABLED;

bool validFeederNo(int8_t signedFeederNo, uint8_t feederNoMandatory = 0)
{
    if (signedFeederNo == -1 && feederNoMandatory >= 1)
    {
        return false;
    }
    else
    {
        if (signedFeederNo < 0 || signedFeederNo > (TOTAL_FEEDERS - 1))
        {
            return false;
        }
        else
        {
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

float parseParameter(char code, float defaultVal)
{
    int codePosition = inputBuffer.indexOf(code);
    if (codePosition != -1)
    {
        int delimiterPosition = inputBuffer.indexOf(" ", codePosition + 1);
        float parsedNumber = inputBuffer.substring(codePosition + 1, delimiterPosition).toFloat();
        return parsedNumber;
    }
    else
    {
        return defaultVal;
    }
}

void processCommand()
{
    int cmd = parseParameter('M', -1);

    switch (cmd)
    {
    case MCODE_SET_FEEDER_ENABLE:
    {
        int8_t _feederEnabled = parseParameter('S', -1);
        if ((_feederEnabled == 0 || _feederEnabled == 1))
        {
            if ((uint8_t)_feederEnabled == 1)
            {
                feederEnabled = ENABLED;
                sendAnswer(0, F("Feeder set enabled and operational"));
            }
            else
            {
                feederEnabled = DISABLED;
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

    case MCODE_ADVANCE:
    {
        if (feederEnabled != ENABLED)
        {
            sendAnswer(1, String(String("Enable feeder first! M") + String(MCODE_SET_FEEDER_ENABLE) + String(" S1")));
            break;
        }

        int8_t signedFeederNo = (int)parseParameter('N', -1);
        if (!validFeederNo(signedFeederNo, 1))
        {
            sendAnswer(1, F("feederNo missing or invalid"));
            break;
        }

        uint8_t feedLength = (uint8_t)parseParameter('F', 2);
        if (((feedLength % 2) != 0) || feedLength < 2 || feedLength > 24)
        {
            sendAnswer(1, F("Invalid feedLength, must be even number 2-24"));
            break;
        }

        bool triggerFeedOK = sendFeederAdvanceCommand((uint8_t)signedFeederNo, feedLength);
        if (!triggerFeedOK)
        {
            sendAnswer(1, F("feeder not OK (not activated, no tape or tension of cover tape not OK)"));
        }
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
        char receivedChar = (char)Serial.read();
        inputBuffer += receivedChar;

        if (receivedChar == '\n')
        {
            processCommand();
            inputBuffer = ""; // Clear buffer after processing
        }
    }
}