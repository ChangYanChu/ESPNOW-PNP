#ifndef PF_GCODE_H
#define PF_GCODE_H

#include <Arduino.h>

namespace pf_gcode {

// Process incoming Serial bytes; call in loop()
void listen();

// For unit tests or external feeding
void injectLine(const String &line);

}

#endif // PF_GCODE_H
