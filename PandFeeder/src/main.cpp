#include <Arduino.h>
#include "pf_servo.h"
#include "pf_gcode.h"
#include "pf_uart.h"
#include "pf_led.h"
#include "pf_board.h"

void setup() {
  Serial.begin(115200);
  delay(50);
  pf_servo::init();
  pf_servo::setEnabled(true);
  pf_uart::init();
  pf_board::initAddress();
  pf_led::init();
  pf_led::set(0,10,0); // brief green to indicate boot
  // Serial.println(F("PandFeeder ready. Send G-code over Serial (e.g., M115, M280 P0 S90, M600 N0 F8)"));
}

void loop() {
  pf_gcode::listen();
  // LED heartbeat
  static uint32_t lastLed = 0;
  uint32_t now = millis();
  if (now - lastLed > 30) { // faster update ~33Hz for quicker breathing
    // Use effect selection (default breath)
    pf_led::breathingStep(); // directly call; dispatcher optional
    lastLed = now;
  }
  // Also accept commands from IN UART
  String line;
  if (pf_uart::readLineFromIn(line)) {
    // Inject to G-code processor
    pf_gcode::injectLine(line + "\n");
  }
  delay(1);
}