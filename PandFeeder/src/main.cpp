#include <Arduino.h>
#include "pf_servo.h"
#include "pf_gcode.h"
#include "pf_uart.h"

void setup() {
  Serial.begin(115200);
  delay(50);
  pf_servo::init();
  pf_servo::setEnabled(true);
  pf_uart::init();
  Serial.println(F("PandFeeder ready. Send G-code over Serial (e.g., M115, M280 P0 S90, M600 N0 F8)"));
}

void loop() {
  pf_gcode::listen();
  // Also accept commands from IN UART
  String line;
  if (pf_uart::readLineFromIn(line)) {
    // Inject to G-code processor
    pf_gcode::injectLine(line + "\n");
  }
  delay(1);
}