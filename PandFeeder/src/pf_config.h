#ifndef PF_CONFIG_H
#define PF_CONFIG_H

// No Arduino headers required in this config-only header

// PCA9685 I2C address and OE pin (migrated from previous main.cpp)
#ifndef PCA9685_I2C_ADDR
#define PCA9685_I2C_ADDR 0x40
#endif

#ifndef PCA9685_OE_PIN
#define PCA9685_OE_PIN 10
#endif

// Servo channels and angle constraints
#ifndef SERVO_CHANNEL_COUNT
#define SERVO_CHANNEL_COUNT 13 // channels 0..12
#endif

#ifndef SERVO_MIN_ANGLE
#define SERVO_MIN_ANGLE 0
#endif

#ifndef SERVO_MAX_ANGLE
#define SERVO_MAX_ANGLE 180
#endif

// Mapping from degrees to HCPCA9685::Servo units (as used in existing code)
#ifndef SERVO_MIN_TICKS
#define SERVO_MIN_TICKS 0
#endif

#ifndef SERVO_MAX_TICKS
#define SERVO_MAX_TICKS 420
#endif

// Feed action defaults (align with hand_servo.cpp semantics)
#ifndef DEFAULT_FULL_ADVANCE_ANGLE
#define DEFAULT_FULL_ADVANCE_ANGLE 180
#endif

#ifndef DEFAULT_RETRACT_ANGLE
#define DEFAULT_RETRACT_ANGLE 55
#endif

#ifndef DEFAULT_SETTLE_TIME
#define DEFAULT_SETTLE_TIME 300 // ms
#endif

#endif // PF_CONFIG_H
