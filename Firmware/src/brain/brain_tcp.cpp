#include "brain_tcp.h"
#include "gcode.h"
#include "lcd.h"
#include <WiFi.h>

WiFiServer server(8080);
WiFiClient currentClient;  // 保存当前活动的客户端
String tcpBuffer = "";

// 外部全局变量，用于与gcode.cpp通信
extern String inputBuffer;

void tcp_setup() {
    server.begin();
    Serial.println("TCP Server started on port 8080");
    Serial.print("Server listening on IP: ");
    Serial.println(WiFi.localIP());
}

void tcp_loop() {
    // 检查是否有新的客户端连接
    if (server.hasClient()) {
        // 如果当前没有客户端或客户端已断开，接受新连接
        if (!currentClient || !currentClient.connected()) {
            currentClient = server.available();
            Serial.println("New Client Connected");
            Serial.print("Client IP: ");
            Serial.println(currentClient.remoteIP());
            
            // 发送欢迎消息
            currentClient.println("ok connected to Brain TCP Server");
            currentClient.flush();
            
            // 通知LCD更新TCP连接状态
            lcd_update_tcp_status(true);
            Serial.println("LCD notified: TCP client connected");
        } else {
            // 如果已有客户端连接，拒绝新连接
            WiFiClient newClient = server.available();
            newClient.stop();
            Serial.println("Additional client rejected - existing client still connected");
        }
    }

    // 处理当前客户端的数据
    if (currentClient && currentClient.connected()) {
        if (currentClient.available()) {
            while (currentClient.available()) {
                char c = currentClient.read();
                if (c == '\n') {
                    tcpBuffer.trim();
                    if (tcpBuffer.length() > 0) {
                        // 设置全局inputBuffer供processCommand使用
                        String backupBuffer = inputBuffer;
                        inputBuffer = tcpBuffer;
                        
                        Serial.print("Received TCP command: ");
                        Serial.println(inputBuffer);

                        // 处理命令
                        processCommand();
                        
                        // 恢复原始buffer
                        inputBuffer = backupBuffer;
                        // currentClient.println("Command processed: " + tcpBuffer);
                    }
                    tcpBuffer = "";
                } else if (c != '\r') {
                    tcpBuffer += c;
                }
            }
        }
    } else {
        // 添加调试信息
        if (currentClient) {
            Serial.println("TCP客户端已断开连接");
        }
    }
    
    // 检查客户端是否断开
    if (currentClient && !currentClient.connected()) {
        Serial.println("Client Disconnected");
        currentClient.stop();
        currentClient = WiFiClient(); // 重置客户端
        
        // 通知LCD更新TCP连接状态
        lcd_update_tcp_status(false);
        Serial.println("LCD notified: TCP client disconnected");
    }
}

// 获取当前TCP客户端的函数
WiFiClient* getCurrentTcpClient() {
    if (currentClient && currentClient.connected()) {
        return &currentClient;
    }
    
    // 调试信息
    if (!currentClient) {
        Serial.println("getCurrentTcpClient: currentClient为空");
    } else if (!currentClient.connected()) {
        Serial.println("getCurrentTcpClient: currentClient未连接");
    }
    
    return nullptr;
}

// 检查TCP客户端是否连接
bool isTcpClientConnected() {
    return currentClient && currentClient.connected();
}