# 配置文件整合说明

## 概述
为了避免在多个配置文件中重复维护WiFi密码等共同配置，我们将配置文件进行了重构。

## 新的配置结构

### 1. 通用配置文件 (`src/common/common_config.h`)
包含所有设备共享的配置项：
- **WiFi配置**: SSID、密码、功率设置
- **系统版本**: 统一的版本号管理
- **喂料器系统配置**: 总数量、默认参数等
- **超时配置**: 命令超时、心跳间隔等
- **EEPROM配置**: 大小、地址定义
- **硬件引脚定义**: ESP01S通用引脚常量
- **调试模式常量**: 便于统一管理

### 2. Brain专用配置文件 (`src/brain/brain_config.h`)
包含Brain控制器特有的配置：
- **串口波特率**: 115200
- **LCD支持**: 硬件LCD开关
- **G-code处理器**: 缓冲区大小等
- **调试模式**: 默认启用（开发模式）

### 3. Hand专用配置文件 (`src/hand/hand_config.h`)
包含Hand控制器特有的配置：
- **喂料器ID**: 支持远程配置
- **硬件引脚分配**: 舵机、按钮引脚
- **舵机测试**: 启动测试配置
- **调试模式**: 默认禁用（正常模式）

## 主要改进

### 1. WiFi配置统一管理
- ✅ **之前**: 需要在`brain_config.h`和`hand_config.h`中分别配置
- ✅ **现在**: 只需在`common_config.h`中配置一次

### 2. 版本号统一
- 所有模块使用`SYSTEM_VERSION`，便于统一管理

### 3. 硬件引脚标准化
- 使用`ESP01S_GPIOx`常量，提高代码可读性

### 4. 调试模式优化
- Brain: 默认开启调试（开发设备）
- Hand: 默认关闭调试（生产设备）

## 使用方法

### 修改WiFi配置
只需修改`src/common/common_config.h`中的：
```cpp
#define WIFI_SSID "你的WiFi名称"
#define WIFI_PASSWORD "你的WiFi密码"
```

### 修改系统版本
只需修改`src/common/common_config.h`中的：
```cpp
#define SYSTEM_VERSION "1.0.1"
```

### 调整喂料器数量
只需修改`src/common/common_config.h`中的：
```cpp
#define TOTAL_FEEDERS 50
```

## 兼容性说明
- 保持了所有原有的宏定义名称
- 现有代码无需修改，只需重新编译
- 配置文件层次清晰，便于维护

## 文件依赖关系
```
common_config.h
├── brain_config.h (includes common_config.h)
└── hand_config.h (includes common_config.h)
```

这样的结构确保了配置的一致性，减少了维护成本。
