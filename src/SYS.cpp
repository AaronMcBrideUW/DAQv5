
#include "SYS.h"

const char PERIPH_REF[] = 
  {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N'};
const int PERIPH_COUNT = sizeof(PERIPH_REF) / sizeof(PERIPH_REF[0]);

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_pin_multi(int pinNum, bool enabled, char peripheral, PIN_MODE mode) {
  const PIN_DESCRIPTOR *pin = get_pin(pinNum);
  if (pin == nullptr)
    return false; 

  #define port PORT->Group[pin->group]
  if (enabled) {
    for (int i = 0; i < PERIPH_COUNT; i++) {
      if (PERIPH_REF[i] == peripheral) {
        break;
      } else if (i >= PERIPH_COUNT - 1) {
        return false;
      }
    }
    port.PMUX[pinNum].reg = (pinNum % 2) 
      ? PORT_PMUX_PMUXE(peripheral) : PORT_PMUX_PMUXO(peripheral);
    port.PINCFG[pinNum].reg = 
        ((uint8_t)(mode == PIN_MODE_INPUT || mode == PIN_MODE_INPUT_PULLUP)
          << PORT_PINCFG_INEN_Pos)
      | ((uint8_t)(mode == PIN_MODE_OUTPUT_PULLUP || mode == PIN_MODE_INPUT_PULLUP)
          << PORT_PINCFG_PULLEN_Pos)
      | PORT_PINCFG_PMUXEN
      | PORT_PINCFG_DRVSTR;
  
  } else {
    port.PINCFG[pinNum].bit.PMUXEN = 0;
  }
  return true;
}