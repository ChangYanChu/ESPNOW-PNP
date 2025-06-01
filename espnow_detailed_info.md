# ESP-NOW通信详细信息

## 1. ESP-NOW简介
ESP-NOW 是一种由 Espressif 提供的无线通信协议，允许多个设备之间进行低延迟的数据传输。它适用于需要快速、可靠通信的场景，例如物联网设备之间的控制和状态反馈。

---

## 2. ESP-NOW通信逻辑
### **Brain模块**
- **通信初始化**:
  - 设置 WiFi 模式为 `WIFI_STA`，确保设备处于站点模式。
  - 调用 `esp_now_init()` 初始化 ESP-NOW。
  - 注册回调函数：
    - `esp_now_register_send_cb()`：用于监控数据发送后的状态。
    - `esp_now_register_recv_cb()`：用于处理接收到的数据。

- **数据发送**:
  - 使用 `esp_now_send()` 向 Hand 模块发送数据包。
  - 数据包通常包含以下信息：
    - 舵机目标角度。
    - 喂料器状态。
    - 操作指令（例如推进或回缩）。

- **数据接收**:
  - 在接收回调中解析 Hand 模块返回的数据包。
  - 数据包通常包含以下信息：
    - 当前舵机角度。
    - 操作完成状态。
    - 错误代码（如果有）。

### **Hand模块**
- **通信初始化**:
  - 设置 WiFi 模式为 `WIFI_STA`，确保设备处于站点模式。
  - 调用 `esp_now_init()` 初始化 ESP-NOW。
  - 注册回调函数：
    - `esp_now_register_send_cb()`：用于监控数据发送后的状态。
    - `esp_now_register_recv_cb()`：用于处理接收到的数据。

- **数据接收**:
  - 在接收回调中解析 Brain 模块发送的控制指令。
  - 根据指令执行以下操作：
    - 更新舵机角度。
    - 执行喂料器推进或回缩操作。

- **数据发送**:
  - 使用 `esp_now_send()` 向 Brain 模块发送状态反馈。
  - 数据包通常包含以下信息：
    - 当前舵机角度。
    - 操作完成状态。
    - 错误代码（如果有）。

---

## 3. 数据包结构
### **Brain发送的数据包**
```cpp
struct BrainToHandPacket {
    uint8_t command;      // 控制指令，例如推进或回缩
    uint16_t targetAngle; // 舵机目标角度
    uint8_t feederState;  // 喂料器状态
    uint8_t reserved[5];  // 保留字段，用于扩展
};
```

### **Hand发送的数据包**
```cpp
struct HandToBrainPacket {
    uint16_t currentAngle; // 当前舵机角度
    uint8_t operationStatus; // 操作完成状态
    uint8_t errorCode;       // 错误代码（如果有）
    uint8_t reserved[5];     // 保留字段，用于扩展
};
```

---

## 4. 内部指令列表
### **Brain发送的指令**
- **COMMAND_ADVANCE** (0x01): 推进喂料器。
- **COMMAND_RETRACT** (0x02): 回缩喂料器。
- **COMMAND_SET_ANGLE** (0x03): 设置舵机角度。
- **COMMAND_ENABLE_FEEDER** (0x04): 启用喂料器。
- **COMMAND_DISABLE_FEEDER** (0x05): 禁用喂料器。

### **Hand返回的状态**
- **STATUS_SUCCESS** (0x00): 操作成功。
- **STATUS_ERROR** (0x01): 操作失败。
- **STATUS_BUSY** (0x02): 设备忙碌中。
- **STATUS_INVALID_COMMAND** (0x03): 无效指令。

---

## 5. 回调函数示例
### **发送回调**
```cpp
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("数据发送成功");
    } else {
        Serial.println("数据发送失败");
    }
}
```

### **接收回调**
```cpp
void onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    Serial.println("接收到数据");
    // 解析数据包
    HandToBrainPacket packet;
    memcpy(&packet, data, sizeof(packet));
    Serial.printf("当前角度: %d\n", packet.currentAngle);
}
```

---

## 6. 数据包解析示例
### **Brain发送数据包**
```cpp
BrainToHandPacket packet;
packet.command = COMMAND_ADVANCE;
packet.targetAngle = 90;
packet.feederState = 1;
memset(packet.reserved, 0, sizeof(packet.reserved));

esp_now_send(peerMacAddress, (uint8_t *)&packet, sizeof(packet));
```

### **Hand接收数据包**
```cpp
void onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    HandToBrainPacket packet;
    memcpy(&packet, data, sizeof(packet));

    if (packet.operationStatus == STATUS_SUCCESS) {
        Serial.println("操作成功");
    } else if (packet.operationStatus == STATUS_ERROR) {
        Serial.println("操作失败");
    }
}
```

---

## 7. 注意事项
- **数据包大小限制**:
  ESP-NOW的数据包最大长度为250字节，需确保数据包不超过此限制。
- **指令扩展**:
  保留字段 `reserved` 可用于未来扩展指令或状态。
- **错误处理**:
  在接收数据时需验证数据包长度和内容的合法性。

---

## 总结
通过设计统一的数据包结构和指令集，Brain 和 Hand 模块可以高效地进行通信。保留字段为未来扩展提供了灵活性，同时明确的指令和状态定义确保了通信的可靠性。