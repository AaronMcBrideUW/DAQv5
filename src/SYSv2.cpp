
#include "SYSv2.h"

#define OSC32KCTRL_XOSC32K_STARTUP_MAX 6

void sys_set_xosc32k(const bool enabled, const uint16_t startupTime, const bool outputSignal) {

  OSC32KCTRL->XOSC32K.reg &= ~OSCCTRL_XOSCCTRL_MASK;
  OSC32KCTRL->XOSC32K.reg |= CLK_DEFAULT_CONFIG.XOSC32K_REG;

  uint8_t suLog = log2(startupTime);
  OSC32KCTRL->XOSC32K.bit.STARTUP = suLog > OSC32KCTRL_XOSC32K_STARTUP_MAX ?  

 

}

