#include "pf_board.h"

static uint8_t g_addr = 0;

namespace pf_board {

void initAddress() {
  pinMode(PF_ADDR_PIN0, INPUT_PULLUP);
  pinMode(PF_ADDR_PIN1, INPUT_PULLUP);
  pinMode(PF_ADDR_PIN2, INPUT_PULLUP);
  // Active low (jumper to GND) => bit = 1 for grounded? Decide: grounded => 1 to increase address.
  // We'll treat: LOW = 1, HIGH = 0 to allow simple jumpers to GND.
  uint8_t b0 = (digitalRead(PF_ADDR_PIN0) == LOW) ? 1 : 0;
  uint8_t b1 = (digitalRead(PF_ADDR_PIN1) == LOW) ? 1 : 0;
  uint8_t b2 = (digitalRead(PF_ADDR_PIN2) == LOW) ? 1 : 0;
  g_addr = (b2 << 2) | (b1 << 1) | b0; // 0..7
}

uint8_t address() { return g_addr; }

uint16_t baseIndex() { return (uint16_t)g_addr * SERVO_CHANNEL_COUNT; }

bool globalToLocal(int globalId, int &localOut) {
  int base = baseIndex();
  if (globalId < base) return false;
  int limit = base + SERVO_CHANNEL_COUNT - 1;
  if (globalId > limit) return false;
  localOut = globalId - base;
  return true;
}

} // namespace pf_board
