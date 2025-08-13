/* 文件：    HCPCA9685_Servo_Example
   日期：    2016/06/10
   版本：    0.1
   作者：    Andrew Davies

   本示例由 Hobby Components Ltd (HOBBYCOMPONENTS.COM) 创建

2016/06/10 版本 0.1：原始版本

本示例演示如何使用 HCPCA9685 库配合 PCA9685 控制最多 16 路舵机。
程序会初始化库并设置为“舵机模式”，然后让连接在 PWM 输出 0 的舵机不断往返摆动。
本示例特别适用于 hobbycomponents.com 出品的 16 通道 12 位 PWM 舵机驱动模块（HCMODU0097）。

模块与 Arduino 的连接方式如下：

PCA9685...........Uno/Nano
GND...............GND
OE................N/A
SCL...............A5
SDA...............A4
VCC...............5V

舵机的外部 5V 电源可通过接线端子的 V+ 和 GND 输入提供。

请注意：根据你的舵机型号，本程序可能会让舵机运动到极限位置。如果舵机撞到极限，请调整本程序中的最小和最大值。

你可以自由复制、修改和使用本代码，但如果你要分发，请保留对 HobbyComponents.com 的引用。
本软件不得直接用于销售与 Hobby Components Ltd 产品直接竞争的产品。

本软件按“原样”提供。HOBBY COMPONENTS 不做任何形式的保证，无论是明示、暗示还是法定保证，包括但不限于对适销性和特定用途适用性的暗示保证、准确性或无疏忽保证。
在任何情况下，HOBBY COMPONENTS 均不对任何损害承担责任，包括但不限于因任何原因造成的特殊、偶然或间接损害。
*/

/* 引入 HCPCA9685 库 */
#include "HCPCA9685.h"

/* 设备/模块的 I2C 从地址。HCMODU0097 的默认 I2C 地址为 0x40 */
#define  I2CAdd 0x40

/* 创建库实例 */
HCPCA9685 HCPCA9685(I2CAdd);

void setup() 
{
  /* 初始化库并设置为“舵机模式” */
  HCPCA9685.Init(SERVO_MODE);

  /* 唤醒设备 */
  HCPCA9685.Sleep(false);
}

void loop() 
{
  unsigned int Pos;

  /* 让舵机在最小和最大位置之间往返运动。
     如果舵机撞到极限，请调整参数，使舵机能在安全范围内运动。
     你可以通过修改 HCPCA9685.h 文件中的修正值来调整最小和最大位置。*/
  for(Pos = 10; Pos < 450; Pos++)
  {
    /* 此函数用于设置舵机位置。第一个参数为舵机编号，第二个参数为舵机位置。 */
    HCPCA9685.Servo(0, Pos);
    delay(10);
  }
  
  for(Pos = 450; Pos >= 10; Pos--)
  {
    HCPCA9685.Servo(0, Pos);
    delay(10);
  }
}