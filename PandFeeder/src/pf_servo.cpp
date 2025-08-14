#include "pf_servo.h"
#include <Wire.h>
#include "HCPCA9685.h"
#include "pf_config.h"

static HCPCA9685 pwm(PCA9685_I2C_ADDR);
static bool g_enabled = true; // global OE

struct FeederRuntime {
    pf_servo::FeederConfig cfg;
    bool enabled; // logical per-feeder enable
    int lastAngle;
};

static FeederRuntime g_feeders[SERVO_CHANNEL_COUNT];
// Logical ID (0..SERVO_CHANNEL_COUNT-1) to PCA9685 physical channel
// Custom remap for upper channels per user specification:
// physical: 8->11,9->12,10->13,11->14,12->15,13->10,14->9,15->8 ; others identity
static uint8_t logicalToPhysical(uint8_t id) {
    // Identity for 0..7
    if (id <= 7) return id;
    // Mapping table for 8..14 logical -> physical
    switch(id) {
        case 8: return 15;  // logical8 -> PWM15 (described as servo8 on PWM15)
        case 9: return 14;  // logical9 -> PWM14
        case 10: return 13; // logical10 -> PWM13
        case 11: return 8;  // logical11 -> PWM8
        case 12: return 9;  // logical12 -> PWM9
        case 13: return 10; // logical13 -> PWM10
        case 14: return 11; // logical14 -> PWM11
        case 15: return 12; // logical15 -> PWM12
        default: return id; // fallback
    }
}

namespace pf_servo {

static int mapAngleToTicks(int deg) {
    if (deg < SERVO_MIN_ANGLE) deg = SERVO_MIN_ANGLE;
    if (deg > SERVO_MAX_ANGLE) deg = SERVO_MAX_ANGLE;
    // Use Arduino-style map but with integers
    long t = (long)(deg - SERVO_MIN_ANGLE) * (SERVO_MAX_TICKS - SERVO_MIN_TICKS) / (SERVO_MAX_ANGLE - SERVO_MIN_ANGLE) + SERVO_MIN_TICKS;
    return (int)t;
}

void scanI2C(Stream &out) {
    out.println(F("I2C scan..."));
    byte error, address;
    int nDevices = 0;
    for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            out.print(F("Found device at 0x"));
            if (address<16) out.print("0");
            out.println(address, HEX);
            nDevices++;
        }
    }
    if (nDevices == 0) out.println(F("No I2C devices found"));
}

void init() {
    Wire.begin();
    delay(10);
    // OE pin
    pinMode(PCA9685_OE_PIN, OUTPUT);
    digitalWrite(PCA9685_OE_PIN, LOW);

    pwm.Init(SERVO_MODE);
    pwm.Sleep(false);

    // init defaults
    for (uint8_t i = 0; i < SERVO_CHANNEL_COUNT; ++i) {
        g_feeders[i].cfg.fullAdvanceAngle = DEFAULT_FULL_ADVANCE_ANGLE;
        g_feeders[i].cfg.halfAdvanceAngle = (DEFAULT_FULL_ADVANCE_ANGLE + DEFAULT_RETRACT_ANGLE) / 2;
        g_feeders[i].cfg.retractAngle = DEFAULT_RETRACT_ANGLE;
        g_feeders[i].cfg.defaultFeedLen = 4;
        g_feeders[i].cfg.settleTimeMs = DEFAULT_SETTLE_TIME;
        g_feeders[i].cfg.minTicks = SERVO_MIN_TICKS;
        g_feeders[i].cfg.maxTicks = SERVO_MAX_TICKS;
        g_feeders[i].cfg.ignoreFeedback = true;
        g_feeders[i].enabled = true;
        g_feeders[i].lastAngle = DEFAULT_FULL_ADVANCE_ANGLE;
    }
}

bool setEnabled(bool enable) {
    g_enabled = enable;
    digitalWrite(PCA9685_OE_PIN, enable ? LOW : HIGH); // OE active low
    return g_enabled;
}

bool isEnabled() {
    return g_enabled;
}

bool setAngle(uint8_t id, int angleDeg) {
    if (id >= SERVO_CHANNEL_COUNT) return false;
    if (!g_enabled || !g_feeders[id].enabled) return false;
    // apply per-feeder PWM range
    if (angleDeg < SERVO_MIN_ANGLE) angleDeg = SERVO_MIN_ANGLE;
    if (angleDeg > SERVO_MAX_ANGLE) angleDeg = SERVO_MAX_ANGLE;
    long t = (long)(angleDeg - SERVO_MIN_ANGLE) * (g_feeders[id].cfg.maxTicks - g_feeders[id].cfg.minTicks) / (SERVO_MAX_ANGLE - SERVO_MIN_ANGLE) + g_feeders[id].cfg.minTicks;
    int ticks = (int)t;
    uint8_t phys = logicalToPhysical(id);
    pwm.Servo(phys, ticks);
    g_feeders[id].lastAngle = angleDeg;
    return true;
}

bool feed(uint8_t id, uint8_t feedLength) {
    if (id >= SERVO_CHANNEL_COUNT) return false;
    if (!g_enabled || !g_feeders[id].enabled) return false;
    if (feedLength < 2 || feedLength > 24 || (feedLength % 2) != 0) return false;

    // emulate 2mm granularity: each 4mm = full cycle; 2mm = half-advance only
    uint8_t cycles = feedLength / 4;
    bool half = (feedLength % 4) == 2;

    for (uint8_t i = 0; i < cycles; i++) {
        setAngle(id, g_feeders[id].cfg.fullAdvanceAngle);
        delay(g_feeders[id].cfg.settleTimeMs);
        setAngle(id, g_feeders[id].cfg.retractAngle);
        delay(g_feeders[id].cfg.settleTimeMs);
        setAngle(id, g_feeders[id].cfg.fullAdvanceAngle);
        delay(g_feeders[id].cfg.settleTimeMs);
    }
    if (half) {
        setAngle(id, g_feeders[id].cfg.halfAdvanceAngle);
        delay(g_feeders[id].cfg.settleTimeMs);
        setAngle(id, g_feeders[id].cfg.retractAngle);
        delay(g_feeders[id].cfg.settleTimeMs);
        setAngle(id, g_feeders[id].cfg.fullAdvanceAngle);
        delay(g_feeders[id].cfg.settleTimeMs);
    }
    return true;
}

bool setFeederEnabled(uint8_t id, bool enable) {
    if (id >= SERVO_CHANNEL_COUNT) return false;
    g_feeders[id].enabled = enable;
    return true;
}

bool isFeederEnabled(uint8_t id) {
    if (id >= SERVO_CHANNEL_COUNT) return false;
    return g_feeders[id].enabled;
}

bool setConfig(uint8_t id, const pf_servo::FeederConfig &cfg) {
    if (id >= SERVO_CHANNEL_COUNT) return false;
    g_feeders[id].cfg = cfg;
    return true;
}

bool getConfig(uint8_t id, pf_servo::FeederConfig &out) {
    if (id >= SERVO_CHANNEL_COUNT) return false;
    out = g_feeders[id].cfg;
    return true;
}

bool getStatus(uint8_t id, pf_servo::FeederStatus &out) {
    if (id >= SERVO_CHANNEL_COUNT) return false;
    out.globalEnabled = g_enabled;
    out.feederEnabled = g_feeders[id].enabled;
    out.lastAngle = g_feeders[id].lastAngle;
    out.cfg = g_feeders[id].cfg;
    return true;
}

} // namespace pf_servo
