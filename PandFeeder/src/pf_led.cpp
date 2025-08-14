#include "pf_led.h"
#include <Adafruit_NeoPixel.h>

#ifndef PF_LED_PIN
#define PF_LED_PIN 4
#endif
#ifndef PF_LED_COUNT
#define PF_LED_COUNT 1
#endif

static Adafruit_NeoPixel strip(PF_LED_COUNT, PF_LED_PIN, NEO_GRB + NEO_KHZ800);
static uint16_t g_hue = 0;
static int16_t g_breathe = 0; // 0..1023 saw
static int8_t g_breatheDir = 1;
static pf_led::Effect g_effect = pf_led::EFFECT_BREATH;
static uint8_t g_brightness = 32; // lowered default brightness (0..255)
static uint8_t g_staticR=0, g_staticG=40, g_staticB=0;

namespace pf_led {

void init() {
  strip.begin();
  strip.clear();
  strip.show();
}

void set(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

static uint32_t hsvToColor(uint16_t h, uint8_t s, uint8_t v) {
  // h:0-359, s:0-255, v:0-255
  float hf = h / 60.0f;
  int i = (int)floor(hf);
  float f = hf - i;
  float pv = v * (1 - s/255.0f);
  float qv = v * (1 - s/255.0f * f);
  float tv = v * (1 - s/255.0f * (1 - f));
  float r=0,g=0,b=0;
  switch(i) {
    default:
    case 0: r=v; g=tv; b=pv; break;
    case 1: r=qv; g=v; b=pv; break;
    case 2: r=pv; g=v; b=tv; break;
    case 3: r=pv; g=qv; b=v; break;
    case 4: r=tv; g=pv; b=v; break;
    case 5: r=v; g=pv; b=qv; break;
  }
  return strip.Color((uint8_t)r, (uint8_t)g, (uint8_t)b);
}

void setHSV(uint16_t h, uint8_t s, uint8_t v) {
  if (h >= 360) h %= 360;
  strip.setPixelColor(0, hsvToColor(h, s, v));
  strip.show();
}

void rainbowStep() {
  g_hue = (g_hue + 2) % 360; // advance hue
  uint8_t v = (uint8_t)((uint16_t)g_brightness * 40 / 255);
  if (v < 5) v = 5;
  setHSV(g_hue, 255, v);
}

void breathingStep() {
  // Create a triangular wave 0..1023
  // Faster breathing: configurable step size
  const int step = 24; // was 8
  g_breathe += g_breatheDir * step; // speed step
  if (g_breathe >= 1023) { g_breathe = 1023; g_breatheDir = -1; }
  else if (g_breathe <= 0) { g_breathe = 0; g_breatheDir = 1; }
  // Apply an ease-in-out curve using a simple square for softer feel
  float x = g_breathe / 1023.0f; // 0..1
  float eased = x * x * (3 - 2 * x); // smoothstep
  // Lower brightness envelope: 5..105 before global scaling
  uint8_t base = (uint8_t)(eased * 100.0f + 5); // 5..105
  uint16_t scaled = (uint16_t)base * g_brightness / 255; // with default 32 => ~5..13
  if (scaled < 5) scaled = 5;
  if (scaled > 255) scaled = 255;
  strip.setPixelColor(0, strip.Color(0, (uint8_t)scaled, 0));
  strip.show();
}

// ----- Control API -----
void setEffect(Effect e) { g_effect = e; }
Effect getEffect() { return g_effect; }
void setBrightness(uint8_t b) { g_brightness = b; }
uint8_t getBrightness() { return g_brightness; }
void setStaticColor(uint8_t r, uint8_t g, uint8_t b) { g_staticR=r; g_staticG=g; g_staticB=b; }

// Internal dispatcher (optional; user code currently calls specific step funcs)
static void updateEffect() {
  using namespace pf_led;
  switch (g_effect) {
    case EFFECT_BREATH: breathingStep(); break;
    case EFFECT_STATIC: {
      uint16_t r = (uint16_t)g_staticR * g_brightness / 255;
      uint16_t g = (uint16_t)g_staticG * g_brightness / 255;
      uint16_t b = (uint16_t)g_staticB * g_brightness / 255;
      strip.setPixelColor(0, strip.Color(r,g,b));
      strip.show();
      break; }
    case EFFECT_RAINBOW: rainbowStep(); break;
  }
}

} // namespace pf_led
