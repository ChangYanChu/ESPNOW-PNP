#ifndef GCODE_H
#define GCODE_H

#include <Arduino.h>

// G-code command definitions
#define MCODE_SET_FEEDER_ENABLE 610
#define MCODE_ADVANCE 600

// Function declarations
void processCommand();
void sendAnswer(uint8_t error, String message);
void sendAnswer(int error, const __FlashStringHelper *message);
float parseParameter(char code, float defaultVal);
bool validFeederNo(int8_t signedFeederNo, uint8_t feederNoMandatory = 0);
void listenToSerialStream();

#endif // GCODE_H