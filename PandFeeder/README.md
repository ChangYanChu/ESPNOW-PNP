# PandFeeder 串口 G-code 控制

本工程已集成 G-code 解析与 PCA9685 舵机控制：通过串口接收 G-code 指令，驱动 HCPCA9685 上的 0..12 共 13 个通道舵机，并按 `ok ...` / `error ...` 返回执行结果。

## 支持的 G-code 指令
(完整参考见仓库 doc/PandFeeder-GCode.md)
- M115
  - 查询设备信息
- M17 / M18
  - 使能/禁用 PCA9685 输出（OE 低有效）
- M610 S0/1（或不带 S 查询）
  - 与原架构一致的电源/使能语义映射到 OE 输出
- M280 P<id> S<angle>
  - 设置单通道舵机角度（id: 0..12，angle: 0..180）
  - 例：`M280 P0 S90`、`M280 P5 S10`
- M600 N<id> F<len>
  - 送料动作，模仿 hand 逻辑，以 4mm 为一个动作；len 必须为偶数且 2..24
  - 例：`M600 N0 F8`

响应格式：`ok ...` 或 `error ...`，与 Firmware/brain 的习惯统一。

## 构建与烧录
- 依赖：PlatformIO
- 目标板：esp32-c3-devkitm-1（见 `platformio.ini`）

可选命令（在 PandFeeder 目录）：
- 构建：`platformio run -e esp32-c3-devkitm-1`
- 烧录：`platformio run -e esp32-c3-devkitm-1 -t upload`
- 串口监视器：`platformio device monitor -b 115200`

## 连接与引脚
- PCA9685 I2C 地址：0x40（可在 `src/pf_config.h` 修改）
- OE 引脚：GPIO10（低有效）
- 角度-脉宽映射：`10..450` ticks（可在 `src/pf_config.h` 调整）

## 快速测试
1. 上电后打开串口（115200），应看到：`PandFeeder ready ...`
2. 发送：`M115` → 返回设备信息
3. 发送：`M17` 使能输出
4. 发送：`M280 P0 S90` 观察 0 号通道舵机旋转到 90°
5. 发送：`M600 N0 F8` 进行一次送料序列

## 说明
- PandFeeder 已移除 Web 控制页面依赖，专注串口 G-code 控制（可按需再开新分支加入 Web）。
- 舵机数默认 13 个通道（0..12），如需变更请修改 `SERVO_CHANNEL_COUNT`。
- 若需与 Firmware/brain 的更多 G-code 保持一致，可在 `src/pf_gcode.cpp` 扩展。
