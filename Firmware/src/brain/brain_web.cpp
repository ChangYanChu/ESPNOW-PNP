#include "brain_web.h"
#include "brain_config.h"
#include "brain_espnow.h"
#include "gcode.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// External variable declarations
extern uint32_t lastHandResponse[TOTAL_FEEDERS];
extern FeederStatus feederStatusArray[NUMBER_OF_FEEDER];
extern uint32_t totalSessionFeeds;
extern uint32_t totalWorkCount;

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
    
    // Debug output
    Serial.printf("Web API - Online: %d, SessionFeeds: %lu, WorkCount: %lu\n", 
                  getOnlineHandCount(), totalSessionFeeds, totalWorkCount);
    
    String result;
    serializeJson(doc, result);
    Serial.printf("JSON Response Length: %d\n", result.length());
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
        
        request->send(200, "application/json", "{\"success\":true}");
    });
    
    // 提供静态HTML页面
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", R"rawhtml(
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
        .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(120px, 1fr)); gap: 8px; margin-bottom: 20px; }
        .feeder { width: 120px; height: 80px; border-radius: 8px; display: flex; flex-direction: column; align-items: center; justify-content: center; font-weight: bold; color: white; font-size: 10px; cursor: pointer; padding: 5px; box-sizing: border-box; }
        .feeder-id { font-size: 14px; font-weight: bold; margin-bottom: 2px; }
        .feeder-name { font-size: 9px; opacity: 0.9; margin-bottom: 2px; max-width: 100%; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
        .feeder-package { font-size: 8px; opacity: 0.8; margin-bottom: 2px; }
        .feeder-parts { font-size: 9px; opacity: 0.9; margin-bottom: 1px; }
        .feeder-session { font-size: 8px; opacity: 0.8; }
        .offline { background: #3D4451; color: #ccc; }
        .online { background: #22c55e; }
        .busy { background: #f43f5e; }
        .unassigned { background: #FBBD23; color: #333; }
        .log { background: white; padding: 15px; border-radius: 5px; height: 200px; overflow-y: auto; }
        .log-entry { padding: 5px 0; border-bottom: 1px solid #eee; font-family: monospace; font-size: 12px; }
        .status-text { margin-top: 10px; font-size: 12px; }
        
        /* 模态框样式 */
        .modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.5); }
        .modal-content { background-color: white; margin: 10% auto; padding: 20px; border-radius: 10px; width: 90%; max-width: 500px; }
        .modal-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
        .modal-title { font-size: 18px; font-weight: bold; color: #570DF8; }
        .close { font-size: 28px; font-weight: bold; cursor: pointer; color: #aaa; }
        .close:hover { color: #000; }
        .form-group { margin-bottom: 15px; }
        .form-group label { display: block; margin-bottom: 5px; font-weight: bold; }
        .form-group input { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        .form-buttons { display: flex; gap: 10px; justify-content: flex-end; margin-top: 20px; }
        .btn { padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-weight: bold; }
        .btn-primary { background: #570DF8; color: white; }
        .btn-secondary { background: #6c757d; color: white; }
    </style>
</head>
<body>
    <div class="header">
        <h1>PNP Feeder Monitor</h1>
        <div id="connectionStatus">Connecting...</div>
    </div>
    
    <div class="stats">
        <div class="stat">
            <h3>Online Count</h3>
            <div id="onlineCount">-</div>
        </div>
        <div class="stat">
            <h3>Total Count</h3>
            <div>50</div>
        </div>
        <div class="stat">
            <h3>Work Count</h3>
            <div id="workCount">-</div>
        </div>
        <div class="stat">
            <h3>Session Feeds</h3>
            <div id="sessionFeeds">-</div>
        </div>
    </div>
    
    <div class="grid" id="feederGrid"></div>
    
    <div class="status-text">
        <strong>Status Legend:</strong>
        <span style="color: #22c55e;">■ Online</span>
        <span style="color: #f43f5e;">■ Busy</span>
        <span style="color: #3D4451;">■ Offline</span>
        <span style="color: #FBBD23;">■ Unassigned</span>
    </div>
    
    <div class="log" id="eventLog">
        <div class="log-entry">System starting...</div>
    </div>

    <!-- Configuration Modal -->
    <div id="configModal" class="modal">
        <div class="modal-content">
            <div class="modal-header">
                <span class="modal-title" id="modalTitle">Configure Feeder</span>
                <span class="close" onclick="closeModal()">X</span>
            </div>
            <form id="configForm">
                <input type="hidden" id="feederId" name="feederId">
                
                <div class="form-group">
                    <label for="componentName">Component Name:</label>
                    <input type="text" id="componentName" name="componentName" maxlength="15">
                </div>
                
                <div class="form-group">
                    <label for="packageType">Package Type:</label>
                    <input type="text" id="packageType" name="packageType" maxlength="7">
                </div>
                
                <div class="form-group">
                    <label for="totalPartCount">Total Count:</label>
                    <input type="number" id="totalPartCount" name="totalPartCount" min="0" max="65535">
                </div>
                
                <div class="form-group">
                    <label for="remainingPartCount">Remaining Count:</label>
                    <input type="number" id="remainingPartCount" name="remainingPartCount" min="0" max="65535">
                </div>
                
                <div class="form-group">
                    <label>Statistics:</label>
                    <div id="statsInfo" style="background: #f8f9fa; padding: 10px; border-radius: 4px; font-size: 12px;"></div>
                </div>
                
                <div class="form-buttons">
                    <button type="button" class="btn btn-secondary" onclick="closeModal()">Cancel</button>
                    <button type="button" class="btn btn-primary" onclick="saveConfig()">Save</button>
                </div>
            </form>
        </div>
    </div>

    <script>
        const ws = new WebSocket('ws://' + window.location.host + '/ws');
        let feeders = {};
        
        ws.onopen = function() {
            document.getElementById('connectionStatus').innerText = 'Connected';
            addLog('WebSocket connected successfully');
        };
        
        ws.onclose = function() {
            document.getElementById('connectionStatus').innerText = 'Disconnected';
            addLog('WebSocket disconnected');
        };
        
        ws.onmessage = function(event) {
            const data = JSON.parse(event.data);
            console.log('WebSocket received:', data);
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
                const feeder = feederData.find(f => f.id === i) || {
                    id: i, status: 0, remainingPartCount: 0, totalPartCount: 0, 
                    sessionFeedCount: 0, componentName: 'N' + i, packageType: 'Unknown',
                    totalFeedCount: 0
                };
                
                const div = document.createElement('div');
                div.className = 'feeder';
                div.onclick = () => showFeederConfig(feeder);
                
                // Create display content
                const idDiv = document.createElement('div');
                idDiv.className = 'feeder-id';
                idDiv.textContent = i;
                
                const nameDiv = document.createElement('div');
                nameDiv.className = 'feeder-name';
                nameDiv.textContent = feeder.componentName || 'N' + i;
                nameDiv.title = feeder.componentName || 'N' + i; // Hover to show full name
                
                const packageDiv = document.createElement('div');
                packageDiv.className = 'feeder-package';
                packageDiv.textContent = feeder.packageType || 'Unknown';
                
                const partsDiv = document.createElement('div');
                partsDiv.className = 'feeder-parts';
                partsDiv.textContent = (feeder.remainingPartCount || 0) + '/' + (feeder.totalPartCount || 0);
                
                const sessionDiv = document.createElement('div');
                sessionDiv.className = 'feeder-session';
                sessionDiv.textContent = 'This: ' + (feeder.sessionFeedCount || 0);
                
                div.appendChild(idDiv);
                div.appendChild(nameDiv);
                div.appendChild(packageDiv);
                div.appendChild(partsDiv);
                div.appendChild(sessionDiv);
                
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
            console.log('Updating stats with:', data);
            document.getElementById('onlineCount').innerText = data.onlineCount || 0;
            document.getElementById('workCount').innerText = data.totalWorkCount || 0;
            document.getElementById('sessionFeeds').innerText = data.totalSessionFeeds || 0;
        }
        
        function handleEvent(data) {
            const now = new Date().toLocaleTimeString();
            let message = '';
            
            switch(data.event) {
                case 'command_received':
                    message = '[' + now + '] Command received: Feeder ' + data.feederId + ' feed ' + data.feedLength + 'mm';
                    // Update Feeder status to busy
                    updateSingleFeeder(data.feederId, 2);
                    break;
                case 'command_completed':
                    message = '[' + now + '] Command completed: Feeder ' + data.feederId + ' ' + (data.success ? 'Success' : 'Failed') + ' - ' + data.message;
                    // Update Feeder status to online idle
                    updateSingleFeeder(data.feederId, 1);
                    break;
                case 'hand_online':
                    message = '[' + now + '] Hand online: Feeder ' + data.feederId;
                    // Update Feeder status to online idle
                    updateSingleFeeder(data.feederId, 1);
                    break;
                case 'hand_offline':
                    message = '[' + now + '] Hand offline: Feeder ' + data.feederId;
                    // Update Feeder status to offline
                    updateSingleFeeder(data.feederId, 0);
                    break;
            }
            
            if (message) addLog(message);
        }
        
        function updateSingleFeeder(feederId, status) {
            const grid = document.getElementById('feederGrid');
            const feederDiv = grid.children[feederId];
            if (feederDiv) {
                // Remove all status classes
                feederDiv.className = 'feeder';
                
                // Add new status class
                switch(status) {
                    case 0: feederDiv.className += ' offline'; break;
                    case 1: feederDiv.className += ' online'; break;
                    case 2: feederDiv.className += ' busy'; break;
                    default: feederDiv.className += ' unassigned';
                }
                
                // Update local status data
                feeders[feederId] = {id: feederId, status: status};
                
                // Update statistics
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
        }
        
        function showFeederConfig(feeder) {
            document.getElementById('feederId').value = feeder.id;
            document.getElementById('componentName').value = feeder.componentName || 'N' + feeder.id;
            document.getElementById('packageType').value = feeder.packageType || 'Unknown';
            document.getElementById('totalPartCount').value = feeder.totalPartCount || 0;
            document.getElementById('remainingPartCount').value = feeder.remainingPartCount || 0;
            
            document.getElementById('modalTitle').textContent = 'Configure Feeder ' + feeder.id;
            
            // Display statistics
            const statsInfo = 
                'Total feeds: ' + (feeder.totalFeedCount || 0) + '<br>' +
                'Session feeds: ' + (feeder.sessionFeedCount || 0) + '<br>' +
                'Status: ' + getStatusText(feeder.status);
            document.getElementById('statsInfo').innerHTML = statsInfo;
            
            document.getElementById('configModal').style.display = 'block';
        }
        
        function closeModal() {
            document.getElementById('configModal').style.display = 'none';
        }
        
        function getStatusText(status) {
            switch(status) {
                case 0: return 'Offline';
                case 1: return 'Online Idle';
                case 2: return 'Busy';
                default: return 'Unassigned';
            }
        }
        
        function saveConfig() {
            const data = {
                id: parseInt(document.getElementById('feederId').value),
                componentName: document.getElementById('componentName').value.trim(),
                packageType: document.getElementById('packageType').value.trim(),
                totalPartCount: parseInt(document.getElementById('totalPartCount').value) || 0,
                remainingPartCount: parseInt(document.getElementById('remainingPartCount').value) || 0
            };
            
            fetch('/api/feeder/config', {
                method: 'PUT',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(data)
            }).then(response => {
                if (response.ok) {
                    addLog('Feeder ' + data.id + ' config updated');
                    closeModal();
                    // Refresh display
                    setTimeout(() => {
                        fetch('/api/feeders')
                            .then(response => response.json())
                            .then(data => {
                                updateFeeders(data.feeders);
                                updateStats(data);
                            });
                    }, 500);
                } else {
                    addLog('Feeder ' + data.id + ' config update failed');
                    alert('Configuration update failed, please try again');
                }
            }).catch(error => {
                console.error('Error:', error);
                addLog('Feeder ' + data.id + ' config update error');
                alert('Network error, please try again');
            });
        }
        
        // Click outside modal to close
        window.onclick = function(event) {
            const modal = document.getElementById('configModal');
            if (event.target === modal) {
                closeModal();
            }
        }
        
        function addLog(message) {
            const log = document.getElementById('eventLog');
            const entry = document.createElement('div');
            entry.className = 'log-entry';
            entry.textContent = message;
            log.appendChild(entry);
            log.scrollTop = log.scrollHeight;
            
            // Limit log entries
            while (log.children.length > 100) {
                log.removeChild(log.firstChild);
            }
        }
        
        // Periodic status refresh
        setInterval(() => {
            if (ws.readyState === WebSocket.OPEN) {
                fetch('/api/feeders')
                    .then(response => response.json())
                    .then(data => {
                        console.log('Fetch API response:', data);
                        updateFeeders(data.feeders);
                        updateStats(data);
                    })
                    .catch(error => {
                        console.error('Fetch error:', error);
                    });
            }
        }, 5000);
    </script>
</body>
</html>
        )rawhtml");
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
