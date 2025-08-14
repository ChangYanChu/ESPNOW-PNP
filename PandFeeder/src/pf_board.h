#ifndef PF_BOARD_H
#define PF_BOARD_H

#include <Arduino.h>
#include "pf_config.h"

#ifndef PF_ADDR_PIN0
#define PF_ADDR_PIN0 5
#endif
#ifndef PF_ADDR_PIN1
#define PF_ADDR_PIN1 6
#endif
#ifndef PF_ADDR_PIN2
#define PF_ADDR_PIN2 7
#endif

namespace pf_board {

void initAddress();
uint8_t address();               // 0..7 (from pins)
uint16_t baseIndex();            // address * SERVO_CHANNEL_COUNT
bool globalToLocal(int globalId, int &localOut); // returns true if belongs to this board

}

#endif // PF_BOARD_H
