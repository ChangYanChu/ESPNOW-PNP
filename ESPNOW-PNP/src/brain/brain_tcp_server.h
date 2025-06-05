#ifndef BRAIN_TCP_SERVER_H
#define BRAIN_TCP_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

// TCP server configuration
#define TCP_SERVER_PORT 8080

// Function declarations
void tcpServerSetup();
void handleClientConnection();
void processGCodeCommand(String command);

#endif // BRAIN_TCP_SERVER_H