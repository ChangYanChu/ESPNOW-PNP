#include "brain_tcp_server.h"
#include "gcode.h"

WiFiServer server(8080);
WiFiClient client;
String tcpBuffer = "";
String inputBuffer1 = "";

void tcp_setup() {
    server.begin();
    Serial.println("TCP Server started on port 8080");
}

void tcp_loop() {
    // 检查新连接
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            client = server.available();
            Serial.println("TCP client connected");
        } else {
            server.available().stop(); // 拒绝新连接
        }
    }
    
    // 处理客户端数据
    if (client && client.connected() && client.available()) {
        while (client.available()) {
            char c = client.read();
            if (c == '\n') {
                tcpBuffer.trim();
                if (tcpBuffer.length() > 0) {
                    // 备份原始buffer
                    String backup = inputBuffer1;
                    inputBuffer1 = tcpBuffer;
                    
                    // 打印接收到的命令
                    // Serial.print("Received TCP command: ");
                    // Serial.println(inputBuffer1);

                    // 处理命令
                    processCommand();
                    
                    // 恢复buffer
                    inputBuffer1 = backup;
                }
                tcpBuffer = "";
            } else if (c != '\r') {
                tcpBuffer += c;
            }
        }
    }
}

// 重载sendAnswer以支持TCP输出
void tcpsendAnswer(uint8_t error, String message) {
    String response = (error == 0) ? "ok " : "error ";
    response += message;
    
    Serial.println(response);
    if (client && client.connected()) {
        client.println(response);
    }
}

void tcpsendAnswer(int error, const __FlashStringHelper *message) {
    String response = (error == 0) ? "ok " : "error ";
    response += String(message);
    
    Serial.println(response);
    if (client && client.connected()) {
        client.println(response);
    }
}