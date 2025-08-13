#include "pf_uart.h"

// Configuration: you can adjust pins/baud here or move to pf_config.h
#ifndef UART_IN_NUM
#define UART_IN_NUM 1
#endif
#ifndef UART_OUT_NUM
#define UART_OUT_NUM 2
#endif
#ifndef UART_IN_BAUD
#define UART_IN_BAUD 115200
#endif
#ifndef UART_OUT_BAUD
#define UART_OUT_BAUD 115200
#endif

// Pin mapping per user requirement:
// IN UART: GPIO1 (TX), GPIO0 (RX)
// OUT UART: GPIO21 (TX), GPIO20 (RX)
#ifndef UART1_TX_PIN
#define UART1_TX_PIN 1
#endif
#ifndef UART1_RX_PIN
#define UART1_RX_PIN 0
#endif
#ifndef UART2_TX_PIN
#define UART2_TX_PIN 21
#endif
#ifndef UART2_RX_PIN
#define UART2_RX_PIN 20
#endif

static HardwareSerial SerialIn(UART_IN_NUM);
static HardwareSerial SerialOut(UART_OUT_NUM);

namespace pf_uart {

void init() {
  SerialIn.begin(UART_IN_BAUD, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);
  SerialOut.begin(UART_OUT_BAUD, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);
}

bool readLineFromIn(String &outLine) {
  static String buf;
  while (SerialIn.available()) {
    char c = (char)SerialIn.read();
    if (c == '\r') continue;
    if (c == '\n') {
      outLine = buf;
      buf = "";
      return true;
    }
    buf += c;
  }
  return false;
}

void writeOut(const String &line) {
  SerialOut.println(line);
}

size_t writeOutRaw(const uint8_t *data, size_t len) {
  return SerialOut.write(data, len);
}

} // namespace pf_uart
