#ifndef TCP_PROTOCOL_H
#define TCP_PROTOCOL_H

#include <Arduino.h>

// TCP Protocol Constants
#define TCP_PORT 8080
#define MAX_TCP_CONNECTIONS 5
#define MAX_TCP_MESSAGE_LENGTH 256

// TCP Command Identifiers
#define CMD_GCODE 0x01
#define CMD_STATUS 0x02
#define CMD_HEARTBEAT 0x03

// Structure for TCP Command
typedef struct {
    uint8_t commandId;
    char message[MAX_TCP_MESSAGE_LENGTH];
} TcpCommand;

// Function Prototypes
void handleTcpConnection();
void sendTcpResponse(int clientSocket, const char* response);
void parseTcpCommand(const char* buffer, TcpCommand* command);

#endif // TCP_PROTOCOL_H