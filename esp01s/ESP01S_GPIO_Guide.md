# ESP01S GPIO配置和故障排除指南

## ESP01S引脚配置

ESP01S只有4个可用的GPIO引脚：
- **TX (GPIO1)** - 串口发送，通常用于调试输出
- **RX (GPIO3)** - 串口接收，通常用于调试输入  
- **GPIO0** - 多功能引脚，启动控制+通用IO
- **GPIO2** - 多功能引脚，启动指示+通用IO+板载LED

## 当前配置策略

### 方案A: GPIO2用于舵机控制（推荐）
```cpp
#define SERVO_PIN 2              // 舵机连接到GPIO2
#define STATUS_LED_PIN 2         // 状态LED与舵机共用引脚
#define STATUS_LED_INVERTED true // ESP01S的LED是低电平点亮
```

**优点:**
- GPIO0避免启动问题
- LED和舵机智能共享引脚
- 舵机工作时LED自动禁用

**缺点:**
- 舵机工作时无法显示LED状态

### 方案B: GPIO0用于舵机控制（备用）
```cpp
#define SERVO_PIN 0              // 舵机连接到GPIO0
#define STATUS_LED_PIN 2         // 专用LED引脚
```

**优点:**
- LED和舵机完全独立
- 可以同时显示状态和控制舵机

**缺点:**
- GPIO0在启动时不能有负载
- 需要启动完成后才能连接舵机

## GPIO0启动问题解决

如果使用GPIO0控制舵机，必须注意：

1. **启动时序**: GPIO0在启动时用于模式选择
   - 高电平 = 正常启动模式
   - 低电平 = 下载模式

2. **代码处理**:
```cpp
if (SERVO_PIN == 0) {
    // 等待启动完全完成
    delay(2000);  
    DEBUG_HAND_SERVO_PRINT("GPIO0 ready for servo control after boot");
}
```

## 故障排除

### 问题1: 舵机初始化后无法控制

**原因分析:**
- ESP01S功耗限制
- GPIO启动时序问题
- 电源供应不足
- 引脚冲突

**解决方案:**
1. 确保电源供应充足（舵机需要外部5V电源）
2. 使用合适的启动延迟
3. 在舵机不使用时及时detach()释放引脚
4. 避免同时操作LED和舵机

### 问题2: ESP01S启动失败

**原因:** GPIO0在启动时被舵机拉低

**解决方案:**
- 使用GPIO2作为舵机控制引脚
- 或者在GPIO0和舵机之间添加启动隔离电路

### 问题3: LED状态无法显示

**原因:** 舵机和LED共享引脚时冲突

**解决方案:**
- 代码中实现智能引脚调度
- 舵机工作时暂停LED
- 舵机完成后恢复LED状态

## 最佳实践

1. **引脚优先级**:
   - GPIO2: 舵机控制（优先）+ LED状态
   - GPIO0: 避免用于关键功能

2. **电源管理**:
   - 舵机使用外部电源
   - ESP01S和舵机共享GND
   - 避免从3.3V引脚给舵机供电

3. **代码优化**:
   - 使用非阻塞操作
   - 及时释放未使用的引脚
   - 实现智能引脚共享

4. **调试策略**:
   - 关闭不必要的串口输出节省资源
   - 使用LED状态指示替代串口调试
   - 分步测试各个功能模块

## 配置切换

如需更改配置，修改 `hand_config.h`:

```cpp
// 当前配置（GPIO2舵机+LED共享）
#define SERVO_PIN 2
#define STATUS_LED_PIN 2
#define STATUS_LED_INVERTED true

// 备用配置（GPIO0舵机+GPIO2专用LED）
// #define SERVO_PIN 0
// #define STATUS_LED_PIN 2
// #define STATUS_LED_INVERTED true
```

重新编译并上传固件即可生效。
