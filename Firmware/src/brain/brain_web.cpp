#include "brain_web.h"
#include "brain_config.h"
#include "brain_espnow.h"
#include "gcode.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// 外部变量声明
extern uint32_t lastHandResponse[TOTAL_FEEDERS];
extern FeederStatus feederStatusArray[NUMBER_OF_FEEDER];

AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

// 获取Feeder状态的辅助函数
String getFeederStatusJSON() {
    DynamicJsonDocument doc(2048);
    JsonArray feeders = doc.createNestedArray("feeders");
    
    uint32_t now = millis();
    for (int i = 0; i < NUMBER_OF_FEEDER; i++) {
        JsonObject feeder = feeders.createNestedObject();
        feeder["id"] = i;
        
        // 判断状态：0=离线, 1=在线空闲, 2=忙碌
        if (lastHandResponse[i] > 0 && (now - lastHandResponse[i] < 30000)) {
            if (feederStatusArray[i].waitingForResponse) {
                feeder["status"] = 2; // 忙碌
            } else {
                feeder["status"] = 1; // 在线空闲
            }
            feeder["lastSeen"] = now - lastHandResponse[i];
        } else {
            feeder["status"] = 0; // 离线
            feeder["lastSeen"] = -1;
        }
    }
    
    doc["onlineCount"] = getOnlineHandCount();
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
    }
}

// Web服务器初始化
void web_setup() {
    // 设置WebSocket
    ws.onEvent(onWsEvent);
    webServer.addHandler(&ws);
    
    // API端点：获取所有Feeder状态
    webServer.on("/api/feeders", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getFeederStatusJSON());
    });
    
    // API端点：获取未分配Hand列表
    webServer.on("/api/unassigned", HTTP_GET, [](AsyncWebServerRequest *request){
        String response;
        listUnassignedHands(response);
        
        DynamicJsonDocument doc(1024);
        doc["data"] = response;
        
        String result;
        serializeJson(doc, result);
        request->send(200, "application/json", result);
    });
    
    // 提供静态HTML页面
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Feeder Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 0; padding: 20px; background: #f0f0f0; }
        .header { background: #570DF8; color: white; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        .stats { display: flex; gap: 20px; margin-bottom: 20px; flex-wrap: wrap; }
        .stat { background: white; padding: 15px; border-radius: 5px; flex: 1; min-width: 150px; }
        .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(60px, 1fr)); gap: 5px; margin-bottom: 20px; }
        .feeder { width: 60px; height: 60px; border-radius: 5px; display: flex; align-items: center; justify-content: center; font-weight: bold; color: white; font-size: 12px; }
        .offline { background: #3D4451; color: #ccc; }
        .online { background: #22c55e; }
        .busy { background: #f43f5e; }
        .unassigned { background: #FBBD23; color: #333; }
        .log { background: white; padding: 15px; border-radius: 5px; height: 200px; overflow-y: auto; }
        .log-entry { padding: 5px 0; border-bottom: 1px solid #eee; font-family: monospace; font-size: 12px; }
        .status-text { margin-top: 10px; font-size: 12px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>PNP Feeder Monitor</h1>
        <div id="connectionStatus">连接中...</div>
    </div>
    
    <div class="stats">
        <div class="stat">
            <h3>在线数量</h3>
            <div id="onlineCount">-</div>
        </div>
        <div class="stat">
            <h3>总数量</h3>
            <div>50</div>
        </div>
        <div class="stat">
            <h3>繁忙数量</h3>
            <div id="busyCount">-</div>
        </div>
    </div>
    
    <div class="grid" id="feederGrid"></div>
    
    <div class="status-text">
        <strong>状态说明：</strong>
        <span style="color: #22c55e;">■ 在线</span>
        <span style="color: #f43f5e;">■ 忙碌</span>
        <span style="color: #3D4451;">■ 离线</span>
        <span style="color: #FBBD23;">■ 未分配</span>
    </div>
    
    <div class="log" id="eventLog">
        <div class="log-entry">系统启动...</div>
    </div>

    <script>
        const ws = new WebSocket('ws://' + window.location.host + '/ws');
        let feeders = {};
        
        ws.onopen = function() {
            document.getElementById('connectionStatus').innerText = '已连接';
            addLog('WebSocket连接成功');
        };
        
        ws.onclose = function() {
            document.getElementById('connectionStatus').innerText = '连接断开';
            addLog('WebSocket连接断开');
        };
        
        ws.onmessage = function(event) {
            const data = JSON.parse(event.data);
            if (data.feeders) {
                updateFeeders(data.feeders);
                updateStats(data);
            } else if (data.event) {
                handleEvent(data);
            }
        };
        
        function updateFeeders(feederData) {
            const grid = document.getElementById('feederGrid');
            grid.innerHTML = '';
            
            for (let i = 0; i < 50; i++) {
                const feeder = feederData.find(f => f.id === i) || {id: i, status: 0};
                const div = document.createElement('div');
                div.className = 'feeder';
                div.textContent = i;
                
                switch(feeder.status) {
                    case 0: div.className += ' offline'; break;
                    case 1: div.className += ' online'; break;
                    case 2: div.className += ' busy'; break;
                    default: div.className += ' unassigned';
                }
                
                grid.appendChild(div);
                feeders[i] = feeder;
            }
        }
        
        function updateStats(data) {
            document.getElementById('onlineCount').innerText = data.onlineCount || 0;
            
            const busyCount = data.feeders ? data.feeders.filter(f => f.status === 2).length : 0;
            document.getElementById('busyCount').innerText = busyCount;
        }
        
        function handleEvent(data) {
            const now = new Date().toLocaleTimeString();
            let message = '';
            
            switch(data.event) {
                case 'command_received':
                    message = `[${now}] 收到命令: Feeder ${data.feederId} 送料 ${data.feedLength}mm`;
                    // 更新Feeder状态为忙碌
                    updateSingleFeeder(data.feederId, 2);
                    break;
                case 'command_completed':
                    message = `[${now}] 命令完成: Feeder ${data.feederId} ${data.success ? '成功' : '失败'} - ${data.message}`;
                    // 更新Feeder状态为在线空闲
                    updateSingleFeeder(data.feederId, 1);
                    break;
                case 'hand_online':
                    message = `[${now}] Hand上线: Feeder ${data.feederId}`;
                    // 更新Feeder状态为在线空闲
                    updateSingleFeeder(data.feederId, 1);
                    break;
                case 'hand_offline':
                    message = `[${now}] Hand离线: Feeder ${data.feederId}`;
                    // 更新Feeder状态为离线
                    updateSingleFeeder(data.feederId, 0);
                    break;
            }
            
            if (message) addLog(message);
        }
        
        function updateSingleFeeder(feederId, status) {
            const grid = document.getElementById('feederGrid');
            const feederDiv = grid.children[feederId];
            if (feederDiv) {
                // 移除所有状态类
                feederDiv.className = 'feeder';
                
                // 添加新状态类
                switch(status) {
                    case 0: feederDiv.className += ' offline'; break;
                    case 1: feederDiv.className += ' online'; break;
                    case 2: feederDiv.className += ' busy'; break;
                    default: feederDiv.className += ' unassigned';
                }
                
                // 更新本地状态数据
                feeders[feederId] = {id: feederId, status: status};
                
                // 更新统计信息
                updateStatsFromFeeders();
            }
        }
        
        function updateStatsFromFeeders() {
            let onlineCount = 0;
            let busyCount = 0;
            
            for (let i = 0; i < 50; i++) {
                const feeder = feeders[i];
                if (feeder && feeder.status > 0) {
                    onlineCount++;
                    if (feeder.status === 2) {
                        busyCount++;
                    }
                }
            }
            
            document.getElementById('onlineCount').innerText = onlineCount;
            document.getElementById('busyCount').innerText = busyCount;
        }
        
        function addLog(message) {
            const log = document.getElementById('eventLog');
            const entry = document.createElement('div');
            entry.className = 'log-entry';
            entry.textContent = message;
            log.appendChild(entry);
            log.scrollTop = log.scrollHeight;
            
            // 限制日志条数
            while (log.children.length > 100) {
                log.removeChild(log.firstChild);
            }
        }
        
        // 定期刷新状态
        setInterval(() => {
            if (ws.readyState === WebSocket.OPEN) {
                fetch('/api/feeders')
                    .then(response => response.json())
                    .then(data => {
                        updateFeeders(data.feeders);
                        updateStats(data);
                    });
            }
        }, 5000);
    </script>
</body>
</html>
        )");
    });
    
    webServer.begin();
    Serial.println("Web server started on port 80");
    Serial.printf("Visit: http://%s\n", WiFi.localIP().toString().c_str());
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
