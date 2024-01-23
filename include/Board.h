
#pragma once
#include <inttypes.h>
#include "sam.h"

#define BOARD_PIN_NUM 45

struct PinDescriptor {
  bool inUse;
  uint8_t pinNum;
  uint8_t portNum;
};

const PinDescriptor PIN_REF[BOARD_PIN_NUM] = {}; 