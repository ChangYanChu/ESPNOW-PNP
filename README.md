# ESP-NOW PNP 分布式Pick & Place喂料器系统

> 基于ESP32C3 (Brain) 和 ESP01S (Hand) 的高性能分布式喂料器控制系统

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Ready-green.svg)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-C3-blue.svg)](https://www.espressif.com/en/products/socs/esp32-c3)
[![ESP8266](https://img.shields.io/badge/ESP8266-ESP01S-orange.svg)](https://www.espressif.com/en/products/socs/esp8266)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## 🌟 系统特性

### 🏗️ 分布式架构
- **Brain控制器**: ESP32C3主控，负责G-code解析和系统协调
- **Hand控制器**: ESP01S分布式控制，每个喂料器独立控制
- **ESP-NOW通信**: 低延迟无线通信，自动设备发现和注册

### 🔧 智能反馈系统  
- **胶带状态检测**: 微动开关实时监测SMT胶带装载状态
- **预检验证**: 推进前自动验证胶带张力，防止取料错误
- **手动进料**: 短按张紧器触发手动进料(5-50ms检测窗口)
- **错误覆盖**: 支持强制操作，便于调试和维护

### 📟 完整G-code支持
- **11个标准命令**: 涵盖系统控制、喂料器操作、反馈管理
- **参数验证**: 严格的参数范围检查和错误提示
- **实时状态**: 详细的系统状态查询和诊断信息

## 🚀 快速开始

### 硬件准备
```
Brain控制器:  ESP32C3 开发板
Hand控制器:   ESP01S 模块 (每个喂料器1个)
反馈硬件:    常开型微动开关 + 胶带张紧机构
舵机:       SG90或兼容的9g舵机
```

### 软件安装
```bash
# 1. 克隆项目
git clone https://github.com/your-repo/ESPNOW-PNP.git
cd ESPNOW-PNP

# 2. 使用PlatformIO编译和上传
# Brain控制器 (ESP32C3)
pio run -e brain -t upload

# Hand控制器 (ESP01S) 
pio run -e hand -t upload
```

### 系统配置
```gcode
# 连接Brain控制器串口，执行初始化命令:
M610 S1          # 启用系统
M620             # 检查所有Hand状态  
M605 N0 S1       # 启用喂料器0反馈检测
M602 N0          # 查询喂料器0状态
```

## 📋 G-code命令参考

### 🔧 核心系统命令
| 命令 | 功能 | 语法 | 示例 |
|------|------|------|------|
| `M610` | 系统启用/禁用 | `M610 S<0/1>` | `M610 S1` |
| `M600` | 喂料器推进 | `M600 N<id> F<length>` | `M600 N0 F4` |
| `M601` | 喂料器回缩 | `M601 N<id>` | `M601 N0` |
| `M602` | 状态查询 | `M602 N<id>` | `M602 N0` |
| `M280` | 舵机角度设置 | `M280 N<id> A<angle>` | `M280 N0 A120` |
| `M603` | 配置更新 | `M603 N<id> A<angle> F<length>` | `M603 N0 A120 F4` |
| `M620` | 所有状态查询 | `M620` | `M620` |

### 📡 反馈系统命令
| 命令 | 功能 | 语法 | 示例 |
|------|------|------|------|
| `M604` | 反馈状态检查 | `M604 N<id>` | `M604 N0` |
| `M605` | 启用/禁用反馈 | `M605 N<id> S<0/1>` | `M605 N0 S1` |
| `M606` | 清除手动进料标志 | `M606 N<id>` | `M606 N0` |
| `M607` | 处理手动进料 | `M607 N<id>` | `M607 N0` |

### 参数范围
- **喂料器编号**: 0-49 (最多50个喂料器)
- **推进长度**: 2-24mm (必须为偶数)
- **舵机角度**: 0-180度
- **脉宽范围**: 500-2500μs

## 🔗 常用工作流程

### 标准取料流程
```gcode
M610 S1          # 1. 启用系统
M602 N0          # 2. 检查喂料器0状态
M600 N0 F4       # 3. 推进4mm (自动反馈检查)
M601 N0          # 4. 回缩到初始位置
```

### 故障排查流程  
```gcode
M620             # 1. 检查所有Hand状态
M602 N0          # 2. 检查特定喂料器详细状态
M604 N0          # 3. 检查反馈系统状态
M606 N0          # 4. 清除错误标志
M605 N0 S0       # 5. 禁用反馈(如需强制操作)
M600 N0 F4       # 6. 强制推进测试
```

## 📚 文档导航

### 🔰 [用户手册](docs/ESP-NOW-PNP-User-Manual.md)
**面向最终用户** - 快速开始、命令速查、基本操作和故障排除

### 🔧 [开发者指南](docs/ESP-NOW-PNP-Developer-Guide.md) 
**面向开发人员** - 项目结构、技术架构、反馈系统集成和扩展开发

### 🧪 [测试和维护指南](docs/ESP-NOW-PNP-Testing-Guide.md)
**面向测试和运维人员** - 测试流程、性能基准、调试方法和系统维护

### 📝 补充文档
- **[ESP-NOW通信分析](espnow_communication_analysis.md)** - 通信协议详解
- **[更新日志](CHANGELOG.md)** - 版本变更记录和新功能
- **[原始Demo分析](demo/0816feeder/)** - 原始feeder系统参考实现

### 📝 项目文档
- **[更新日志](CHANGELOG.md)** - 版本变更记录和新功能
- **[原始Demo分析](demo/0816feeder/)** - 原始feeder系统参考实现

## 🏗️ 系统架构

```
┌─────────────────┐    ESP-NOW     ┌─────────────────┐
│   Brain控制器   │◄──────────────►│   Hand控制器    │
│    (ESP32C3)    │   无线通信      │    (ESP01S)     │
├─────────────────┤                ├─────────────────┤
│• G-code解析     │                │• 舵机控制       │
│• 系统协调       │                │• 反馈检测       │
│• 状态管理       │                │• 状态报告       │
│• 错误处理       │                │• 手动进料检测   │
└─────────────────┘                └─────────────────┘
         │                                  │
         │ USB串口                          │ 硬件接口
         ▼                                  ▼
┌─────────────────┐                ┌─────────────────┐
│     主机软件    │                │   喂料器机构    │
│  (G-code发送)   │                │• SG90舵机       │
└─────────────────┘                │• 微动开关       │
                                   │• 胶带张紧器     │
                                   └─────────────────┘
```

## 🔧 硬件集成

### Brain控制器 (ESP32C3)
```cpp
// 主要功能模块
├── G-code处理器      // brain_gcode.cpp
├── 喂料器管理器      // brain_feeder_manager.cpp  
├── ESP-NOW通信       // brain_espnow.cpp
└── 系统配置          // brain_config.h
```

### Hand控制器 (ESP01S)
```cpp
// 核心组件
├── 舵机控制          // hand_servo.cpp
├── 反馈检测          // hand_feedback.cpp
├── ESP-NOW通信       // hand_espnow.cpp
└── 主循环            // hand_main.cpp

// GPIO分配
├── GPIO0: 反馈输入 (INPUT_PULLUP) 
├── GPIO2: 舵机控制 (PWM输出)
└── GPIO3: 备用调试 (可选)
```

### 反馈系统硬件
```
微动开关 (常开NO型)
├── 触点1 → ESP01S GPIO0 (INPUT_PULLUP)
└── 触点2 → ESP01S GND

工作原理:
├── 胶带装载正常 → 开关闭合 → GPIO0=LOW  → 状态正常
└── 胶带未装载   → 开关断开 → GPIO0=HIGH → 状态错误
```

## 🚨 故障排除

### 常见问题

#### 1. Hand控制器连接失败
```bash
错误: Hand not responding
解决: 
1. 检查Hand控制器电源
2. 确认Brain-Hand距离 < 100米
3. 执行 M620 检查连接状态
4. 重启Brain控制器进行重新发现
```

#### 2. 反馈检测异常
```bash
错误: Feedback error - No tape detected  
解决:
1. 检查微动开关连接 (GPIO0)
2. 确认为常开(NO)型开关
3. 验证胶带张紧机构
4. 使用 M604 N0 测试反馈状态
```

#### 3. G-code命令失败
```bash
错误: Enable system first! M610 S1
解决: 先执行 M610 S1 启用系统

错误: Invalid feeder number  
解决: 使用0-49范围内的喂料器编号

错误: Invalid feed length
解决: 使用2-24mm范围内的偶数值
```

## 🔬 调试模式

### 启用调试输出
```cpp
// hand_config.h 中配置
#define DEBUG_ENABLED
#define DEBUG_HAND_FEEDBACK     // 反馈系统调试
#define DEBUG_HAND_SERVO        // 舵机控制调试  
#define DEBUG_HAND_ESPNOW       // 通信调试

// brain_config.h 中配置
#define DEBUG_ENABLED
#define DEBUG_GCODE            // G-code解析调试
#define DEBUG_FEEDER           // 喂料器管理调试
```

### 调试命令
```gcode
M602 N0          # 详细状态信息
M604 N0          # 反馈系统诊断
M620             # 全系统状态报告
```

## 🤝 贡献指南

### 开发环境
```bash
# 必需工具
- PlatformIO Core 或 IDE  
- ESP32/ESP8266工具链
- Git版本控制

# 推荐工具  
- VS Code + PlatformIO插件
- 串口调试助手
- 万用表(硬件调试)
```

### 代码规范
- 使用一致的代码格式和注释
- 新功能需要相应的文档更新
- 提交前执行编译测试
- 关键功能需要提供测试用例

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 📞 技术支持

- **Issues**: [GitHub Issues](https://github.com/your-repo/ESPNOW-PNP/issues)
- **文档**: 查看新的三层文档架构 - [用户手册](docs/ESP-NOW-PNP-User-Manual.md) | [开发者指南](docs/ESP-NOW-PNP-Developer-Guide.md) | [测试指南](docs/ESP-NOW-PNP-Testing-Guide.md)
- **示例**: 参考 `demo/` 目录中的示例代码

---

## 📊 系统状态

### 当前版本: v2.0.1
- ✅ 基础ESP-NOW通信系统
- ✅ 完整G-code命令支持 (11个命令)
- ✅ 反馈系统硬件集成
- ✅ 自动设备发现和注册  
- ✅ 错误处理和状态管理
- ✅ 完整技术文档

### 开发路线
- 🚧 高级反馈算法优化
- 🚧 预测性维护功能
- 🚧 Web界面集成
- 🚧 批量配置工具

---

*Last Updated: 2024-12-19*
