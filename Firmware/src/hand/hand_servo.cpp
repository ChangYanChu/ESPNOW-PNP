#include "hand_servo.h"
#include "SoftServo.h"

// 舵机对象
SoftServo myservo;

void setup_Servo()
{
    myservo.attach(SERVO_PIN);  // 连接舵机到指定引脚
    myservo.delayMode();        // 使用延迟模式，更稳定
    DEBUG_PRINTF("Servo attached to pin %d\n", SERVO_PIN);
    
    #ifdef ENABLE_SERVO_STARTUP_TEST
    // 开机测试舵机
    testServoOnStartup();
    #else
    DEBUG_PRINTLN("Servo startup test disabled");
    #endif
}

// 推进料带函数
void feedTapeAction(uint8_t feedLength)
{
    // 计算需要执行的动作次数：每4mm执行一次动作
    uint8_t actionCount = feedLength / 4;

    DEBUG_PRINTF("Feed tape: %dmm, actions: %d\n", feedLength, actionCount);

    // 执行推进动作
    for (uint8_t i = 0; i < actionCount; i++)
    {
        DEBUG_PRINTF("Action %d/%d\n", i + 1, actionCount);

        // 移动到0度位置
        myservo.write(90);
        unsigned long startTime = millis();
        while (millis() - startTime < 300) {
            myservo.tick(); // 必须持续调用tick()
            delay(1);
        }

        // 移动到80度位置
        myservo.write(0);
        startTime = millis();
        while (millis() - startTime < 300) {
            myservo.tick(); // 必须持续调用tick()
            delay(1);
        }

        // 回到0度位置
        myservo.write(90);
        startTime = millis();
        while (millis() - startTime < 300) {
            myservo.tick(); // 必须持续调用tick()
            delay(1);
        }
    }
    
    DEBUG_PRINTLN("Feed tape action completed");
}

// 舵机tick函数，需要在主循环中频繁调用
void servoTick() {
    myservo.tick();
}

// 开机测试舵机函数
void testServoOnStartup() {
    DEBUG_PRINTLN("\n=== Starting Servo Test ===");
    
    int testAngles[] = {90, 0, 80, 90}; // 测试角度序列：中间→0°→80°→中间
    const char* angleNames[] = {"Center", "0°", "80°", "Center"};
    
    for (int i = 0; i < 4; i++) {
        DEBUG_PRINTF("Test step %d: Moving to %s position (%d°)\n", 
                     i + 1, angleNames[i], testAngles[i]);
        
        myservo.write(testAngles[i]);
        
        // 等待300ms，持续调用tick
        unsigned long startTime = millis();
        while (millis() - startTime < 300) {
            myservo.tick();
            delay(1);
        }
        
        DEBUG_PRINTF("  - Reached %d°\n", testAngles[i]);
    }
    
    DEBUG_PRINTLN("=== Servo Test Complete ===\n");
    delay(500); // 额外延迟确保舵机稳定
}

void feedOnce() {
    feedTapeAction(4); // 例如推进16mm
    DEBUG_PRINTLN("Feed action completed");
}