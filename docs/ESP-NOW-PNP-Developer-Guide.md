# ESP-NOW PNP 系统开发者指南

## 📖 概述

本文档面向开发人员，提供ESP-NOW PNP系统的完整技术实现细节、项目结构分析、反馈系统集成方案以及扩展开发指南。

---

## 📁 项目结构详解

### 目录结构
```
ESPNOW-PNP/
├── 📄 README.md                              # 项目主文档
├── 📄 CHANGELOG.md                           # 版本变更记录
├── 📄 platformio.ini                         # PlatformIO构建配置
├── 📄 partitions.csv                         # ESP32分区表
├── 
├── 📂 src/                                   # 主源代码目录
│   ├── 📂 brain/                            # Brain控制器 (ESP32C3)
│   │   ├── 🔧 brain_main.cpp               # Brain主程序入口
│   │   ├── 🔧 brain_config.h               # Brain配置文件
│   │   ├── 🔧 brain_gcode.h/.cpp           # G-code命令处理器
│   │   ├── 🔧 brain_espnow.h/.cpp          # ESP-NOW通信管理
│   │   └── 🔧 brain_feeder_manager.h/.cpp  # 喂料器管理器
│   │
│   ├── 📂 hand/                             # Hand控制器 (ESP01S)
│   │   ├── 🔧 hand_main.cpp                # Hand主程序入口
│   │   ├── 🔧 hand_config.h                # Hand配置文件
│   │   ├── 🔧 hand_servo.h/.cpp            # 舵机控制模块
│   │   ├── 🔧 hand_espnow.h/.cpp           # ESP-NOW通信处理
│   │   └── 🔧 hand_feedback.h/.cpp         # 反馈检测模块
│   │
│   └── 📂 common/                           # 共享代码
│       ├── 🔧 espnow_protocol.h            # ESP-NOW协议定义
│       └── 🔧 debug_utils.h                # 调试工具
│
├── 📂 docs/                                 # 完整技术文档
├── 📂 test/                                 # 测试脚本和工具
├── 📂 demo/                                 # 参考实现和示例
├── 📂 .vscode/                             # VS Code配置
├── 📂 .pio/                                # PlatformIO构建输出
├── 📂 include/                             # 头文件目录
└── 📂 lib/                                 # 第三方库
```

---

## 🧠 Brain控制器模块详解

### brain_main.cpp - 主程序入口
```cpp
功能:
├── 系统初始化和配置
├── 串口通信管理
├── 主循环调度
└── 帮助信息显示

关键函数:
├── setup() - 系统初始化
├── loop() - 主循环
└── printHelp() - 帮助信息
```

### brain_config.h - 系统配置
```cpp
配置项:
├── #define TOTAL_FEEDERS 50        # 最大喂料器数量
├── #define HEARTBEAT_INTERVAL 5000 # 心跳间隔
├── 调试开关配置
├── ESP-NOW通信参数
└── G-code命令常量定义

M-code定义:
├── MCODE_ENABLE_SYSTEM 610     # M610 - 系统启用/禁用
├── MCODE_FEEDER_ADVANCE 600    # M600 - 喂料器推进
├── MCODE_FEEDER_RETRACT 601    # M601 - 喂料器回缩
├── MCODE_FEEDER_STATUS 602     # M602 - 喂料器状态查询
├── MCODE_SERVO_ANGLE 280       # M280 - 舵机角度控制
├── MCODE_FEEDER_CONFIG 603     # M603 - 喂料器配置更新
├── MCODE_ALL_STATUS 620        # M620 - 所有手部状态查询
├── MCODE_CHECK_FEEDBACK 604    # M604 - 反馈状态检查
├── MCODE_ENABLE_FEEDBACK 605   # M605 - 反馈功能启用/禁用
├── MCODE_CLEAR_MANUAL_FEED 606 # M606 - 手动进料标志清除
└── MCODE_PROCESS_MANUAL_FEED 607 # M607 - 手动进料处理
```

### brain_gcode.h/.cpp - G-code处理器
```cpp
类: BrainGCode
├── 串口输入缓冲和解析
├── G-code命令分发
├── 参数提取和验证
└── 响应格式化输出

主要方法:
├── update() - 处理串口输入
├── processCommand() - 解析和执行命令
├── parseParameter() - 提取参数
├── validateFeederNumber() - 验证喂料器编号
├── processM610() - 系统启用/禁用
├── processM600() - 喂料器推进
├── processM601() - 喂料器回缩
├── processM602() - 状态查询
├── processM280() - 舵机角度控制
├── processM603() - 配置更新
├── processM620() - 所有状态查询
├── processM604() - 反馈状态检查
├── processM605() - 反馈启用/禁用
├── processM606() - 清除手动进料标志
└── processM607() - 处理手动进料
```

### brain_espnow.h/.cpp - ESP-NOW通信管理
```cpp
类: BrainESPNow
├── ESP-NOW协议初始化
├── 设备发现和注册管理
├── 命令发送和响应处理
└── 连接状态维护

数据结构:
├── struct ESPNowPacket - 通信数据包格式
├── struct HandInfo - Hand设备信息
└── 命令类型枚举

主要方法:
├── begin() - 初始化ESP-NOW
├── update() - 处理接收数据
├── startDiscovery() - 开始设备发现
├── registerHand() - 注册Hand设备
├── sendFeederAdvance() - 发送推进命令
├── sendFeederRetract() - 发送回缩命令
├── sendServoAngle() - 发送舵机角度
├── sendConfigUpdate() - 发送配置更新
├── requestAllStatus() - 请求所有状态
└── isHandOnline() - 检查Hand在线状态
```

### brain_feeder_manager.h/.cpp - 喂料器管理器
```cpp
类: FeederManager
├── 喂料器状态跟踪
├── 系统启用状态管理
├── 心跳监控
└── 反馈状态管理

数据结构:
├── struct FeederInfo - 喂料器信息
└── 系统状态变量

主要方法:
├── begin() - 初始化管理器
├── update() - 状态更新和心跳检查
├── enableSystem() - 启用/禁用系统
├── advanceFeeder() - 推进喂料器
├── retractFeeder() - 回缩喂料器
├── setServoAngle() - 设置舵机角度
├── requestFeederStatus() - 请求状态
├── isFeederOnline() - 检查在线状态
└── getFeederStatus() - 获取状态字符串
```

---

## 🤚 Hand控制器模块详解

### hand_main.cpp - 主程序入口
```cpp
功能:
├── 硬件初始化(舵机、反馈、ESP-NOW)
├── 主循环管理
├── 模块协调
└── 状态同步

关键函数:
├── setup() - 系统初始化
└── loop() - 主循环(舵机更新、反馈检测、通信处理)
```

### hand_config.h - Hand配置
```cpp
配置项:
├── #define SERVO_PIN 2           # 舵机控制引脚(GPIO2)
├── #define FEEDBACK_PIN 0        # 反馈输入引脚(GPIO0)
├── #define HAS_FEEDBACK_PIN      # 启用反馈功能
├── 舵机参数(角度、脉宽、稳定时间)
├── 反馈检测参数(防抖时间、检测窗口)
└── 调试输出开关
```

### hand_servo.h/.cpp - 舵机控制模块
```cpp
类: HandServoController
├── PWM信号生成
├── 异步角度控制
├── 稳定时间管理
└── 状态跟踪

主要方法:
├── begin() - 初始化舵机
├── update() - 更新PWM输出
├── requestSetAngle() - 请求设置角度
├── isReady() - 检查是否就绪
├── getCurrentAngle() - 获取当前角度
└── setConfigParameters() - 更新配置参数
```

### hand_espnow.h/.cpp - ESP-NOW通信处理
```cpp
类: HandESPNow
├── ESP-NOW初始化和回调
├── 命令接收和处理
├── 状态响应发送
└── Brain设备管理

主要方法:
├── begin() - 初始化通信
├── update() - 处理接收队列
├── sendDiscoveryResponse() - 发送发现响应
├── processFeederAdvance() - 处理推进命令
├── processFeederRetract() - 处理回缩命令
├── processServoAngle() - 处理舵机角度命令
├── processStatusRequest() - 处理状态请求
├── processConfigUpdate() - 处理配置更新
└── sendResponse() - 发送响应
```

### hand_feedback.h/.cpp - 反馈检测模块
```cpp
类: HandFeedbackManager
├── GPIO状态监测
├── 防抖算法
├── 手动进料检测
└── 状态变化通知

数据结构:
├── struct FeederFeedbackStatus - 反馈状态
└── enum FeederFeedbackState - 状态枚举

主要方法:
├── begin() - 初始化反馈检测
├── update() - 更新状态检测
├── isTapeLoaded() - 检查胶带装载状态
├── isManualFeedRequested() - 检查手动进料请求
├── clearManualFeedFlag() - 清除手动进料标志
├── getStatus() - 获取完整状态
└── enableFeedback() - 启用/禁用反馈
```

---

## 📡 反馈系统架构详解

### 系统设计理念

ESP-NOW PNP反馈系统采用分布式架构，将原始0816 feeder系统的反馈机制集成到新的ESP-NOW通信框架中。

#### 原始反馈系统分析
**硬件组件:**
- 微动开关（NO - 常开型）
- 反馈引脚（INPUT_PULLUP配置）
- 胶带张紧机构

**工作原理:**
- 胶带正确装载且张力适当 → 微动开关闭合 → 反馈引脚 LOW（正常）
- 胶带未装载或张力不足 → 微动开关断开 → 反馈引脚 HIGH（错误）

#### 新架构集成策略
```
Brain Controller (ESP32C3)
├── 喂料器管理器 (FeederManager)
│   ├── 反馈状态缓存
│   ├── 手动进料标志管理
│   └── G-code命令处理
│
└── ESP-NOW通信层
    ├── 反馈状态查询命令
    ├── 反馈启用/禁用命令
    └── 手动进料处理命令

Hand Controller (ESP01S)  
├── 反馈管理器 (HandFeedbackManager)
│   ├── GPIO状态监测 (GPIO0)
│   ├── 防抖算法实现
│   ├── 手动进料检测 (5-50ms窗口)
│   └── 本地手动进料执行
│
├── 舵机控制器 (HandServoController)
│   ├── 手动进料舵机动作
│   └── 角度控制和状态管理
│
└── ESP-NOW通信层
    ├── 反馈状态响应
    └── 状态更新通知
```

### 核心功能实现

#### 1. 胶带装载检测
```cpp
// Hand端实现
bool HandFeedbackManager::isTapeLoaded() {
    if (!feedbackEnabled) return true; // 反馈禁用时始终返回true
    
    int currentState = digitalRead(FEEDBACK_PIN);
    
    // 防抖处理
    if (currentState != lastPinState) {
        debounceTimer = millis();
        lastPinState = currentState;
    }
    
    if (millis() - debounceTimer < DEBOUNCE_DELAY) {
        return lastStableState == LOW; // 返回上次稳定状态
    }
    
    lastStableState = currentState;
    return currentState == LOW; // LOW表示胶带装载
}
```

#### 2. 手动进料检测（关键改进）
```cpp
// Hand端本地处理实现
void HandFeedbackManager::checkManualFeed() {
    if (!feedbackEnabled) return;
    
    int currentState = digitalRead(FEEDBACK_PIN);
    
    // 独立的手动进料状态跟踪
    if (currentState != lastManualFeedPinState) {
        if (currentState == HIGH && lastManualFeedPinState == LOW) {
            // 检测到按压开始
            manualFeedPressStartTime = millis();
        } else if (currentState == LOW && lastManualFeedPinState == HIGH) {
            // 检测到按压结束
            unsigned long pressDuration = millis() - manualFeedPressStartTime;
            
            // 检查按压时长是否在有效窗口内 (5-50ms)
            if (pressDuration >= MANUAL_FEED_MIN_DURATION && 
                pressDuration <= MANUAL_FEED_MAX_DURATION) {
                manualFeedRequested = true;
                executeManualFeed(); // 立即本地执行
            }
        }
        lastManualFeedPinState = currentState;
    }
}

void HandFeedbackManager::executeManualFeed() {
    DEBUG_FEEDBACK_PRINT("Executing local manual feed");
    
    // 获取当前配置的推进角度
    uint16_t targetAngle = currentAdvanceAngle > 0 ? currentAdvanceAngle : 120;
    
    // 设置舵机角度
    servoController->requestSetAngle(targetAngle);
    
    // 等待舵机到位，同时更新舵机状态
    unsigned long startTime = millis();
    while (!servoController->isReady() && (millis() - startTime < 3000)) {
        servoController->update(); // 关键：更新舵机状态
        delay(10);
    }
    
    if (servoController->isReady()) {
        // 保持角度一段时间
        delay(100);
        
        // 回到中性位置
        servoController->requestSetAngle(90);
        
        // 等待回到中性位置
        startTime = millis();
        while (!servoController->isReady() && (millis() - startTime < 3000)) {
            servoController->update(); // 关键：更新舵机状态
            delay(10);
        }
        
        DEBUG_FEEDBACK_PRINT("Manual feed completed successfully");
    } else {
        DEBUG_FEEDBACK_PRINT("Manual feed failed - servo timeout");
    }
}
```

#### 3. Brain端状态管理简化
```cpp
// Brain端M607命令实现（已简化）
void BrainGCode::processM607() {
    int feederNumber = parseParameter('N');
    
    if (!validateFeederNumber(feederNumber)) {
        Serial.println("error: Invalid feeder number");
        return;
    }
    
    // 简化为仅清除标志，实际处理在Hand端完成
    feederManager->processManualFeed(feederNumber);
    Serial.printf("ok Manual feed flag cleared for feeder %d\n", feederNumber);
}

void FeederManager::processManualFeed(int feederId) {
    // 简化实现：仅清除Brain端的手动进料标志
    if (feederId >= 0 && feederId < TOTAL_FEEDERS) {
        feeders[feederId].manualFeedRequested = false;
        DEBUG_FEEDER_PRINT("Manual feed flag cleared for feeder " + String(feederId));
    }
}
```

### G-code命令扩展详解

#### M604 - 反馈状态检查
**功能**: 查询指定喂料器的完整反馈状态
**语法**: `M604 N<feeder_id>`
**实现流程**:
1. Brain验证参数并检查Hand在线状态
2. 通过ESP-NOW发送状态查询命令
3. Hand响应当前反馈状态
4. Brain格式化输出结果

**响应格式**:
```
Feeder N: Tape loaded: YES/NO, Manual feed requested: YES/NO, Feedback enabled: YES/NO
```

#### M605 - 反馈功能启用/禁用
**功能**: 控制指定喂料器的反馈功能开关
**语法**: `M605 N<feeder_id> S<0/1>`
**实现流程**:
1. Brain验证参数有效性
2. 发送启用/禁用命令到对应Hand
3. Hand更新本地反馈状态
4. 返回确认响应

#### M606 - 手动进料标志清除
**功能**: 清除指定喂料器的手动进料请求标志
**语法**: `M606 N<feeder_id>`
**用途**: 用于复位手动进料状态，通常在维护时使用

#### M607 - 手动进料处理（已简化）
**功能**: 处理手动进料请求（现在主要用于状态清理）
**语法**: `M607 N<feeder_id>`
**当前实现**: 由于手动进料已改为Hand端本地处理，此命令现在主要用于清除Brain端的手动进料标志

---

## 🔧 构建和配置

### platformio.ini配置
```ini
[env:esp32c3-brain]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
build_flags = 
    -DBRAIN_CONTROLLER
    -DDEBUG_ENABLED
monitor_speed = 115200

[env:esp01s-hand]
platform = espressif8266
board = esp01
framework = arduino
build_flags = 
    -DHAND_CONTROLLER
    -DDEBUG_ENABLED
monitor_speed = 115200
```

### 编译和部署
```bash
# Brain控制器 (ESP32C3)
pio run -e esp32c3-brain -t upload

# Hand控制器 (ESP01S)  
pio run -e esp01s-hand -t upload

# 清理构建缓存
pio run -t clean

# 串口监控
pio device monitor --baud 115200
```

### 调试配置
```cpp
// 调试开关配置 (在各自的config.h中)
#define DEBUG_ENABLED              // 总调试开关
#define DEBUG_ESPNOW              // ESP-NOW通信调试
#define DEBUG_GCODE               // G-code处理调试
#define DEBUG_FEEDER              // 喂料器管理调试
#define DEBUG_HAND_FEEDBACK       // Hand反馈系统调试
#define DEBUG_HAND_SERVO          // Hand舵机控制调试
```

---

## 📊 开发指南

### 添加新G-code命令
```cpp
// 1. 在brain_config.h中定义M-code常量
#define MCODE_NEW_COMMAND 608

// 2. 在brain_gcode.cpp中添加处理函数
void BrainGCode::processM608() {
    // 解析参数
    int param = parseParameter('P');
    
    // 验证参数
    if (param < 0 || param > 100) {
        Serial.println("error: Invalid parameter");
        return;
    }
    
    // 执行命令逻辑
    feederManager->executeNewCommand(param);
    
    // 返回响应
    Serial.printf("ok New command executed with param %d\n", param);
}

// 3. 在processCommand()中添加case分支
case MCODE_NEW_COMMAND:
    processM608();
    break;

// 4. 更新帮助信息和文档
```

### 扩展Hand功能模块
```cpp
// 1. 创建新的模块类
class HandNewModule {
private:
    bool moduleEnabled;
    unsigned long lastUpdate;
    
public:
    void begin();
    void update();
    void processCommand(uint8_t command, const uint8_t* data);
    // 其他方法...
};

// 2. 在hand_main.cpp中集成
HandNewModule newModule;

void setup() {
    // 现有初始化...
    newModule.begin();
}

void loop() {
    // 现有更新...
    newModule.update();
}

// 3. 添加ESP-NOW命令处理
// 在hand_espnow.cpp中添加相应处理函数
void HandESPNow::processNewCommand(const uint8_t* data) {
    newModule.processCommand(data[0], &data[1]);
}
```

### 扩展反馈系统功能
```cpp
// 1. 在HandFeedbackManager中添加新检测
class HandFeedbackManager {
private:
    bool newSensorEnabled;
    int newSensorPin;
    
public:
    bool checkNewSensor();
    void enableNewSensor(bool enable);
    
    // 扩展状态结构
    struct ExtendedFeedbackStatus : public FeederFeedbackStatus {
        bool newSensorStatus;
        uint32_t newSensorCount;
    };
};

// 2. 添加对应的G-code命令
// M608 - 新传感器控制
void BrainGCode::processM608() {
    int feederId = parseParameter('N');
    int enable = parseParameter('S');
    
    // 发送命令到Hand
    espnowManager->sendNewSensorCommand(feederId, enable);
    Serial.printf("ok New sensor %s for feeder %d\n", 
                  enable ? "enabled" : "disabled", feederId);
}
```

### 调试最佳实践
```cpp
// 1. 使用条件编译的调试输出
#ifdef DEBUG_ENABLED
    DEBUG_PRINT("Debug message");
#endif

// 2. 分模块的调试开关
#ifdef DEBUG_HAND_FEEDBACK
    DEBUG_FEEDBACK_PRINT("Feedback status changed");
#endif

// 3. 详细的状态信息
Serial.printf("Feeder %d: angle=%d, state=%s, feedback=%s\n", 
              id, angle, getStateString(state), 
              feedbackEnabled ? "ON" : "OFF");

// 4. 性能监控
unsigned long startTime = millis();
// 执行操作...
unsigned long duration = millis() - startTime;
if (duration > 1000) {
    DEBUG_PRINT("Operation took " + String(duration) + "ms");
}
```

### 错误处理策略
```cpp
// 1. 分层错误处理
enum ErrorCode {
    ERROR_NONE = 0,
    ERROR_INVALID_PARAMETER = 1,
    ERROR_COMMUNICATION_TIMEOUT = 2,
    ERROR_DEVICE_OFFLINE = 3,
    ERROR_HARDWARE_FAILURE = 4
};

// 2. 错误恢复机制
class ErrorManager {
public:
    void handleError(ErrorCode error, int deviceId);
    void attemptRecovery(int deviceId);
    bool isRecoveryPossible(ErrorCode error);
};

// 3. 超时和重试
bool sendCommandWithRetry(int deviceId, const uint8_t* data, int retries = 3) {
    for (int i = 0; i < retries; i++) {
        if (sendCommand(deviceId, data)) {
            return true;
        }
        delay(100 * (i + 1)); // 递增延迟
    }
    return false;
}
```

---

## 🔄 版本管理和扩展

### 当前架构特点
- **模块化设计**: 各功能模块独立，便于扩展
- **异步通信**: ESP-NOW非阻塞通信，提高响应性
- **本地智能**: Hand端本地处理，减少通信依赖
- **可配置性**: 运行时参数调整，适应不同需求

### 扩展路线图
1. **高级反馈算法**: 预测性维护、故障分析
2. **多传感器支持**: 温度、压力、位置传感器集成
3. **Web界面**: 浏览器基础的监控和配置界面
4. **批量管理**: 配置模板、批量部署工具
5. **数据记录**: 操作日志、性能统计、趋势分析

### 兼容性考虑
- **向后兼容**: 新版本保持旧G-code命令兼容
- **渐进升级**: 支持Brain和Hand独立升级
- **配置迁移**: 自动迁移旧版本配置

---

*本开发者指南提供了ESP-NOW PNP系统的完整技术实现细节。对于测试验证、性能基准和故障排除，请参考测试指南。*
