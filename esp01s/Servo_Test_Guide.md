# ESP01S 舵机测试配置指南

## 舵机测试开关配置

在 `src/hand/hand_config.h` 中可以配置舵机测试行为：

### 主要测试开关

```cpp
// 启用启动时舵机功能测试
#define ENABLE_SERVO_STARTUP_TEST

// 如果注释掉上面这行，启动时将跳过舵机测试
// #define ENABLE_SERVO_STARTUP_TEST
```

### 舵机行为控制

```cpp
// 静默模式：舵机仅在明确指令时才连接和移动（推荐）
#define SERVO_SILENT_MODE

// 启动时立即连接舵机（可能导致舵机移动）
// #define SERVO_ATTACH_ON_STARTUP
```

### 详细配置选项

```cpp
// 详细的舵机测试输出
#define SERVO_TEST_VERBOSE

// 舵机测试每步延迟时间(毫秒)
#define SERVO_TEST_DELAY 800

// 舵机首次连接时的初始角度
#define SERVO_INITIAL_ANGLE 90
```

## 舵机测试模式

### 1. 启动时自动测试

当启用 `ENABLE_SERVO_STARTUP_TEST` 时，系统启动时会自动执行舵机测试：

**测试序列:**
1. 连接舵机
2. 移动到 90°
3. 移动到 0°
4. 移动到 180°
5. 返回 90°
6. 断开舵机

**LED状态指示:**
- 测试期间：LED显示工作状态（短暂常亮）
- 测试失败：LED显示错误状态（快速闪烁）

### 2. 手动测试命令

无论是否启用启动测试，都可以通过串口手动执行测试：

```
test_servo
```

**手动测试特点:**
- 保存并恢复原有LED状态
- 提供详细的测试步骤输出
- 测试序列：0° → 90° → 180° → 90°

## 配置示例

### 生产环境配置（静默模式，推荐）
```cpp
// 生产环境：启用静默模式，防止开机时舵机移动
#define SERVO_SILENT_MODE
// #define ENABLE_SERVO_STARTUP_TEST  // 关闭启动测试节省时间
// #define SERVO_TEST_VERBOSE
#define SERVO_TEST_DELAY 500        // 手动测试时仍可用
#define SERVO_INITIAL_ANGLE 90      // 默认角度
```

### 开发环境配置（启用所有测试）
```cpp
// 开发环境：启用完整测试但使用静默模式
#define SERVO_SILENT_MODE
#define ENABLE_SERVO_STARTUP_TEST
#define SERVO_TEST_VERBOSE
#define SERVO_TEST_DELAY 1000       // 较长延迟便于观察
```

### 快速测试配置
```cpp
// 快速测试：启用但减少延迟，使用静默模式
#define SERVO_SILENT_MODE
#define ENABLE_SERVO_STARTUP_TEST
// #define SERVO_TEST_VERBOSE       // 简化输出
#define SERVO_TEST_DELAY 400        // 减少延迟
```

### 传统模式配置（可能导致开机移动）
```cpp
// 仅在需要验证传统行为时使用
// #define SERVO_SILENT_MODE        // 关闭静默模式
#define SERVO_ATTACH_ON_STARTUP     // 启动时立即连接
#define ENABLE_SERVO_STARTUP_TEST
```

## 串口命令

测试相关的串口命令：

| 命令 | 功能 | 示例 |
|------|------|------|
| `test_servo` | 手动执行舵机测试 | `test_servo` |
| `help` | 显示所有可用命令 | `help` |

## 故障排除

### 舵机开机时移动问题
如果舵机在开机时仍然移动：

1. **启用静默模式**
   ```cpp
   #define SERVO_SILENT_MODE
   ```

2. **关闭启动时连接**
   ```cpp
   // #define SERVO_ATTACH_ON_STARTUP
   ```

3. **调整初始角度**
   ```cpp
   #define SERVO_INITIAL_ANGLE 90  // 或您舵机的中位角度
   ```

### 舵机测试失败
如果看到 "Warning: Servo attach failed during test"：

1. **检查硬件连接**
   - 确认舵机信号线连接到GPIO2
   - 确认舵机电源连接（外部5V）
   - 确认共地连接

2. **检查电源供应**
   - 舵机需要外部5V电源
   - ESP01S的3.3V无法驱动舵机

3. **检查GPIO配置**
   - 确认 `SERVO_PIN` 配置正确
   - 如果使用GPIO0，确保启动完成后再测试

### LED状态异常
如果LED显示错误状态：

1. **检查引脚冲突**
   - GPIO2同时用于舵机和LED
   - 确保舵机断开后LED能正常工作

2. **检查状态管理**
   - LED状态应在舵机工作时暂停
   - 舵机完成后恢复LED状态

## 性能优化

### 减少启动时间
- 注释掉 `ENABLE_SERVO_STARTUP_TEST`
- 减少 `SERVO_TEST_DELAY` 值

### 增强调试信息
- 启用 `SERVO_TEST_VERBOSE`
- 启用相应的调试宏

### 自定义测试序列
可以修改 `runServoTest()` 函数来自定义测试角度和序列。

## 注意事项

1. **ESP01S限制**
   - 只有GPIO0和GPIO2可用于舵机控制
   - GPIO0有启动时序要求
   - GPIO2与板载LED共用

2. **电源要求**
   - 舵机必须使用外部电源
   - 功耗过大可能导致ESP01S重启

3. **测试时机**
   - 启动测试在ESP-NOW初始化之后
   - 手动测试可在任何时候执行
   - 避免在舵机工作时执行其他测试
