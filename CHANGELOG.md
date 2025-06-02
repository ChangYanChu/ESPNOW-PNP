# ESP-NOW PNP 系统变更记录

## v2.0.1 - 2024-12-19

### 🔧 调试系统完善
- **完成调试宏替换**: 将hand_espnow.cpp中所有直接Serial.print语句替换为统一的debug宏
- **启用调试开关**: 默认启用DEBUG_ENABLED开关，便于开发和调试
- **统一输出控制**: 确保所有调试输出都通过debug控制系统管理
- **代码清理**: 消除最后的不受控制的调试输出，完善整个调试架构

### 🎯 舵机启动优化
- **实现静默模式**: 添加SERVO_SILENT_MODE配置，彻底解决舵机开机时不必要的移动
- **智能角度管理**: 舵机连接时使用当前角度或默认角度，避免位置跳跃
- **平滑初始化**: 确保舵机仅在明确指令时才连接和移动，提升用户体验

### 🎯 完成的更改
- `hand_espnow.cpp`: 8处Serial.print语句替换为DEBUG_HAND_ESPNOW_*宏
- `hand_config.h`: 启用SERVO_SILENT_MODE和DEBUG_ENABLED开关
- `hand_servo.cpp`: 实现静默连接逻辑，防止启动时舵机移动
- 所有hand controller文件现在完全使用统一的调试宏系统

### Feeder Feedback System Analysis & Implementation Guide

Based on analysis of the original 0816 feeder demo code, we have identified a critical **tape loading detection and tension monitoring system** that should be considered for integration into the new ESP-NOW architecture.

#### Original Feedback System Analysis

**Hardware Components:**
- Each feeder has a microswitch connected to a feedback pin via `feederFeedbackPinMap[]`
- Microswitch is connected as NO (Normally Open) to feedback pin with INPUT_PULLUP
- Active LOW logic: microswitch pulls pin LOW when cover tape tension is correct

**Core Functions:**
1. **Pre-feed Validation**: System checks `feederIsOk()` before every advance operation
2. **Error State Detection**: Monitors if SMT tape is properly loaded and cover tape is tensioned
3. **Manual Feed Trigger**: Short press of tensioner allows manual tape advance (debounced 50ms)
4. **Error Override**: `ignore_feedback` setting allows bypassing feedback checks

**State Machine:**
```cpp
enum tFeederErrorState {
    sOK = 0,                    // Feedback checked, explicit feeder OK
    sOK_NOFEEDBACKLINE = 1,     // No feedback line, implicitly OK
    sERROR_IGNORED = 2,         // Error present but ignored via setting
    sERROR = -1,                // Error signaled on feedback line
}
```

**Implementation Logic:**
- `digitalRead(feederFeedbackPinMap[feederNo]) == LOW` → Tape properly loaded
- `digitalRead(feederFeedbackPinMap[feederNo]) == HIGH` → Error: No tape or improper tension
- Manual feed: Press tensioner short (5-50ms) to trigger default feed length
- Error reporting: `"feeder not OK (not activated, no tape or tension of cover tape not OK)"`

#### Integration Recommendations for ESP-NOW Architecture

**1. Hardware Integration on Hand Controllers:**
```cpp
// In hand_config.h
#define HAS_FEEDBACK_PIN
#define FEEDBACK_PIN_GPIO 3  // ESP01S GPIO3 when not using hardware serial

// In hand_servo.h
class HandServo {
private:
    bool feedbackEnabled;
    uint8_t feedbackPin;
    
public:
    bool isTapeLoaded();
    bool canAdvance();
    void enableFeedback(uint8_t pin);
};
```

**2. ESP-NOW Protocol Extensions:**
```cpp
// Add to brain_espnow.h
struct ESPNowFeederStatus {
    bool tapeLoaded;
    bool feedbackEnabled;
    uint8_t errorState;
    uint32_t lastFeedTime;
};

// Add commands
CMD_CHECK_TAPE_STATUS,
CMD_ENABLE_FEEDBACK,
CMD_OVERRIDE_FEEDBACK
```

**3. Brain Controller Integration:**
```cpp
// In brain_feeder_manager.cpp
bool FeederManager::advanceFeeder(uint8_t feederId, uint8_t length, bool overrideError) {
    // Check tape status before advance
    if (!overrideError && !checkTapeStatus(feederId)) {
        return false; // "feeder not OK - no tape or improper tension"
    }
    
    return espnowManager->sendFeederAdvance(handId, length);
}

bool FeederManager::checkTapeStatus(uint8_t feederId) {
    if (!feeders[feederId].feedbackEnabled) {
        return true; // sOK_NOFEEDBACKLINE
    }
    
    // Request status from hand controller
    return espnowManager->requestTapeStatus(feeders[feederId].handId);
}
```

**4. G-code Command Extensions:**
```cpp
// Add feedback control commands
MCODE_CHECK_TAPE_STATUS 604    // M604 N<feeder> - Check tape status
MCODE_ENABLE_FEEDBACK 605      // M605 N<feeder> S<0/1> - Enable/disable feedback
MCODE_OVERRIDE_ADVANCE 606     // M606 N<feeder> F<length> X1 - Force advance ignoring errors
```

**5. Manual Feed Implementation:**
```cpp
// In hand_main.cpp - for setup and testing
void checkManualFeedTrigger() {
    static uint32_t lastCheck = 0;
    static uint8_t tickCounter = 0;
    static bool lastState = HIGH;
    
    if (millis() - lastCheck >= 10) { // 10ms debounce
        bool currentState = digitalRead(FEEDBACK_PIN_GPIO);
        
        if (currentState != lastState && currentState == LOW) {
            tickCounter = 1; // Start counter
        } else if (currentState != lastState) {
            lastState = currentState;
        }
        
        if (tickCounter > 5 && currentState == HIGH) {
            // Manual feed triggered - send to brain
            triggerManualFeed();
            tickCounter = 0;
        }
        
        lastCheck = millis();
    }
}
```

#### Implementation Priority

**Phase 1 (High Priority):**
- Basic tape status checking before advance operations
- Error reporting via ESP-NOW protocol
- Override mechanism for setup and testing

**Phase 2 (Medium Priority):**
- Manual feed trigger via feedback pin
- Automatic error recovery mechanisms
- Status LED integration with tape status

**Phase 3 (Low Priority):**
- Advanced diagnostics and logging
- Automatic tape insertion detection
- Predictive maintenance based on feed patterns

#### Benefits for Production Systems

1. **Reliability**: Prevents pick-and-place errors due to empty feeders
2. **Setup Assistance**: Manual feed trigger simplifies feeder setup
3. **Error Detection**: Early detection of tape loading issues
4. **Maintenance**: Monitoring feed patterns for predictive maintenance
5. **Quality Control**: Ensures consistent tape advancement

This feedback system is particularly valuable for production environments where reliable tape feeding is critical for pick-and-place accuracy and uptime.

## v2.0.0 - 2024-12-19

### 🚀 重大功能更新

#### 广播发现机制
- **替换硬编码MAC地址**: 实现基于广播地址(FF:FF:FF:FF:FF:FF)的动态设备发现
- **动态手部注册**: 手部设备可自动向brain注册，提供configurable feeder ID
- **防碰撞机制**: 手部响应时使用0-2000ms随机延迟，避免同时注册冲突
- **EEPROM持久化**: 手部设备记住配置的feeder ID，重启后自动恢复

#### 系统扩展性提升
- **增加最大手部数量**: 从8个扩展到50个 (MAX_HANDS: 8 → 50)
- **增加总喂料器数量**: 从8个扩展到50个 (TOTAL_FEEDERS: 8 → 50)
- **动态映射管理**: brain启动时清空所有MAC/feeder映射，支持运行时重新配置

#### 新增管理命令
- `discovery/broadcast` - 发送广播发现信号
- `request_registration` - 请求所有手部重新注册
- `clear_registration` - 清除所有注册信息
- `status/hands` - 显示所有手部注册状态
- `help` - 显示帮助信息

### 🔧 技术改进

#### 统一调试系统架构
- **分层调试控制**: 实现主开关+模块开关的分层调试架构
- **编译时优化**: 调试关闭时代码完全移除，零运行时开销
- **模块化设计**: 每个功能模块独立的调试控制开关
- **标准化输出**: 统一的调试宏接口，支持普通和格式化输出
- **详细调试模式**: 可选的verbose模式提供更详细的跟踪信息

#### ESP-NOW通信优化
- **改进peer管理**: 修复注册时序问题，确保peer正确添加
- **增强错误处理**: 添加详细的发送状态检查和错误报告
- **连接状态监控**: 实现心跳机制和超时检测

#### G-code处理重构
- **统一命令处理**: 将管理命令和G-code命令合并到同一处理器
- **修复命令冲突**: 解决串口输入被多个处理器竞争的问题
- **增强调试输出**: 添加详细的命令解析和执行日志

#### 手部控制器改进
- **非阻塞延迟**: 替换blocking delay()，避免ESP8266看门狗超时
- **串口配置接口**: 支持通过串口设置feeder ID
- **自动心跳发送**: 定期向brain发送状态更新
- **ESP01S GPIO优化**: 针对ESP01S的GPIO限制进行专门优化
- **智能引脚共享**: GPIO2同时支持舵机控制和LED状态显示
- **启动时序处理**: 解决GPIO0启动控制问题，支持舵机可靠初始化
- **状态LED系统**: 新增可视化状态指示（初始化/搜索/连接/工作/错误）
- **舵机测试控制**: 可配置的启动舵机测试，支持生产/开发环境切换
- **手动测试命令**: 通过串口 `test_servo` 命令随时执行舵机功能测试

### 🐛 bug修复

#### 关键问题解决
- **ESP-NOW发送失败**: 修复peer注册逻辑错误导致的通信失败
- **串口命令冲突**: 重构命令处理流程，避免G-code和管理命令冲突
- **注册标志丢失**: 确保手部注册成功后正确设置registered标志
- **ESP8266崩溃**: 使用非阻塞延迟机制避免看门狗超时

#### 稳定性提升
- **内存管理**: 优化字符串操作，避免内存泄漏
- **超时处理**: 完善各种超时机制，提高系统健壮性
- **错误恢复**: 添加自动重连和状态恢复机制

### 📝 配置更新

#### 新增配置项
```cpp
// 统一调试系统配置
#define DEBUG_ENABLED       // 主调试开关，控制所有调试输出

// Brain控制器调试模块
#define DEBUG_BRAIN         // Brain主控制器调试
#define DEBUG_ESPNOW        // ESP-NOW通信调试
#define DEBUG_GCODE         // G-code处理调试
#define DEBUG_FEEDER_MANAGER // 喂料器管理调试

// Hand控制器调试模块  
#define DEBUG_HAND          // Hand主控制器调试
#define DEBUG_HAND_SERVO    // 舵机控制调试
#define DEBUG_HAND_ESPNOW   // Hand ESP-NOW通信调试

// 详细调试选项
#define DEBUG_VERBOSE_GCODE     // G-code字符级输入跟踪
#define DEBUG_VERBOSE_ESPNOW    // ESP-NOW数据包详细信息
#define DEBUG_VERBOSE_FEEDER    // 喂料器操作详细状态

// 调试宏定义示例
#ifdef DEBUG_ENABLED
  #ifdef DEBUG_BRAIN
    #define DEBUG_BRAIN_PRINT(x) Serial.print(x)
    #define DEBUG_BRAIN_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
  #else
    #define DEBUG_BRAIN_PRINT(x)
    #define DEBUG_BRAIN_PRINTF(fmt, ...)
  #endif
#else
  // 所有调试宏为空，零性能开销
  #define DEBUG_BRAIN_PRINT(x)
  #define DEBUG_BRAIN_PRINTF(fmt, ...)
#endif

// 系统扩展配置
#define TOTAL_FEEDERS 50    // 总喂料器数量
#define MAX_HANDS 50        // 最大手部数量

// ESP01S硬件配置
#define SERVO_PIN 2              // 舵机连接到GPIO2
#define STATUS_LED_PIN 2         // 状态LED引脚（与舵机共用）
#define STATUS_LED_INVERTED true // ESP01S的LED是低电平点亮

// 状态LED配置
#define LED_BLINK_FAST 200        // 快速闪烁间隔（连接中）
#define LED_BLINK_SLOW 1000       // 慢速闪烁间隔（已连接）
#define LED_BLINK_ERROR 100       // 错误闪烁间隔（错误状态）
#define LED_ON_TIME 50            // LED点亮时间（避免影响舵机）

// 舵机测试配置
#define ENABLE_SERVO_STARTUP_TEST // 启用启动时舵机功能测试
#define SERVO_TEST_VERBOSE        // 详细的舵机测试输出
#define SERVO_TEST_DELAY 800      // 舵机测试每步延迟时间(毫秒)

// EEPROM配置
#define EEPROM_SIZE 512     // EEPROM大小
#define FEEDER_ID_ADDR 0    // Feeder ID存储地址
```

#### 协议扩展
```cpp
// 新增命令类型
CMD_HAND_REGISTER     = 8   // 手部注册命令
CMD_BRAIN_DISCOVERY   = 10  // Brain发现命令
CMD_REQUEST_REGISTRATION = 11 // 请求注册命令

// 广播MAC地址
const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
```

### 🧪 测试改进

#### 统一调试系统重构
- **主调试开关**: `DEBUG_ENABLED` 全局控制所有调试输出
- **模块化调试**: 独立控制各模块调试开关
  - `DEBUG_BRAIN` - Brain主控制器调试
  - `DEBUG_ESPNOW` - ESP-NOW通信调试  
  - `DEBUG_GCODE` - G-code处理调试
  - `DEBUG_FEEDER_MANAGER` - 喂料器管理调试
  - `DEBUG_HAND` - Hand控制器调试
  - `DEBUG_HAND_SERVO` - 舵机控制调试
- **详细调试选项**: `DEBUG_VERBOSE_*` 开关提供更详细的输出
- **标准化宏**: 所有模块使用统一的调试宏格式
  - `DEBUG_MODULE_PRINT()` - 普通调试输出
  - `DEBUG_MODULE_PRINTF()` - 格式化调试输出
- **零性能开销**: 调试关闭时完全不影响运行时性能
- **代码现代化**: 完全替换散乱的Serial.print调用
- **统一输出控制**: 所有hand controller文件中的直接Serial.print语句已替换为debug宏
- **完整调试覆盖**: hand_espnow.cpp中所有调试输出现在使用统一的DEBUG_HAND_ESPNOW_*宏

#### 验证机制
- **参数验证**: G-code参数严格验证(偶数、范围检查)
- **状态检查**: 系统启用状态和设备在线状态验证
- **校验和验证**: ESP-NOW数据包完整性检查

### 📋 支持的G-code命令

| 命令 | 参数 | 功能 | 示例 |
|------|------|------|------|
| M610 | S0/S1 | 禁用/启用喂料器系统 | `M610 S1` |
| M600 | Nx Fy | 推进喂料器x，长度y mm | `M600 N0 F4` |
| M601 | Nx | 回缩喂料器x | `M601 N0` |
| M602 | Nx | 查询喂料器x状态 | `M602 N0` |
| M280 | Nx Ay | 设置喂料器x舵机角度y | `M280 N0 A90` |
| M603 | Nx... | 更新喂料器x配置 | `M603 N0 A80 B40` |
| M620 | - | 查询所有手部状态 | `M620` |

### 🔄 迁移指南

#### 从v1.x升级到v2.0.0
1. **移除硬编码MAC地址**: 不再需要在brain_config.h中配置MAC地址
2. **手部初始化**: 首次使用需要通过串口配置feeder ID
3. **发现流程**: 系统启动后使用`discovery`命令发现手部设备
4. **G-code兼容**: 所有现有G-code命令保持兼容

#### 配置步骤
1. 上传新固件到brain和hand设备
2. 手部设备串口输入: `SET_FEEDER_ID x` (x为0-49)
3. Brain设备串口输入: `discovery`
4. 验证注册: `status`
5. 启用系统: `M610 S1`
6. 测试喂料: `M600 N0 F4`

#### ESP01S状态LED指示
- **快速闪烁**: 初始化中或搜索Brain控制器
- **慢速闪烁**: 已连接到Brain，系统正常
- **短暂常亮**: 执行舵机动作中
- **极快闪烁**: 系统错误状态
- **熄灭**: 系统关闭或无电源

#### ESP01S硬件连接
- **GPIO2**: 舵机控制信号线（与板载LED共用）
- **GND**: 舵机和ESP01S共地
- **外部5V**: 舵机电源（不要用ESP01S的3.3V）
- **备用方案**: 可配置GPIO0为舵机引脚，GPIO2专用LED

#### 舵机测试命令
- **启动测试**: 默认启用，可通过注释 `ENABLE_SERVO_STARTUP_TEST` 禁用
- **手动测试**: 串口输入 `test_servo` 随时执行舵机功能测试
- **测试序列**: 0° → 90° → 180° → 90°（启动）或 0° → 90° → 180° → 90°（手动）
- **生产模式**: 注释测试开关以减少启动时间

#### 调试系统使用指南
1. **开发环境调试**: 在`brain_config.h`或`hand_config.h`中启用`DEBUG_ENABLED`
2. **模块调试**: 根据需要启用特定模块调试开关
   ```cpp
   #define DEBUG_ENABLED     // 启用调试系统
   #define DEBUG_BRAIN       // 启用Brain主控制器调试
   #define DEBUG_ESPNOW      // 启用ESP-NOW通信调试
   ```
3. **详细调试**: 启用verbose选项获取更详细信息
   ```cpp
   #define DEBUG_VERBOSE_ESPNOW  // 启用ESP-NOW详细调试
   ```
4. **生产环境**: 注释掉`DEBUG_ENABLED`以获得最佳性能
5. **串口监控**: 使用115200波特率监控调试输出

### 🚀 后续计划

#### v2.1.0 规划
- [ ] Web界面管理
- [ ] OTA固件更新
- [ ] 配置文件导入/导出
- [ ] 批量操作支持
- [ ] 性能统计dashboard

#### v3.0.0 规划
- [ ] 多brain集群支持
- [ ] 数据库存储历史记录
- [ ] REST API接口
- [ ] 移动端App控制

---

## 技术文档

### 架构图
```
Brain (ESP32C3)              Hand Controllers (ESP8266)
     |                              |
     ├── G-code处理器                ├── 串口配置接口
     ├── ESP-NOW管理器              ├── ESP-NOW通信
     ├── 喂料器管理器                ├── 舵机控制器
     └── 统一调试系统                └── EEPROM存储
     
     通信协议: ESP-NOW
     发现机制: 广播 + 随机延迟注册
     配置管理: 动态映射 + 持久化存储
```

### 调试系统架构
```
DEBUG_ENABLED (主开关)
    ├── Brain 调试模块
    │   ├── DEBUG_BRAIN (主控制器)
    │   ├── DEBUG_ESPNOW (通信模块)
    │   ├── DEBUG_GCODE (G-code处理)
    │   └── DEBUG_FEEDER_MANAGER (喂料器管理)
    │
    ├── Hand 调试模块  
    │   ├── DEBUG_HAND (主控制器)
    │   ├── DEBUG_HAND_ESPNOW (通信模块)
    │   └── DEBUG_HAND_SERVO (舵机控制)
    │
    └── 详细调试选项
        ├── DEBUG_VERBOSE_GCODE (字符级跟踪)
        ├── DEBUG_VERBOSE_ESPNOW (数据包详情)
        └── DEBUG_VERBOSE_FEEDER (详细状态)

编译优化:
- DEBUG_ENABLED 关闭 → 所有调试代码移除
- 模块开关关闭 → 该模块调试代码移除  
- 运行时零性能开销
```

### 性能指标
- **通信延迟**: < 50ms (ESP-NOW)
- **注册时间**: 0-2秒 (随机延迟)
- **心跳间隔**: 5秒
- **离线检测**: 10秒超时
- **支持设备**: 最多50个手部控制器

### 开发者注意事项
- ESP8266内存有限，避免大量字符串操作
- 使用非阻塞代码，避免看门狗超时
- ESP-NOW有包大小限制(250字节)
- 调试信息会占用内存，生产环境建议关闭

---

*本项目基于ESP-NOW协议实现分布式PNP喂料器控制系统*
