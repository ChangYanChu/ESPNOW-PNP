#ifndef PF_LED_H
#define PF_LED_H

#include <Arduino.h>

namespace pf_led {

enum Effect {
	EFFECT_BREATH = 0,
	EFFECT_STATIC = 1,
	EFFECT_RAINBOW = 2
};

// Initialize WS2812B on fixed GPIO4 (can be overridden with PF_LED_PIN macro before include)
void init();

// Set a single RGB color for pixel 0 (for now single LED) and show.
void set(uint8_t r, uint8_t g, uint8_t b);

// Convenience helpers
void setHSV(uint16_t h, uint8_t s, uint8_t v); // h:0-359
void rainbowStep(); // simple cycling
// Retro green breathing effect (call periodically, e.g., every 20-50ms)
void breathingStep();

// Effect selection + brightness (0..255 logical scaling)
void setEffect(Effect e);
Effect getEffect();
void setBrightness(uint8_t b); // logical brightness scaling
uint8_t getBrightness();

// For static effect color
void setStaticColor(uint8_t r, uint8_t g, uint8_t b);

}

#endif // PF_LED_H
