#include "brain_web.h"
#include "brain_config.h"
#include "brain_udp.h"     // 替换ESP-NOW为UDP
#include "gcode.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#if defined(ESP32)
#include <FS.h>
#include <LittleFS.h>
#elif defined(ESP8266)
#include <FS.h>
#include <LittleFS.h>
#endif

// External variable declarations
extern uint32_t lastHandResponse[TOTAL_FEEDERS];
extern FeederStatus feederStatusArray[TOTAL_FEEDERS];
extern uint32_t totalSessionFeeds;
extern uint32_t totalWorkCount;
extern UnassignedHand unassignedHands[10];

AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

// Helper function to get Feeder status
String getFeederStatusJSON() {
    DynamicJsonDocument doc(8192); // Further increase buffer size for all data
    JsonArray feeders = doc.createNestedArray("feeders");
    
    uint32_t now = millis();
    for (int i = 0; i < NUMBER_OF_FEEDER; i++) {
        JsonObject feeder = feeders.createNestedObject();
        feeder["id"] = i;
        
        // 使用UDP连接状态判断：0=离线, 1=在线空闲, 2=忙碌
        const char* handStatus = getHandStatusString(i);
        bool isOnline = (strcmp(handStatus, "在线") == 0 || strcmp(handStatus, "不稳定") == 0);
        
        if (isOnline) {
            if (feederStatusArray[i].waitingForResponse) {
                feeder["status"] = 2; // 忙碌
            } else {
                feeder["status"] = 1; // 在线空闲
            }
            feeder["lastSeen"] = 0; // UDP连接活跃
        } else {
            feeder["status"] = 0; // 离线
            feeder["lastSeen"] = -1;
        }
        
        // 添加统计信息
        feeder["totalFeedCount"] = feederStatusArray[i].totalFeedCount;
        feeder["sessionFeedCount"] = feederStatusArray[i].sessionFeedCount;
        feeder["totalPartCount"] = feederStatusArray[i].totalPartCount;
        feeder["remainingPartCount"] = feederStatusArray[i].remainingPartCount;
        feeder["componentName"] = String(feederStatusArray[i].componentName);
        feeder["packageType"] = String(feederStatusArray[i].packageType);
    }
    
    doc["onlineCount"] = getOnlineHandCount();
    doc["totalSessionFeeds"] = totalSessionFeeds;
    doc["totalWorkCount"] = totalWorkCount;
    doc["timestamp"] = now;
    
    String result;
    serializeJson(doc, result);
    return result;
}

// WebSocket事件处理
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected\n", client->id());
        // 发送初始状态
        client->text(getFeederStatusJSON());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0; // Null terminate
            String message = (char*)data;
            
            // Parse JSON message
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, message);
            
            if (!error && doc.containsKey("action")) {
                String action = doc["action"];
                if (action == "get_status") {
                    // 发送完整状态更新
                    client->text(getFeederStatusJSON());
                }
            }
        }
    }
}

// Web服务器初始化
void web_setup() {
    // 初始化LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    
    // 设置WebSocket
    ws.onEvent(onWsEvent);
    webServer.addHandler(&ws);
    
    // API endpoint: Get unassigned feeders (必须在 /api/feeders 之前注册)
    webServer.on("/api/feeders/unassigned", HTTP_GET, [](AsyncWebServerRequest *request){
      
        
        DynamicJsonDocument doc(2048);
        JsonArray feeders = doc.createNestedArray("feeders");
        uint32_t currentTime = millis();
        int unassignedCount = 0;
        
      
        
        // 查找未分配的Hand设备
        // 1. 检查unassignedHands数组中的设备
        for (int i = 0; i < 10; i++) {
            if (unassignedHands[i].isValid) {
              
                
                // 检查设备是否还在线（30秒内有心跳）
                if (currentTime - unassignedHands[i].lastSeen < 30000) {
                    
                    
                    JsonObject feeder = feeders.createNestedObject();
                    IPAddress deviceIP(unassignedHands[i].mac[0], unassignedHands[i].mac[1], 
                                     unassignedHands[i].mac[2], unassignedHands[i].mac[3]);
                    feeder["id"] = 255; // 未分配ID
                    feeder["ip"] = deviceIP.toString();
                    feeder["port"] = UDP_HAND_PORT;
                    feeder["status"] = 1; // 在线状态
                    feeder["info"] = String(unassignedHands[i].info);
                    feeder["lastSeen"] = currentTime - unassignedHands[i].lastSeen;
                    feeder["feederId"] = 255;
                    feeder["isUnassigned"] = true;
                    
                    // 添加默认的统计信息
                    feeder["totalFeedCount"] = 0;
                    feeder["sessionFeedCount"] = 0;
                    feeder["totalPartCount"] = 0;
                    feeder["remainingPartCount"] = 0;
                    feeder["componentName"] = "未分配";
                    feeder["packageType"] = "N/A";
                    
                    unassignedCount++;
                } else {
                  
                }
            }
        }
        
      
        
        // 2. 检查connectedHands数组中feederId为255的设备（备用检查）
        for (int i = 0; i < TOTAL_FEEDERS; i++) {
            if (connectedHands[i].isOnline && connectedHands[i].feederId == 255) {
              
                // 检查设备是否还在线（30秒内有心跳）
                if (currentTime - connectedHands[i].lastSeen < 30000) {
                    JsonObject feeder = feeders.createNestedObject();
                    feeder["id"] = 255; // 未分配ID
                    feeder["ip"] = connectedHands[i].ip.toString();
                    feeder["port"] = connectedHands[i].port;
                    feeder["status"] = 1; // 在线状态
                    feeder["info"] = String(connectedHands[i].handInfo);
                    feeder["lastSeen"] = currentTime - connectedHands[i].lastSeen;
                    feeder["feederId"] = 255;
                    feeder["isUnassigned"] = true;
                    
                    // 添加默认的统计信息
                    feeder["totalFeedCount"] = 0;
                    feeder["sessionFeedCount"] = 0;
                    feeder["totalPartCount"] = 0;
                    feeder["remainingPartCount"] = 0;
                    feeder["componentName"] = "未分配";
                    feeder["packageType"] = "N/A";
                    
                    unassignedCount++;
                }
            }
        }
        // 添加统计信息（与主API保持一致的结构）
        doc["onlineCount"] = unassignedCount;
        doc["totalSessionFeeds"] = totalSessionFeeds;
        doc["totalWorkCount"] = totalWorkCount;
        doc["timestamp"] = currentTime;
        doc["unassignedCount"] = unassignedCount; // 额外的未分配设备数量
        
        String result;
        serializeJson(doc, result);
       
        request->send(200, "application/json", result);
    });
    
    // API端点：获取所有Feeder状态
    webServer.on("/api/feeders", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getFeederStatusJSON());
    });
    
    // 调试API：获取UDP连接状态
    webServer.on("/api/debug/hands", HTTP_GET, [](AsyncWebServerRequest *request){
        DynamicJsonDocument doc(2048);
        JsonArray hands = doc.createNestedArray("hands");
        
        for (int i = 0; i < TOTAL_FEEDERS; i++) {
            JsonObject hand = hands.createNestedObject();
            hand["id"] = i;
            hand["status"] = getHandStatusString(i);
            hand["onlineCount"] = getOnlineHandCount();
        }
        
        String result;
        serializeJson(doc, result);
        request->send(200, "application/json", result);
    });
    
    // API端点：获取未分配Hand列表
    webServer.on("/api/unassigned", HTTP_GET, [](AsyncWebServerRequest *request){
        String response;
        getUnassignedHandsList(response);
        
        DynamicJsonDocument doc(1024);
        doc["data"] = response;
        
        String result;
        serializeJson(doc, result);
        request->send(200, "application/json", result);
    });
    
    // API endpoint: Update Feeder configuration
    webServer.on("/api/feeder/config", HTTP_PUT, [](AsyncWebServerRequest *request){}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, (char*)data);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        int id = doc["id"];
        if (id < 0 || id >= NUMBER_OF_FEEDER) {
            request->send(400, "application/json", "{\"error\":\"Invalid feeder ID\"}");
            return;
        }
        
        // Update configuration
        if (doc.containsKey("componentName")) {
            strncpy(feederStatusArray[id].componentName, doc["componentName"], sizeof(feederStatusArray[id].componentName) - 1);
        }
        if (doc.containsKey("packageType")) {
            strncpy(feederStatusArray[id].packageType, doc["packageType"], sizeof(feederStatusArray[id].packageType) - 1);
        }
        if (doc.containsKey("totalPartCount")) {
            feederStatusArray[id].totalPartCount = doc["totalPartCount"];
        }
        if (doc.containsKey("remainingPartCount")) {
            feederStatusArray[id].remainingPartCount = doc["remainingPartCount"];
        }
        
        // Save configuration
        saveFeederConfig();
        
        // 通过WebSocket推送单个Feeder配置更新
        if (ws.count() > 0) {
            DynamicJsonDocument updateDoc(512);
            JsonObject feeder = updateDoc.createNestedObject("feeder");
            feeder["id"] = id;
            feeder["componentName"] = feederStatusArray[id].componentName;
            feeder["packageType"] = feederStatusArray[id].packageType;
            feeder["totalPartCount"] = feederStatusArray[id].totalPartCount;
            feeder["remainingPartCount"] = feederStatusArray[id].remainingPartCount;
            feeder["sessionFeedCount"] = feederStatusArray[id].sessionFeedCount;
            feeder["totalFeedCount"] = feederStatusArray[id].totalFeedCount;
            
            // 添加状态信息
            const char* handStatus = getHandStatusString(id);
            bool isOnline = (strcmp(handStatus, "在线") == 0 || strcmp(handStatus, "不稳定") == 0);
            if (isOnline) {
                feeder["status"] = feederStatusArray[id].waitingForResponse ? 2 : 1;
            } else {
                feeder["status"] = 0;
            }
            
            String updateResult;
            serializeJson(updateDoc, updateResult);
            ws.textAll(updateResult);
        }
        
        request->send(200, "application/json", "{\"success\":true}");
    });
    
    // API endpoint: Find Me command for specific feeder
    webServer.on("/api/feeder/findme", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, (char*)data);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        if (!doc.containsKey("feederId")) {
            request->send(400, "application/json", "{\"error\":\"Missing feederId\"}");
            return;
        }
        
        int feederId = doc["feederId"];
        if (feederId < 0 || feederId >= NUMBER_OF_FEEDER) {
            String errorMsg = "{\"error\":\"Invalid feeder ID: " + String(feederId) + "\"}";
            request->send(400, "application/json", errorMsg);
            return;
        }
        
        // 检查Hand是否在线
        if (!connectedHands[feederId].isOnline) {
            String errorMsg = "{\"error\":\"Feeder " + String(feederId) + " is offline\"}";
            request->send(400, "application/json", errorMsg);
            return;
        }
        
        if (sendFindMeCommand(feederId)) {
            String successMsg = "{\"success\":true,\"message\":\"Find Me command sent to Feeder " + String(feederId) + "\"}";
            request->send(200, "application/json", successMsg);
        } else {
            String errorMsg = "{\"error\":\"Failed to send Find Me command to Feeder " + String(feederId) + "\"}";
            request->send(500, "application/json", errorMsg);
        }
    });
    
    // API endpoint: Assign feeder ID
    webServer.on("/api/feeder/assign", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, (char*)data);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        if (!doc.containsKey("feederId") || !doc.containsKey("ip") || !doc.containsKey("port")) {
            request->send(400, "application/json", "{\"error\":\"Missing required fields\"}");
            return;
        }
        
        int feederId = doc["feederId"];
        String ip = doc["ip"];
        int port = doc["port"];
        
        if (feederId < 0 || feederId >= NUMBER_OF_FEEDER) {
            request->send(400, "application/json", "{\"error\":\"Invalid feeder ID\"}");
            return;
        }
        
        // 发送设置ID命令到指定的Hand设备
        IPAddress deviceIP;
        if (!deviceIP.fromString(ip)) {
            request->send(400, "application/json", "{\"error\":\"Invalid IP address\"}");
            return;
        }
        
        // 调用UDP函数发送设置ID命令
        if (sendSetFeederIDCommandToDevice(deviceIP, port, feederId)) {
            String successMsg = "{\"success\":true,\"message\":\"Set Feeder ID command sent to " + ip + ":" + String(port) + "\"}";
            request->send(200, "application/json", successMsg);
        } else {
            String errorMsg = "{\"error\":\"Failed to send Set Feeder ID command to " + ip + ":" + String(port) + "\"}";
            request->send(500, "application/json", errorMsg);
        }
    });
    
    // API endpoint: Find Me for unassigned device
    webServer.on("/api/feeder/findme-unassigned", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, (char*)data);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        if (!doc.containsKey("ip") || !doc.containsKey("port")) {
            request->send(400, "application/json", "{\"error\":\"Missing required fields\"}");
            return;
        }
        
        String ip = doc["ip"];
        int port = doc["port"];
        
        // 向指定IP地址和端口发送Find Me命令
        IPAddress deviceIP;
        if (!deviceIP.fromString(ip)) {
            request->send(400, "application/json", "{\"error\":\"Invalid IP address\"}");
            return;
        }
        
        // 调用UDP函数发送Find Me命令
        if (sendFindMeCommandToDevice(deviceIP, port)) {
            String successMsg = "{\"success\":true,\"message\":\"Find Me command sent to " + ip + ":" + String(port) + "\"}";
            request->send(200, "application/json", successMsg);
        } else {
            String errorMsg = "{\"error\":\"Failed to send Find Me command to " + ip + ":" + String(port) + "\"}";
            request->send(500, "application/json", errorMsg);
        }
    });
    
    // 提供静态HTML页面
    webServer.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    webServer.begin();
    Serial.println("Web server started on port 80");
    Serial.printf("Visit: http://%s\n", WiFi.localIP().toString().c_str());
}

// Web服务器更新函数
void web_update() {
    static uint32_t lastStatusUpdate = 0;
    uint32_t now = millis();
    
    // 每10秒推送一次完整状态给所有WebSocket客户端
    if (now - lastStatusUpdate > 10000) {
        if (ws.count() > 0) {
            ws.textAll(getFeederStatusJSON());
        }
        lastStatusUpdate = now;
    }
}

// WebSocket通知函数（轻量级实现）
void notifyFeederStatusChange(uint8_t feederId, uint8_t status) {
    if (ws.count() > 0) {
        String msg = "{\"event\":\"status_change\",\"feederId\":" + String(feederId) + ",\"status\":" + String(status) + "}";
        ws.textAll(msg);
    }
}

void notifyCommandReceived(uint8_t feederId, uint8_t feedLength) {
    if (ws.count() > 0) {
        String msg = "{\"event\":\"command_received\",\"feederId\":" + String(feederId) + ",\"feedLength\":" + String(feedLength) + "}";
        ws.textAll(msg);
    }
}

void notifyCommandCompleted(uint8_t feederId, bool success, const char* message) {
    if (ws.count() > 0) {
        String msg = "{\"event\":\"command_completed\",\"feederId\":" + String(feederId) + ",\"success\":" + (success ? "true" : "false") + ",\"message\":\"" + String(message) + "\"}";
        ws.textAll(msg);
    }
}

void notifyHandOnline(uint8_t feederId) {
    if (ws.count() > 0) {
        String msg = "{\"event\":\"hand_online\",\"feederId\":" + String(feederId) + "}";
        ws.textAll(msg);
    }
}

void notifyHandOffline(uint8_t feederId) {
    if (ws.count() > 0) {
        String msg = "{\"event\":\"hand_offline\",\"feederId\":" + String(feederId) + "}";
        ws.textAll(msg);
    }
}
