<!-- filepath: /Users/chanchu/Code/ESPNOW-PNP/README.md -->
# ESPNOW-PNP 项目文档

## 1. 作者注
本人非软件工程专业，属于业余开发者。本项目的主要代码由 AI 辅助编写，我负责提供整体架构设计和思路。

## 2. 项目概述
ESPNOW-PNP 是一个基于 ESP-NOW 无线通信协议的 Pick and Place (PNP) 系统组件。该系统主要由一个中央控制单元 ("Brain") 和多个执行单元 ("Hand") 组成。"Brain" 负责接收上层指令 (如 G-code)，管理并调度 "Hand" 单元执行物料拾取和放置相关的喂料动作。"Hand" 单元直接控制伺服电机等执行机构完成具体的喂料操作。

本项目借鉴了 [MaxFeeder](https://github.com/RinthLabs/MaxFeeder) 项目的许多代码和设计思想。

## 3. 系统架构
*   **核心组件**:
    *   **Brain (大脑)**: 通常运行在 ESP32C3 开发板 (例如 `esp32-c3-devkitm-1`)。作为主控制器，负责：
        *   解析 G-code 指令。
        *   通过 ESP-NOW 与多个 Hand 通信。
        *   管理 Hand 单元的状态（在线、离线）。
        *   通过 LCD 显示系统状态和操作信息。
    *   **Hand (手)**: 通常运行在 ESP8266 系列芯片 (例如 ESP-01S)。作为从控制器，负责：
        *   接收 Brain 发送的指令。
        *   控制伺服电机 (Servo) 执行喂料动作。
        *   向 Brain 发送状态响应。
        *   每个 Hand 控制一个或多个喂料器 (Feeder)。
*   **通信方式**:
    *   Brain 和 Hand 之间主要通过 **ESP-NOW** 协议进行无线通信。ESP-NOW 是一种低功耗、快速响应的通信协议，适合此类多设备协作场景。
    *   Brain 通过串口接收 G-code 指令。
*   **硬件平台**:
    *   Brain: `espressif32` 平台, `esp32-c3-devkitm-1` 板, Arduino 框架。
    *   Hand: `espressif8266` 平台, `esp01_1m` 板, Arduino 框架。

## 4. ESP-NOW 通信协议 (`src/common/espnow_protocol.h`)
ESP-NOW 通信协议定义了 Brain 和 Hand 之间交换数据的格式和类型。

*   **数据包结构**:
    *   `ESPNowPacket` (Brain -> Hand): 用于 Brain 向 Hand 发送指令。
      ```cpp
      // filepath: src/common/espnow_protocol.h
      struct ESPNowPacket {
          uint8_t commandType;             // 命令类型 (ESPNowCommandType)
          uint8_t feederId;                // 喂料器ID (0xFF 为广播)
          uint8_t feedLength;              // 喂料长度 (例如，单位mm)
          uint8_t reserved[4];             // 保留字段
      } __attribute__((packed));
      ```
    *   `ESPNowResponse` (Hand -> Brain): 用于 Hand 向 Brain 发送响应。
      ```cpp
      // filepath: src/common/espnow_protocol.h
      struct ESPNowResponse {
          uint8_t handId;                  // 手部ID (即 Feeder ID)
          uint8_t commandType;             // 原始命令类型
          uint8_t status;                  // 状态码 (ESPNowStatusCode)
          uint8_t reserved[3];             // 保留字段
          uint32_t sequence;               // 对应的序列号 (当前未使用)
          uint32_t timestamp;              // 时间戳 (当前未使用)
          char message[16];                // 状态消息 (例如 "Feed OK", "Online")
      } __attribute__((packed));
      ```
*   **命令类型 (`ESPNowCommandType`)**:
    *   `CMD_FEEDER_ADVANCE (0x04)`: 喂料推进指令。
    *   `CMD_STATUS_REQUEST (0x06)`: 状态查询指令 (当前主要通过心跳实现)。
    *   `CMD_RESPONSE (0x07)`: 响应命令 (Hand -> Brain)。
    *   `CMD_HEARTBEAT (0x08)`: 心跳包 (Brain -> Hand, Hand 响应心跳)。
    *   `CMD_HAND_REGISTER (0x09)`: Hand 注册命令 (Hand -> Brain, Hand 启动时发送)。
*   **状态码 (`ESPNowStatusCode`)**:
    *   `STATUS_OK (0x00)`: 操作成功。
    *   `STATUS_ERROR (0x01)`: 通用错误。
    *   `STATUS_BUSY (0x02)`: 设备忙。
    *   `STATUS_TIMEOUT (0x03)`: 操作超时。
    *   `STATUS_INVALID_PARAM (0x04)`: 无效参数。

## 5. Brain 单元逻辑 (`src/brain/`)
*   **初始化 (`brain_main.cpp` -> `setup()`)**:
    *   初始化串口通信 (用于 G-code 和调试信息)。
    *   初始化 LCD 显示屏。
    *   初始化 ESP-NOW 通信 (`espnow_setup()` in `brain_espnow.cpp`)，设置回调函数 `dataReceived` 处理来自 Hand 的消息。
*   **主循环 (`brain_main.cpp` -> `loop()`)**:
    *   更新 LCD 显示内容 (`lcd_update()`)。
    *   监听串口，接收并处理 G-code 指令 (`listenToSerialStream()` in `gcode.cpp`)。
    *   处理接收到的 ESP-NOW 响应 (`processReceivedResponse()` in `brain_espnow.cpp`)。
    *   检查是否有命令发送超时 (`checkCommandTimeout()` in `brain_espnow.cpp`)。
    *   定期发送心跳包给所有 Hand，并检测 Hand 的在线状态 (`sendHeartbeat()` in `brain_espnow.cpp`)。
    *   更新 LCD 上显示的在线 Hand 数量。
*   **ESP-NOW 通信 (`brain_espnow.cpp`)**:
    *   `espnow_setup()`: 初始化 ESP-NOW，注册数据接收回调。
    *   `dataReceived()`: 回调函数，当收到 Hand 的 ESP-NOW 数据包时被调用。解析 `ESPNowResponse`，更新 Hand 状态，标记 `hasNewResponse`。
    *   `processReceivedResponse()`: 处理 `hasNewResponse` 标记的响应。根据响应类型（如喂料完成、心跳响应）更新内部状态，并通过串口发送 G-code 响应 (`sendAnswer()`)。
    *   `sendFeederAdvanceCommand()`: 向指定的 Hand 发送喂料指令 (`CMD_FEEDER_ADVANCE`)。设置 `waitingForResponse` 标志，记录命令发送时间用于超时检测。
    *   `sendHeartbeat()`: 定期向所有 Hand 广播或逐个发送心跳包 (`CMD_HEARTBEAT`)。
    *   `checkCommandTimeout()`: 检查 `waitingForResponse` 状态，如果命令超时未收到响应，则认为命令失败。
    *   `getOnlineHandCount()`: 根据 `lastHandResponse` 数组（记录每个 Hand 最后响应时间）统计在线 Hand 数量。
*   **G-code 处理 (`gcode.cpp`, `gcode.h`)**:
    *   `listenToSerialStream()`: 从串口读取 G-code 指令行。
    *   `processCommand()`: 解析 G-code 指令 (主要是 M-code)。
        *   `M600 N<feederId> F<feedLength>`: 喂料指令。调用 `sendFeederAdvanceCommand()` 通过 ESP-NOW 发送给对应的 Hand。
        *   `M610 S<0|1>`: 使能/禁用所有喂料器。
        *   其他 M-code (部分在测试脚本中可见，如 `M601`, `M602`, `M280`)。
    *   `sendAnswer()`: 通过串口向上位机发送 G-code 执行结果 (e.g., "ok", "error <message>")。
*   **LCD 显示 (`lcd.cpp`, `lcd.h`)**:
    *   使用 `LCDI2C_Multilingual_MCD` 库控制 I2C LCD1602 屏幕。
    *   `lcd_setup()`: 初始化 LCD，定义自定义字符。
    *   `lcd_update()`: 根据当前模式和系统状态刷新 LCD 显示内容。
    *   显示模式包括：启动信息、系统运行状态、G-code 命令、错误信息等。

## 6. Hand 单元逻辑 (`src/hand/`)
*   **初始化 (`hand_main.cpp` -> `setup()`)**:
    *   初始化串口通信。
    *   初始化 Feeder ID 管理器 (`initFeederID()` in `feeder_id_manager.cpp`)。
    *   初始化 ESP-NOW 通信 (`espnow_setup()` in `hand_espnow.cpp`)。
    *   初始化舵机控制 (`setup_Servo()` in `hand_servo.cpp`)。
*   **主循环 (`hand_main.cpp` -> `loop()`)**:
    *   调用舵机 tick 函数 (`servoTick()` in `hand_servo.cpp`)。
    *   处理串口命令 (`processSerialCommand()` in `feeder_id_manager.cpp`)。
    *   处理接收到的 ESP-NOW 命令 (`processReceivedCommand()` in `hand_espnow.cpp`)。
    *   处理待发送的 ESP-NOW 响应 (`processPendingResponse()` in `hand_espnow.cpp`)。
*   **ESP-NOW 通信 (`hand_espnow.cpp`)**:
    *   `dataReceived()`: 回调函数，处理来自 Brain 的 `ESPNowPacket`。
    *   `processReceivedCommand()`: 根据命令类型执行操作，如 `CMD_FEEDER_ADVANCE`, `CMD_HEARTBEAT`。
    *   `handleFeederAdvanceCommand()`: 执行喂料动作，准备响应。
    *   `schedulePendingResponse()`: 设置待发送的响应内容。
    *   `processPendingResponse()`: 发送 `ESPNowResponse` 给 Brain。
*   **舵机控制 (`hand_servo.cpp`, `hand_servo.h`)**:
    *   使用 `SoftServo` 库。
    *   `setup_Servo()`: 初始化舵机。
    *   `feedTapeAction(uint8_t feedLength)`: 核心喂料逻辑，控制舵机执行预定动作序列。
    *   `servoTick()`: 软舵机库循环调用。
*   **Feeder ID 管理 (`feeder_id_manager.cpp`, `feeder_id_manager.h`)**:
    *   `initFeederID()`: 从 EEPROM 加载或使用默认 Feeder ID。
    *   `saveFeederID()`: 保存 Feeder ID 到 EEPROM。
    *   `getCurrentFeederID()`: 获取当前 Hand ID。
    *   `processSerialCommand()`: 支持通过串口查询和设置 Feeder ID。

## 7. 配置 (`platformio.ini`, `src/brain/brain_config.h`, `src/hand/hand_config.h`)
*   **`platformio.ini`**:
    *   定义了 `env:esp32c3-brain` 和 `env:esp01s-hand` 两个环境。
    *   详细指定了各自的平台、板子、框架、编译标志、库依赖和源文件过滤器。
*   **`src/brain/brain_config.h`**:
    *   `TOTAL_FEEDERS`: 系统支持的最大 Hand 数量。
    *   `COMMAND_TIMEOUT_MS`, `HEARTBEAT_INTERVAL_MS`, `HAND_OFFLINE_TIMEOUT_MS`: 通信和状态管理相关超时参数。
*   **`src/hand/hand_config.h`**:
    *   `FEEDER_ID`: 当前 Hand 的 ID (重要，需唯一)。
    *   `SERVO_PIN`: 舵机引脚。
    *   舵机行为、测试及 EEPROM 相关配置。

## 8. G-Code/M-Code 指令 (`src/brain/gcode.h`, `src/brain/gcode.cpp`)
Brain 单元通过串口接收 G-code 格式的指令。
*   `M600 N<feeder_id> F<feed_length> [X1]`: 执行喂料。
*   `M610 [S<0|1>]`: 使能/禁用或查询喂料器状态。
*   (部分其他 M-Code 在 `gcode.h` 中定义，具体实现在 `gcode.cpp` 中可能不完整或被注释)

## 9. 测试 (`test/` 目录)
项目包含 Python 测试脚本，用于验证系统功能：
*   **`auto_test_script.py`**: 自动化测试基本命令、舵机控制、喂料序列和错误处理。
*   **`diagnostic_tool.py`**: 快速诊断系统状态和反馈系统。
*   其他如 `hardware_test.cpp`, `test_feedback_system.py`, `test_manual_feed.py`。

## 10. 目录结构
*   `src/`: 核心源代码。
    *   `brain/`: Brain (ESP32C3) 代码。
    *   `hand/`: Hand (ESP01S) 代码。
    *   `common/`: Brain 和 Hand 共用代码 (如协议定义)。
*   `lib/`, `include/`: 库和头文件。
*   `test/`: 测试脚本和代码。
*   `demo/`: 示例或参考代码 (其中 `0816feeder` 是一个功能更复杂的独立喂料器项目，与当前 ESPNOW-PNP 架构不完全相同)。
*   `platformio.ini`: PlatformIO 配置文件。

## 11. 进一步分析和注意事项
*   **Hand 注册**: `CMD_HAND_REGISTER` 协议已定义，但具体实现流程需进一步确认。
*   **MAC 地址管理**: 当前似乎依赖 Hand 响应心跳来动态管理，而非静态 MAC 列表。
*   **G-code 完整性**: `gcode.cpp` 中部分 M-code 的实现可能与测试脚本或 `gcode.h` 定义存在差异，需确认最终支持的指令集。

## 12. 免责声明
本项目按“原样”提供，不作任何明示或暗示的保证，包括但不限于对适销性、特定用途适用性和非侵权性的保证。在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任承担任何责任，无论是在合同诉讼、侵权行为还是其他方面，由软件或软件的使用或其他交易引起、产生或与之相关的。

您可以自由更改源代码以满足您的需求，但请注意，修改后的代码可能不受原始作者的支持。

请自行承担使用风险。

## 13. 贡献
欢迎对此项目做出贡献！如果您有任何改进建议、错误修复或新功能，请随时创建 Pull Request 或提交 Issue。

我们鼓励社区参与，并感谢所有帮助改进此项目的贡献者。
