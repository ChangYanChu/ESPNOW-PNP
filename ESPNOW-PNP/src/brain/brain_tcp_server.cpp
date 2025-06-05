#include "brain_tcp_server.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "gcode.h"

WiFiServer tcpServer(80); // TCP server will listen on port 80

void tcpServerSetup() {
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin("HONOR", "chu107610.");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("TCP Server started at IP: %s\n", WiFi.localIP().toString().c_str());
    tcpServer.begin();
}

void handleClient(WiFiClient client) {
    String inputBuffer = "";
    while (client.connected()) {
        if (client.available()) {
            char c = client.read();
            inputBuffer += c;

            if (c == '\n') {
                processCommand(inputBuffer);
                inputBuffer = ""; // Clear the buffer after processing
            }
        }
    }
    client.stop();
}

void tcpServerLoop() {
    WiFiClient client = tcpServer.available();
    if (client) {
        handleClient(client);
    }
}