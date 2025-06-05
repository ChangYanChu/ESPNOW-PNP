#include "brain_espnow.h"
#include "gcode.h"
#include "../common/espnow_protocol.h"
#include "../common/tcp_protocol.h"
#include <Arduino.h>
#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#define WIFI_MODE_STA WIFI_STA
#else
#error "Unsupported platform"
#endif // ESP32
#include <QuickEspNow.h>

static const String msg = "Hello esp-now!";

#define USE_BROADCAST 1 // Set this to 1 to use broadcast communication

#if USE_BROADCAST != 1
static uint8_t receiver[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12};
#define DEST_ADDR receiver
#else
#define DEST_ADDR ESPNOW_BROADCAST_ADDRESS
#endif // USE_BROADCAST != 1

void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast)
{
    Serial.print("Received: ");
    Serial.printf("%.*s\n", len, data);
    Serial.printf("RSSI: %d dBm\n", rssi);
    Serial.printf("From: " MACSTR "\n", MAC2STR(address));
    Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");
}

void espnow_setup()
{
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin("HONOR", "chu107610.");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("Connected to %s in channel %d\n", WiFi.SSID().c_str(), WiFi.channel());
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
    quickEspNow.onDataRcvd(dataReceived);
    quickEspNow.begin();
}

void esp_update()
{
    static time_t lastSend = 60000;
    static unsigned int counter = 0;

    if (millis() - lastSend >= 1000)
    {
        lastSend = millis();
        String message = String(msg) + " " + String(counter++);
        if (!quickEspNow.send(DEST_ADDR, (uint8_t *)message.c_str(), message.length()))
        {
            sendAnswer(0, F("Feeder advance command sent"));
        }
        else
        {
            Serial.printf(">>>>>>>>>> Message not sent\n");
        }
    }
}

bool sendFeederAdvanceCommand(uint8_t feederId, uint8_t feedLength)
{
    Serial.printf("Sending feeder advance command: Feeder ID: %d, Feed Length: %d\n", feederId, feedLength);
    ESPNowPacket packet;
    packet.commandType = CMD_FEEDER_ADVANCE;
    packet.feederId = feederId;
    packet.feedLength = feedLength;
    memset(packet.reserved, 0, sizeof(packet.reserved));

    bool result = quickEspNow.send(DEST_ADDR, (uint8_t *)&packet, sizeof(packet));

    if (!result)
    {
        sendAnswer(0, F("Feeder advance command sent"));
        return true;
    }
    else
    {
        sendAnswer(1, F("Failed to send feeder advance command"));
        return false;
    }
}