G 代码参考
以下是所有支持的 Mxxx G 代码命令的列表。

命令	简短的介绍
M600	推进特定馈线
M601	拾取后缩回
M602	馈线状态
M603	将伺服驱动到特定角度
M610	启用/禁用所有控制器上的所有馈线
M611	启用/禁用特定馈线
M620	更新馈线配置
M621	读取馈线配置
M600 推进特定喂料器
发出命令以推进一个进纸器的磁带。

用法： M600 N<feeder port> F<feedlength>

论点	描述
N<feeder port>	馈线端口号：000-412
F<feedlength>	进给长度，可选：如果给定，必须是 2 mm 的倍数，最大 24 mm
X<override error>	如果反馈线出现错误，则使用 X1 暂时忽略它
例

M600 N3 F4 ; Advance Feeder No 4 (0 based index) on port F3 by 4mm

M600 N0 ; Advance Feeder No 0 by default amount

M600 N11 F2 ; Advance Feeder No 11 by 2 mm (feeder holds on half way), might need to be tweaked for accuracy

M601 拾取后缩回
发出命令以将进纸器设置为完全缩回位置。

用途：M601 N< 进料口>

论点	描述
N<feeder port>	馈线端口号：000-412
例

M601 N103 ; check the status of feeder 103

M602 馈线状态
发出命令检查馈线。馈线的状态将打印到控制台。

用法：M602 N< 进料口>

论点	描述
N<feeder port>	馈线端口号：000-412
例

M602 N3 ; check the status of feeder 3

M603 将舵机驱动到特定角度
在校准进料器时，将伺服系统旋转到特定角度很有用。

用法：M603 N< 进料口> A<angle>

论点	描述
N<feeder port>	馈线端口号：000-412
A<angle>	设置伺服的角度。必须介于 0-180° 之间。如果省略，则默认为 90°。
例

M603 N2 A44 ; Set servo for feeder no. 2 to 44°

M603 N12 ; Set servo for feeder no. 12 to default (90°)

M610 启用/禁用所有控制器上的所有馈线
发出命令以启用或禁用所有控制器上的所有馈线。

用途：M610 S<status>

论点	描述
S<status>	1=开，0=关
例

M610 S1 ; enable all feeders

M610 S0 ; disable all feeders

M611 启用/禁用特定馈线
发出命令以启用或禁用特定馈线或所有馈线，通常在馈线提前作期间与 M600 结合使用。自固件 20231222 以来实施。

提示

一些送料器采用机械限制器，并且永远不会达到伺服前进角度，因此在进给作后禁用送料器以避免伺服电机过度使用和发热是有用的。

用法：M611 N< 进料器端口> S<status>

论点	描述
N<feeder port>	可选：馈线端口号：000-412
S<status>	1=开，0=关
例

M611 S1 ; enable all feeders

M611 S0 ; disable all feeders

M611 N206 S1 ; enable feeder 6 on controller 2, restoring the position from memory

M611 N206 S0 ; disable feeder 6 on controller 2, retaining the position in memory

M620 更新馈线配置
发出的命令以调整特定馈线的设置。

用法： M620 N<feeder port> A<advance angle> B<half advance angle> C<retract angle> F<default feed length> U<settle time> V<min pulsewidth> W<max pulsewidth> X<ignore feedback pin>

论点	描述
N<feeder port>	馈线端口号：000-412
A<advance angle>	进料臂的伺服角度完全先进
B<half advance angle>	送料臂半推进的伺服角度
C<retract angle>	进料臂缩回的伺服角度
F<feedlength>	进给长度，必须是 2 毫米的倍数，最大 24 毫米，通常为 4 毫米
U<settle time>	从进角到退角和反转的固定时间
V<min pulsewidth>	伺服为 0° 的 PWM 脉冲宽度
W<max pulsewidth>	伺服处于 180° 的 PWM 脉冲宽度
X<override error>	如果反馈线出现错误，则使用 X1 忽略它
默认值： M620 N<feeder port> A180 B125 C56 F4 U480 V500 W2500 X1

例

M620 N103 A180 B125 C55 F4 U500 V500 W2500 X1; save new settings for feeder 103

M621 读取馈线配置
发出命令以读取所有馈线端口的设置。

用法：M621

论点	描述
none	-