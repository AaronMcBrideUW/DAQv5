
#include "SYS.h"

#define port PORT->Group[pin->group]


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_pin(int pinID, PIN_MODE mode, bool strongDrive) {
  const PIN_DESCRIPTOR *pin = get_pin(pinID);
  if (!pin)
    return false;   

  if (mode == PIN_DISABLED) {
    port.PINCFG[pin->number].reg = PORT_PINCFG_RESETVALUE;
    port.PMUX[pin->number].reg = PORT_PMUX_RESETVALUE;
    port.DIRCLR.reg |= (1 << pin->number);
    port.OUTCLR.reg |= (1 << pin->number);

  } else {
      port.PMUX[pin->number].reg = (pin->number % 2) ? PORT_PMUX_PMUXE((uint8_t)mode) 
        : PORT_PMUX_PMUXO((uint8_t)mode);

      port.PINCFG[pin->number].reg = PORT_PINCFG_MASK
        | ((uint8_t)(mode != PIN_GPIO) << PORT_PINCFG_PMUXEN_Pos)
        | ((uint8_t)strongDrive << PORT_PINCFG_DRVSTR_Pos)
        | PORT_PINCFG_INEN;

      port.DIRCLR.reg |= (1 << pin->number);
      port.OUTCLR.reg |= (1 << pin->number);
  }
  return true;
}

bool pull_pin(int pinID, int pullType) {
  const PIN_DESCRIPTOR *pin = get_pin(pinID);
  if (!pin)
    return false;

  port.DIRCLR.reg |= (1 << pin->number);
  port.PINCFG[pin->number].bit.PULLEN = (uint8_t)abs(pullType);

  if (pullType > 0) {
    port.OUTSET.reg |= (1 << pin->number);
  } else {
    port.OUTCLR.reg |= (1 << pin->number);
  }
}

bool drive_pin(int pinID, bool high) {
  const PIN_DESCRIPTOR *pin = get_pin(pinID);
  if (!pin)
    return false;

  port.DIRSET.reg |= (1 << pin->number);

  if (high) {
    port.OUTSET.reg |= (1 << pin->number);

  } else {
    port.OUTCLR.reg |= (1 << pin->number);
  }
}

int read_pin(int pinID) {
  const PIN_DESCRIPTOR *pin = get_pin(pinID);
  if (!pin)
    return -1;

  port.DIRCLR.reg |= (1 << pin->number);
  return (uint8_t)(port.IN.reg & (1 << pin->number));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PROG FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

void prog_restart() {
  __disable_irq();
  NVIC_SystemReset();
}