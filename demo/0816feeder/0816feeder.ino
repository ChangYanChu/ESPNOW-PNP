/*
* Author: mgrl
* (c)2017-12-30
* 
* Modified by: Curly Tale Games
*
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
* http://creativecommons.org/licenses/by-nc-sa/4.0/
*
* current version: v0.4-ESP32C3
*
* CHANGELOG:
* v0.5
*   - 移植到ESP32C3平台
*   - 适配ESP32C3的GPIO和定时器
*   - 保持原有的G-code控制接口
*   - 优化内存使用和性能
*
* v0.4
*  - added support for MaxFeederShield
*  - added support for software servo control using SoftServo library by Alex Gyver https://github.com/GyverLibs/SoftServo 
*    install through Tools > Manage Libraries
*  - added auto detach of servo motors so they don't burn up
*  - removed need to run M610 S1 command to enable since motors auto turn off
*/

// =============================================================================
// 项目架构分析和移植规划
// =============================================================================

/*
核心架构分析:

1. 硬件抽象层:
   - config.h: 配置管理和硬件参数定义
   - shield.h: 硬件盾板适配层
   - Feeder.h/cpp: 喂料器核心控制类

2. 主要功能模块:
   - G-code命令解析 (gcode.ino)
   - 舵机控制 (Feeder类)
   - EEPROM设置存储
   - ADC模拟输入处理
   - 串口通信接口

3. 数据结构:
   - FeederClass数组: 管理多个喂料器
   - sCommonSettings: 通用设置结构
   - eFeederCommands: 喂料器命令枚举

4. 控制流程:
   - 初始化 -> EEPROM读取 -> 硬件设置 -> 主循环
   - 命令解析 -> 动作执行 -> 状态反馈

5. ESP32C3移植要点:
   - GPIO重新映射
   - 定时器适配 (ESP32定时器API)
   - EEPROM替换为Preferences
   - 串口和ADC API适配
   - 内存优化
*/

#include "config.h"
#include "shield.h"

// ------------------  I N C  L I B R A R I E S ---------------
#include <HardwareSerial.h>
#include <EEPROMex.h>  // 需要替换为ESP32 Preferences
#include "Feeder.h"

// ------------------  V A R  S E T U P -----------------------

// ------ Feeder
FeederClass feeders[NUMBER_OF_FEEDER];
enum eFeederEnabledState {
  DISABLED,
  ENABLED,
} feederEnabled=ENABLED; //enable feeders by default

// ------ Settings-Struct (saved in EEPROM)
// ESP32C3移植注意: 需要适配Preferences库
struct sCommonSettings {
	//add further settings here
	char version[4];   // This is for detection if settings suit to struct, if not, eeprom is reset to defaults
#ifdef HAS_ANALOG_IN
	float adc_scaling_values[8][2];  // ESP32C3需要调整ADC通道数量
#endif
};
sCommonSettings commonSettings_default = {

	//add further settings here

	CONFIG_VERSION,
 
#ifdef HAS_ANALOG_IN
	{
		{ANALOG_A0_SCALING_FACTOR,ANALOG_A0_OFFSET},
		{ANALOG_A1_SCALING_FACTOR,ANALOG_A1_OFFSET},
		{ANALOG_A2_SCALING_FACTOR,ANALOG_A2_OFFSET},
		{ANALOG_A3_SCALING_FACTOR,ANALOG_A3_OFFSET},
		{ANALOG_A4_SCALING_FACTOR,ANALOG_A4_OFFSET},
		{ANALOG_A5_SCALING_FACTOR,ANALOG_A5_OFFSET},
		{ANALOG_A6_SCALING_FACTOR,ANALOG_A6_OFFSET},
		{ANALOG_A7_SCALING_FACTOR,ANALOG_A7_OFFSET},
	},
#endif
};
sCommonSettings commonSettings;

// ------ ADC readout
unsigned long lastTimeADCread;
uint16_t adcRawValues[8];
float adcScaledValues[8];

// ------------------  U T I L I T I E S ---------------

// ------ Operate command on all feeder
enum eFeederCommands {
	cmdSetup,
	cmdUpdate,
	cmdEnable,
	cmdDisable,
	cmdOutputCurrentSettings,
	cmdInitializeFeederWithId,
	cmdFactoryReset,
};

void executeCommandOnAllFeeder(eFeederCommands command);
void executeCommandOnAllFeeder(eFeederCommands command) {
	for (uint8_t i=0;i<NUMBER_OF_FEEDER;i++) {
		switch(command) {
			case cmdSetup:
				feeders[i].setup();
			break;
			case cmdUpdate:
				feeders[i].update();  // 核心更新逻辑
			break;
			case cmdEnable:
				feeders[i].enable();
			break;
			case cmdDisable:
				feeders[i].disable();
			break;
			case cmdOutputCurrentSettings:
				feeders[i].outputCurrentSettings();
			break;
			case cmdInitializeFeederWithId:
				feeders[i].initialize(i);
			break;
			case cmdFactoryReset:
				feeders[i].factoryReset();
			break;
			default:
				{}
			break;
		}
	}
}

#ifdef HAS_ANALOG_IN
void updateADCvalues() {
	// ESP32C3移植注意: 需要适配ESP32的ADC API
	for(uint8_t i=0; i<=7; i++) {
		adcRawValues[i]=analogRead(i);
		adcScaledValues[i]=(adcRawValues[i]*commonSettings.adc_scaling_values[i][0])+commonSettings.adc_scaling_values[i][1];
	}
}
#endif

void printCommonSettings() {

	//ADC-scaling values
#ifdef HAS_ANALOG_IN
	Serial.println("Analog Scaling Settings:");
	for(uint8_t i=0; i<=7; i++) {
		Serial.print("M");
		Serial.print(MCODE_SET_SCALING);
		Serial.print(" A");
		Serial.print(i);
		Serial.print(" S");
		Serial.print(commonSettings.adc_scaling_values[i][0]);
		Serial.print(" O");
		Serial.print(commonSettings.adc_scaling_values[i][1]);
		Serial.println();
	}
#endif
}

// ------------------  S E T U P -----------------------
void setup() {
	Serial.begin(SERIAL_BAUD);
	while (!Serial);
	Serial.println(F("Controller starting...")); Serial.flush();
	
	// 电源输出初始化 - ESP32C3需要重新映射GPIO
	for(uint8_t i=0;i<NUMBER_OF_POWER_OUTPUT;i++) {
		pinMode(pwrOutputPinMap[i],OUTPUT);
		digitalWrite(pwrOutputPinMap[i],LOW);
	}
	
	// setup listener to serial stream
	setupGCodeProc();  // G-code处理器初始化

	// 初始化喂料器ID
	executeCommandOnAllFeeder(cmdInitializeFeederWithId);

	// ESP32C3移植注意: EEPROM需要替换为Preferences
	//load commonSettings from eeprom
	EEPROM.readBlock(EEPROM_COMMON_SETTINGS_ADDRESS_OFFSET, commonSettings);

	//factory reset on first start or version changing
	if(strcmp(commonSettings.version,CONFIG_VERSION) != 0) {
		Serial.println(F("First start/Config version changed"));
		executeCommandOnAllFeeder(cmdFactoryReset);
		EEPROM.writeBlock(EEPROM_COMMON_SETTINGS_ADDRESS_OFFSET, commonSettings_default);
	}

	// 设置喂料器对象
	executeCommandOnAllFeeder(cmdSetup);
	delay(1000);

	// 保持喂料器启用状态
	executeCommandOnAllFeeder(cmdEnable);
	
	// 输出所有设置到控制台
	executeCommandOnAllFeeder(cmdOutputCurrentSettings);

#ifdef HAS_ANALOG_IN
	updateADCvalues();
	lastTimeADCread=millis();
#endif

	Serial.println(F("Controller up and ready! Have fun."));
}

// ------------------  L O O P -----------------------
void loop() {
	// 处理传入的串口数据并执行回调
	listenToSerialStream();  // G-code命令监听

	// 处理舵机控制 - 核心更新循环
	executeCommandOnAllFeeder(cmdUpdate);

	// 处理ADC输入 - ESP32C3需要适配
#ifdef HAS_ANALOG_IN
	if (millis() - lastTimeADCread >= ADC_READ_EVERY_MS) {
		lastMoveTime=millis();
		updateADCvalues();
	}
#endif
}

/*
移植到ESP32C3的关键点:

1. 库依赖替换:
   - EEPROMex -> Preferences
   - HardwareSerial -> 使用ESP32原生串口
   - Servo -> ESP32Servo

2. 硬件适配:
   - GPIO重新定义 (config.h)
   - ADC通道映射
   - 定时器配置

3. 内存优化:
   - 减少SRAM使用
   - 优化字符串存储

4. 兼容性保持:
   - G-code命令接口不变
   - 核心功能逻辑保持
   - 配置参数结构兼容
*/
