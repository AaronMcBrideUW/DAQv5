
#include "SYS.h"

#define pmask (1 << pin.number)
#define pgroup PORT->Group[pin.group]

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN LOCAL
///////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t PIN_PULLSTATE[BOARD_PIN_COUNT] = { 0 };

static bool validPin(const int &pinID) {
  return pinID >= 0 && pinID < BOARD_PIN_COUNT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool reset_pin(int pinID) {
  if (!validPin(pinID)) 
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];

  pgroup.DIRCLR.reg |= pmask;
  pgroup.OUTCLR.reg |= pmask;
  pgroup.PINCFG[pin.number].reg &= PORT_PINCFG_RESETVALUE;
  pgroup.PMUX[pin.number].reg &= PORT_PMUX_RESETVALUE;
  return true;
}




bool attach_pin(int pinID, int periphID) {
  if (!validPin(pinID))
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID]; 

  if (periphID >= 0) {
    pgroup.PMUX[pin.number].reg = PORT_PMUX_MASK | (pin.number % 2) 
      ? PORT_PMUX_PMUXO((uint8_t)periphID) : PORT_PMUX_PMUXE((uint8_t)periphID);
    pgroup.PINCFG[pin.number].bit.PMUXEN = 1;
  } else {
    pgroup.PINCFG[pin.number].bit.PMUXEN = 0;      
  }
  return true;
}



bool drive_pin(int pinID, int pinState, bool pullPin) {
  if (!validPin(pinID))
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];
  pgroup.PINCFG[pin.number].bit.PULLEN = (uint8_t)pullPin;
  pgroup.PINCFG[pin.number].bit.INEN = 0;
  
  if (pinState) {
    pgroup.PINCFG[pin.number].bit.DRVSTR = (uint8_t)(abs(pinState) >= 2);
    if (pinState >= 1) {
      pgroup.OUTSET.reg |= pmask;
    } else {
      pgroup.OUTCLR.reg |= pmask;
    }
    if (pullPin) {
      pgroup.DIRCLR.reg |= pmask;
    } else {
      pgroup.DIRSET.reg |= pmask; 
    }
  } else {
    pgroup.DIRCLR.reg |= pmask;
    pgroup.OUTCLR.reg |= pmask;
  }
  return true;
}



int read_pin(int pinID) {
  if (!validPin(pinID)) 
    return -1;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];
  pgroup.PINCFG[pin.number].bit.INEN = 1;
  return (int)(pgroup.IN.reg & pmask);
}





/*
bool set_pin(int pinID, PIN_TYPE type) {
  const PIN_DESCRIPTOR *pin = get_pin(pinID);
  if (!pin)
    return false;   

  if (type == PIN_DISABLED) {
    port.PINCFG[pin->number].reg = PORT_PINCFG_RESETVALUE;
    port.PMUX[pin->number].reg = PORT_PMUX_RESETVALUE;
    port.DIRCLR.reg |= (1 << pin->number);
    port.OUTCLR.reg |= (1 << pin->number);

  } else {
    port.PINCFG[pin->number].reg = PORT_PINCFG_MASK
      | PORT_PINCFG_INEN;

    port.DIRCLR.reg |= (1 << pin->number);
    port.OUTCLR.reg |= (1 << pin->number);

    if (type != PIN_GPIO) { // Mode = peripheral
      port.PMUX[pin->number].reg = (pin->number % 2) ? PORT_PMUX_PMUXE((uint8_t)type) 
        : PORT_PMUX_PMUXO((uint8_t)type);
      port.PINCFG[pin->number].bit.PMUXEN = 1;
    }   
  }
  return true;
}


bool pin_out(int pinID, int outState) {
  const PIN_DESCRIPTOR *pin = get_pin(pinID);
  if (!pin)
    return false;

  if (outState == 1) {
    port.DIRSET.reg |= (1 << pin->number);
  } else if (outState == -1) {
    port.DIR
  } else {

  }
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PROG FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

void prog_restart() {
  __disable_irq();
  NVIC_SystemReset();
}