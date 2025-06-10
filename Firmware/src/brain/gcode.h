#ifndef GCODE_H
#define GCODE_H
#include <Arduino.h>

#define NUMBER_OF_FEEDER 48 // Number of feeders supported by the system

#define MCODE_ADVANCE 600
#define MCODE_RETRACT_POST_PICK 601 
#define MCODE_FEEDER_IS_OK 602
#define MCODE_SERVO_SET_ANGLE 603
#define MCODE_SET_FEEDER_ENABLE 610
#define MCODE_UPDATE_FEEDER_CONFIG	620

#define MCODE_GET_ADC_RAW 143
#define MCODE_GET_ADC_SCALED 144
#define MCODE_SET_SCALING 145

#define MCODE_SET_POWER_OUTPUT 155

#define MCODE_FACTORY_RESET 799

void listenToSerialStream();
void sendAnswer(uint8_t error, String message);
void sendAnswer(int error, const __FlashStringHelper* message);
void processCommand();
#endif // GCODE_H