#ifndef PF_SERVO_H
#define PF_SERVO_H

#include <Arduino.h>

bool feed(uint8_t id, uint8_t feedLength);
namespace pf_servo {

struct FeederConfig {
	int fullAdvanceAngle;   // A
	int halfAdvanceAngle;   // B
	int retractAngle;       // C
	uint8_t defaultFeedLen; // F
	uint16_t settleTimeMs;  // U
	uint16_t minTicks;      // V (ticks)
	uint16_t maxTicks;      // W (ticks)
	bool ignoreFeedback;    // X
};

struct FeederStatus {
	bool globalEnabled; // OE state
	bool feederEnabled; // logical per-feeder enable
	int lastAngle;
	FeederConfig cfg;
};

// Initialize PCA9685 and related pins
void init();

// Enable/disable output (OE pin). Returns current state after change.
bool setEnabled(bool enable);
bool isEnabled();

// Per-feeder logical enable (does not toggle OE).
bool setFeederEnabled(uint8_t id, bool enable);
bool isFeederEnabled(uint8_t id);

// Set servo angle on channel id [0..SERVO_CHANNEL_COUNT-1]
// Returns false if id/angle invalid or I2C error or feeder disabled.
bool setAngle(uint8_t id, int angleDeg);

// Perform a feed action similar to original hand implementation.
// Supports 2mm granularity with half-advance handling.
bool feed(uint8_t id, uint8_t feedLength);

// Set/get per-feeder configuration
bool setConfig(uint8_t id, const FeederConfig &cfg);
bool getConfig(uint8_t id, FeederConfig &out);

// Read status snapshot
bool getStatus(uint8_t id, FeederStatus &out);

// Optional I2C scan for debugging
void scanI2C(Stream &out);

}

#endif // PF_SERVO_H
