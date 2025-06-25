#ifndef BRAIN_TCP_H
#define BRAIN_TCP_H

#include <WiFi.h>
#include <WiFiServer.h>

void tcp_setup();
void tcp_loop();
WiFiClient* getCurrentTcpClient(); // 获取当前TCP客户端
bool isTcpClientConnected();       // 检查TCP客户端是否连接

#endif // BRAIN_TCP_H