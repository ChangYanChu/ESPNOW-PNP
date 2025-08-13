#ifndef PF_UART_H
#define PF_UART_H

#include <Arduino.h>

namespace pf_uart {

// Initialize IN and OUT UARTs with configured pins/baud
void init();

// Try reading a full line (ending with '\n') from IN UART.
// Returns true if a line is available and sets outLine (without trailing '\n').
bool readLineFromIn(String &outLine);

// Write a line to OUT UART (appends '\n').
void writeOut(const String &line);

// Optional: send raw bytes to OUT
size_t writeOutRaw(const uint8_t *data, size_t len);

}

#endif // PF_UART_H
