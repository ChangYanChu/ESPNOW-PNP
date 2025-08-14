# PandFeeder 串口 G-code 控制

本工程实现多板级联的 G-code 送料/舵机控制：

- 每块控制板使用 1 片 PCA9685，当前固件支持本地 16 路舵机（逻辑 L0..L15）。
- 通过 3 根地址跳线 GPIO5/6/7（接地=1，上拉=0）设定板地址 addr(0..7)。
- 全局飞达 ID (G) = addr * 16 + 本地 L。G-code 中 `N` 现在使用全局 ID；固件自动映射到本地通道与真实 PWM 通道。 
- 上电 WS2812（GPIO4）显示绿色呼吸灯，表示系统运行正常。
- 返回统一前缀：`ok ...` 或 `error ...`。

## 支持的 G-code 指令
（示例值中 N=G 表示全局 ID）

| 指令 | 作用 | 备注 |
|------|------|------|
| M115 | 基础信息 | `local_channels, addr, base` |
| M116 | 映射表 | 输出 `addr, base, range, map=G->L,...` |
| M17 / M18 | 全局使能开/关 | 控制 PCA9685 OE（低有效） |
| M610 S0/1 | 查询/设置全局使能 | 不带 S 查询 |
| M611 N<G> S0/1 | 启用/禁用单个全局飞达 | 不带 N 作用于本板全部本地 | 
| M280 P<L> S<角度> | (旧接口) 本地通道直接设角 | 仍保留，范围 0..15 |
| M603 N<G> A<角度> | 全局飞达设角 | 推荐使用全局 ID 接口 |
| M600 N<G> F<len> | 送料动作 | len=2..24 且偶数；4mm 为一个完整循环，2mm=半循环 |
| M601 N<G> | 回到收回角 | 使用该飞达配置的收回角 C |
| M602 N<G> | 查询飞达状态 | 输出 G/L 与配置字段 |
| M620 N<G> A B C F U V W X | 设置飞达配置 | 仅影响本地板上对应 L |
| M621 | 列出本板所有本地配置 | 带 G/L 映射 |

字段含义：
- A: fullAdvanceAngle 进给最大角
- B: halfAdvanceAngle 半行程角
- C: retractAngle 收回角
- F: defaultFeedLen 缺省送料长度 (2..24, 偶数)
- U: settleTimeMs 每个动作后的延时(ms)
- V/W: minTicks/maxTicks PWM 脉宽范围（微调角度行程）
- X: ignoreFeedback (预留)

## 地址跳线 (GPIO5/6/7)
读取逻辑：接地=1，未接=0。位顺序：GPIO7:bit2, GPIO6:bit1, GPIO5:bit0。

| GPIO7 | GPIO6 | GPIO5 | addr | 全局范围 |
|-------|-------|-------|------|-----------|
| 开 | 开 | 开 | 0 | 0–15 |
| 开 | 开 | 接 | 1 | 16–31 |
| 开 | 接 | 开 | 2 | 32–47 |
| 开 | 接 | 接 | 3 | 48–63 |
| 接 | 开 | 开 | 4 | 64–79 |
| 接 | 开 | 接 | 5 | 80–95 |
| 接 | 接 | 开 | 6 | 96–111 |
| 接 | 接 | 接 | 7 | 112–127 |

“接”=跳线到 GND；“开”=悬空(上拉)。

## 真实 PWM 通道重映射
逻辑 L0..L7 -> PWM0..7 (直通)
L8..L15 依据机械布线重排：
```
L8->PWM15, L9->PWM14, L10->PWM13, L11->PWM8,
L12->PWM9, L13->PWM10, L14->PWM11, L15->PWM12
```
此逻辑在 `pf_servo.cpp` 的 `logicalToPhysical()` 中实现。

## WS2812 状态灯
- GPIO4 单颗 WS2812B：上电短绿后进入绿色呼吸（系统心跳）。
- 可在 `pf_led.cpp` 调整默认亮度或效果。

## 构建与烧录
- 依赖：PlatformIO
- 目标板：esp32-c3-devkitm-1（见 `platformio.ini`）

可选命令（在 PandFeeder 目录）：
- 构建：`platformio run -e esp32-c3-devkitm-1`
- 烧录：`platformio run -e esp32-c3-devkitm-1 -t upload`
- 串口监视器：`platformio device monitor -b 115200`

## 主要引脚
| 功能 | 引脚 | 说明 |
|------|------|------|
| I2C SDA / SCL | 默认板载 | PCA9685 通讯 |
| PCA9685 OE | GPIO10 | 低有效 |
| 地址跳线 | GPIO5/6/7 | 接地=1 组成 addr |
| WS2812 数据 | GPIO4 | 状态灯 |
| UART IN TX/RX | GPIO1 / GPIO0 | 外部输入串口 |
| UART OUT TX/RX | GPIO21 / GPIO20 | 外部输出串口 |

## 快速测试
1. 设置地址跳线，上电后串口（115200）发送 `M115`，确认 addr/base。
2. `M116` 查看映射表。
3. `M603 N<base> A90` 设该板第一个全局飞达角度。
4. `M600 N<base> F8` 测试送料一次。
5. `M602 N<base>` 查看状态与配置。

## 说明
* 多板扩展：全局 ID 按 16 通道递增，无需修改上位机逻辑。
* 若需兼容旧固件仅本地 ID，可添加模式开关（未默认提供）。
* 角度与 PWM 行程可通过 M620 的 V/W、A/B/C 调整适配不同舵机。
* 若需更多诊断（I2C 扫描等）可扩展新指令（例如 M119）。

## 目录概览
| 文件 | 作用 |
|------|------|
| `src/pf_gcode.cpp` | G-code 解析与命令处理 |
| `src/pf_servo.cpp` | 舵机/送料逻辑、逻辑->物理通道映射 |
| `src/pf_board.cpp` | 板地址读取与全局 ID 计算 |
| `src/pf_led.cpp` | WS2812 呼吸灯效果 |
| `src/pf_uart.cpp` | 额外 IN/OUT UART 通道支持 |
| `src/pf_config.h` | 可调编译期配置 |

---
如需添加更多查询命令或文档章节，提出即可。
